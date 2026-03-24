# Performance Optimization Targets

Findings from code review of the image capture and processing pipeline.
Issues are ordered by impact.

---

## ~~1. DXGI Staging Texture Created Every Frame~~ ✅ Done
**File:** `dxgiscreencapture.cpp`

`pCpuTex` promoted to a member variable. Texture is now reused across frames and only
reallocated when the captured resolution changes.

---

## ~~2. `bits()` Deep Copies Throughout the Pipeline~~ ✅ Done
**Files:** `dxgiscreencapture.cpp`, `scaleprocess.cpp`

`QImage` uses copy-on-write (CoW). Calling `bits()` on a shared image forces a full detach
(deep copy of the entire pixel buffer). At 1080p this is ~8MB per call.

- **DXGI path:** A fresh local `QImage writeFrame` is allocated as the sole owner before
  writing, avoiding the CoW detach on capture.
- **ScaleProcess path:** Output image `pOutImage` promoted to a member and pre-allocated;
  `CopyPixels` writes directly into a single-owner buffer. `constBits()` used for read-only
  input access.

---

## ~~3. LowPassFilter Operates on Full-Resolution Image~~ ✅ Done
**File:** `processmanager.cpp`

`LowPassFilterProcess` moved to execute after `ImageTransform`. Four dedicated
`LowPassFilterProcess` instances in `ProcessManager` now operate on the small strip images
instead of the full capture frame.

---

## ~~4. WICBitmap Allocated and Released Every Frame~~ ✅ Done
**File:** `scaleprocess.cpp`

`pWICBitmap` promoted to a member variable alongside `pWICScaler` and `pWICFactory`.
On subsequent frames with the same input dimensions, pixel data is updated in-place via
`IWICBitmapLock` instead of reallocating. Only recreated on resolution change.
`pWICScaler` was already cached; no change needed there.

---

## ~~5. Strip Buffer Allocations per Frame~~ ⛔ Won't Do

Only one transform is active at a time, so the real cost is 4 strips × 2–3 allocations =
8–12 allocations per frame depending on transform type. Eliminating these would require
replacing `QImage::copy()` and `scaled()` with manual pixel loops into pre-allocated
buffers — significant rewrite for marginal gain. The allocation cost is small relative to
the pixel work in `scaled()`. The GPU downsampling architectural change would eliminate
this entire stage anyway.

---

## ~~6. Bug: Broken Switch in Non-Windows ScaleProcess~~ ✅ Done
**File:** `scaleprocess.cpp`

`case LinearScale | CubicScale` (bitwise OR, value 3, matched neither enumerator) replaced
with two proper fall-through cases. `default` added to cover future enum additions and
eliminate undefined behaviour from falling off a non-void function.

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
