#pragma once
#include <QImage>

class ImageUtils
{
public:
    //Shrinks a 32bpp frame by an integer power-of-two factor in each dimension while copying it out of
    //a (possibly foreign, e.g. PipeWire) buffer. Each output pixel is the average of `factor` adjacent
    //pixels from a single source row, and only every `factor`-th source row is read - so memory traffic
    //drops to 1/factor of a full copy. Fine for ambilight, where everything is averaged into large
    //regions afterwards anyway. Channel order is preserved (works for any 8-bit-per-channel 32bpp
    //format); the 4th byte is forced to 0xff so the result is valid as Format_RGB32/RGBX8888.
    static QImage downsample32(const uchar *source, int width, int height, qsizetype stride, QImage::Format format, int factor);
};
