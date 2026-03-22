# Performance Optimization Targets

Findings from code review of the image capture and processing pipeline.
Issues are ordered by impact.

---

## 1. DXGI Staging Texture Created Every Frame
**File:** `dxgiscreencapture.cpp:78`
**Impact:** Very High

`CreateTexture2D` is called inside `capture()`, which runs at the target frame rate (default 60fps).
D3D texture allocation involves driver-level memory management and is not cheap.

```cpp
// Currently runs every frame:
ID3D11Texture2D* cpuTex = nullptr;
hr = pD3DDevice->CreateTexture2D(&desc, nullptr, &cpuTex);
// ...
cpuTex->Release();
```

**Fix:** Promote `cpuTex` to a member variable. Allocate on first use and only reallocate
when the captured resolution changes (i.e. when `newSize != pScreenSize`).

---

## 2. `bits()` Deep Copies Throughout the Pipeline
**Files:** `lowpassfilterprocess.cpp:37-38`, `scaleprocess.cpp:76`, `dxgiscreencapture.cpp:108`
**Impact:** High
**Note:** Already flagged by in-code `TODO` comments.

`QImage` uses copy-on-write (CoW). Calling `bits()` on a shared image forces a full detach
(deep copy of the entire pixel buffer). At 1080p this is ~8MB per call. The pipeline passes
images by value through multiple stages, so several full-buffer copies are occurring per frame.

**Fix:**
- Use `constBits()` for any read-only pixel access — this does not trigger a detach.
- Ensure mutable access (`bits()`) only happens on an image with a single owner, either by
  passing by reference where the intent is in-place mutation, or by explicitly detaching once
  before entering a loop rather than implicitly inside it.

---

## 3. LowPassFilter Operates on Full-Resolution Image
**File:** `lowpassfilterprocess.cpp:40-47`
**Impact:** High

The filter iterates over every byte of the full input frame before any downsampling occurs:

```cpp
qint64 arrayLength = inputImage.sizeInBytes(); // ~8MB at 1080p, every frame
for (qint32 i = 0; i < arrayLength; ++i) { ... }
```

The filter's purpose is to smooth the LED output over time. Since the LED data is a small
set of averaged strip pixels, applying temporal smoothing to the full megapixel image is
doing orders of magnitude more work than needed.

**Fix:** Move `LowPassFilterProcess` to execute **after** `ImageTransform`, operating on the
small `ImageStrips` output (a few pixels per LED) rather than on the full capture frame.
This also makes the filter more semantically correct — it smooths the LED values, not the
raw screen content.

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
