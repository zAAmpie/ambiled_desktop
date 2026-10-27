#include "imageutils.h"

namespace {

//Averages `Factor` horizontally adjacent pixels into one, for each output pixel of a row. Factor is a
//compile-time constant so the inner loop fully unrolls and the compiler can vectorise across x.
template <int Factor, int Shift>
void downsampleRow(const quint32 *in, quint32 *out, int outWidth)
{
    for (int x = 0; x < outWidth; ++x)
    {
        //Sum two channels at a time in packed 16-bit lanes: bytes 0+2 and bytes 1+3
        quint32 lanes02 = 0;
        quint32 lanes13 = 0;
        for (int k = 0; k < Factor; ++k)
        {
            quint32 pixel = in[x * Factor + k];
            lanes02 += pixel & 0x00ff00ff;
            lanes13 += (pixel >> 8) & 0x00ff00ff;
        }
        out[x] = ((lanes02 >> Shift) & 0x00ff00ff) | (((lanes13 >> Shift) & 0x00ff00ff) << 8) | 0xff000000;
    }
}

}

//Shrinks a 32bpp frame by a power-of-two factor while copying it - see header
QImage ImageUtils::downsample32(const uchar *source, int width, int height, qsizetype stride, QImage::Format format, int factor)
{
    Q_ASSERT(factor > 0 && (factor & (factor - 1)) == 0);
    Q_ASSERT(factor <= 256); //Each 16-bit lane accumulates up to factor * 255

    int outWidth = width / factor;
    int outHeight = height / factor;
    QImage output(outWidth, outHeight, format);
    if (output.isNull())
        return output;

    auto rowFunction = &downsampleRow<1, 0>;
    switch (factor)
    {
    case 1: rowFunction = &downsampleRow<1, 0>; break;
    case 2: rowFunction = &downsampleRow<2, 1>; break;
    case 4: rowFunction = &downsampleRow<4, 2>; break;
    case 8: rowFunction = &downsampleRow<8, 3>; break;
    case 16: rowFunction = &downsampleRow<16, 4>; break;
    case 32: rowFunction = &downsampleRow<32, 5>; break;
    case 64: rowFunction = &downsampleRow<64, 6>; break;
    case 128: rowFunction = &downsampleRow<128, 7>; break;
    default: rowFunction = &downsampleRow<256, 8>; break;
    }

    for (int y = 0; y < outHeight; ++y)
    {
        //Sample the middle row of each block of `factor` rows
        const quint32 *in = reinterpret_cast<const quint32 *>(source + (static_cast<qsizetype>(y) * factor + factor / 2) * stride);
        rowFunction(in, reinterpret_cast<quint32 *>(output.scanLine(y)), outWidth);
    }

    return output;
}
