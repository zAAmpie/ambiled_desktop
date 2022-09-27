#include "scaleprocess.h"

//Constructor
ScaleProcess::ScaleProcess(ImageProcess::ProcessType type, ImageSize outputSize) : pType(type)
{
    Q_ASSERT(outputSize.isSet());
    Q_ASSERT(type == ProcessType::LinearScale || type == ProcessType::CubicScale || type == ProcessType::SampleScale); //Should ensure it's actually a scaler here
    pOutputSize = outputSize;

    //Override to be linear scale if the type is incorrect
    if (pType != ProcessType::LinearScale && pType != ProcessType::CubicScale && pType != ProcessType::SampleScale)
        pType = ProcessType::LinearScale;

#ifdef Q_OS_WINDOWS
    //Create WIC factory
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pWICFactory)
    );
    Q_ASSERT(SUCCEEDED(hr));
#endif
}

//Destructor
ScaleProcess::~ScaleProcess()
{
#ifdef Q_OS_WINDOWS
    if (pWICFactory)
        pWICFactory->Release();
    if (pWICScaler)
        pWICScaler->Release();
#endif
}

//Scales image using the selected scaler type
#ifdef Q_OS_WINDOWS
//For Windows, we'll use the WIC library to do the scaling
QImage ScaleProcess::process(QImage inputImage)
{
    //Create bitmap from screen data
    IWICBitmap *WICBitmap = nullptr;
    HRESULT hr = pWICFactory->CreateBitmapFromMemory(inputImage.width(), inputImage.height(), GUID_WICPixelFormat32bppBGRA, inputImage.bytesPerLine(), inputImage.sizeInBytes(), inputImage.bits(), &WICBitmap);
    Q_ASSERT(SUCCEEDED(hr));

    //Create scaler if it doesn't exist
    if (!pWICScaler)
    {
        hr = pWICFactory->CreateBitmapScaler(&pWICScaler);
        Q_ASSERT(SUCCEEDED(hr));
    }

    //Set interpolation mode
    WICBitmapInterpolationMode interpolationMode;
    switch (pType)
    {
    case SampleScale:
        interpolationMode = WICBitmapInterpolationModeNearestNeighbor;
        break;
    case LinearScale:
        interpolationMode = WICBitmapInterpolationModeLinear;
        break;
    case CubicScale:
        interpolationMode = WICBitmapInterpolationModeCubic;
        break;
    default:
        interpolationMode = WICBitmapInterpolationModeLinear;
    };

    //Initialize scaler
    hr = pWICScaler->Initialize(WICBitmap, pOutputSize.width, pOutputSize.height, interpolationMode);
    Q_ASSERT(SUCCEEDED(hr));

    //Create the same image type, but with different sizes
    quint64 outStride = pOutputSize.width * (inputImage.depth() >> 3); //Depth is in bits, so divide by 8 for bytes
    quint64 outSize = pOutputSize.width * pOutputSize.height * (inputImage.depth() >> 3);
    QImage outImage(pOutputSize.width, pOutputSize.height, inputImage.format());

    //Scale
    hr = pWICScaler->CopyPixels(NULL, outStride, outSize, outImage.bits());
    Q_ASSERT(SUCCEEDED(hr));

    //Relase
    WICBitmap->Release();

    return outImage;
}
#else
//For everything else, use QImage scalers
QImage ScaleProcess::process(QImage inputImage)
{
    //TODO: Actually implement specific scaling methods for non-Windows here
    switch (pType)
    {
    case SampleScale:
        return inputImage.scaled(pOutputSize.width, pOutputSize.height, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    case LinearScale | CubicScale:
        return inputImage.scaled(pOutputSize.width, pOutputSize.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
}
#endif
