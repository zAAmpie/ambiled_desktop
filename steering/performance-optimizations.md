# Performance Optimization Targets

Findings from code review of the image capture and processing pipeline.
Issues are ordered by impact.

---

## ~~1. DXGI Staging Texture Created Every Frame~~ ✅ Done
**File:** `dxgiscreencapture.cpp`

`pCpuTex` promoted to a member variable. Texture is now reused across frames and only
reallocated when the captured resolution changes.

---

## 2. `bits()` Deep Copies Throughout the Pipeline
**Files:** `scaleprocess.cpp:76`, `dxgiscreencapture.cpp` (partially fixed)
**Impact:** High
**Note:** Already flagged by in-code `TODO` comments.

`QImage` uses copy-on-write (CoW). Calling `bits()` on a shared image forces a full detach
(deep copy of the entire pixel buffer). At 1080p this is ~8MB per call.

**Status:** The DXGI capture path was fixed — a fresh local `QImage writeFrame` is now
allocated as the sole owner before writing, avoiding the CoW detach. Remaining site:
- `scaleprocess.cpp:76` — `CreateBitmapFromMemory` writes into a `QImage` retrieved via
  `bits()`. The image may be shared. Use `constBits()` for read-only access; ensure the
  output image is freshly allocated before writing.

---

## ~~3. LowPassFilter Operates on Full-Resolution Image~~ ✅ Done
**File:** `processmanager.cpp`

`LowPassFilterProcess` moved to execute after `ImageTransform`. Four dedicated
`LowPassFilterProcess` instances in `ProcessManager` now operate on the small strip images
instead of the full capture frame.

---

## 4. WICBitmap Allocated and Released Every Frame
**File:** `scaleprocess.cpp:75-117`
**Impact:** Medium

On Windows, `ScaleProcess::process()` calls `CreateBitmapFromMemory` and `Release` on every
invocation. The output `QImage` is also freshly constructed each frame.

```cpp
IWICBitmap *WICBitmap = nullptr;
hr = pWICFactory->CreateBitmapFromMemory(..., &WICBitmap);
// ... scale ...
WICBitmap->Release(); // every frame
```

**Fix:** Cache `WICBitmap` as a member. Reuse it when input dimensions haven't changed,
only reallocating on resolution change. Similarly, pre-allocate the output `QImage` and
reuse its buffer by writing into `outImage.bits()` in-place.

---

## 5. Strip Buffer Allocations per Frame
**Files:** `imagetransform.cpp:37-47`, `striptransform.cpp:27-29`, `blocktransform.cpp:27`
**Impact:** Medium

Each call to `transform()` produces 4 strips. For each strip:
- `copyStrip` calls `QImage::copy()` — allocates a new image
- `StripTransform` calls `scaled()` twice — two more allocations
- `BlockTransform` calls `scaled()` once

That's up to 12 QImage heap allocations per frame in the transform stage alone.

**Fix:** Pre-allocate strip output images as member variables in each transform class.
Reuse them each frame by writing into existing buffers. Check that dimensions still match
before reuse; reallocate only on change.

---

## 6. Bug: Broken Switch in Non-Windows ScaleProcess
**File:** `scaleprocess.cpp:138`
**Impact:** Correctness bug on Linux/X11

```cpp
case LinearScale | CubicScale:  // Bitwise OR — matches neither value individually
    return inputImage.scaled(...);
// No default: — falls off end of non-void function → undefined behaviour
```

This means on non-Windows builds, any call with `LinearScale` or `CubicScale` hits UB.

**Fix:**
```cpp
case LinearScale:
case CubicScale:
    return inputImage.scaled(..., Qt::SmoothTransformation);
default:
    return inputImage.scaled(..., Qt::FastTransformation);
```

---

## Potential Architectural Win: GPU-Side Downsampling (DXGI)

Not a bug fix, but the highest-ceiling optimization available on Windows.

Currently the full desktop frame (~8MB at 1080p) is read back from GPU to CPU every frame,
then the CPU averages it down to a handful of LED color values. Instead:

1. Run a D3D11 compute shader (or simple render pass) on the GPU that samples only the edge
   strips and writes out one colour value per LED.
2. Read back only the tiny result buffer (e.g. 200 LEDs × 4 bytes = 800 bytes).

This reduces PCIe readback bandwidth from ~8MB/frame to ~1KB/frame, which at 60fps is the
difference between ~480MB/s and ~60KB/s of bus traffic. The CPU processing loop disappears
entirely for the DXGI path.

This is a significant rewrite and is Windows/DXGI-specific, but the other capture backends
(X11, GDI, D3D9) could still use the CPU pipeline.
