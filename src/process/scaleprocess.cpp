#include "scaleprocess.h"

//Constructor
ScaleProcess::ScaleProcess(ImageProcess::ProcessType type, ImageSize outputSize) : pType(type)
{
    Q_ASSERT(outputSize.isSet());
    Q_ASSERT(type == ProcessType::LinearScale || type == ProcessType::CubicScale || type == ProcessType::SampleScale); //Should ensure it's actually a scaler here
    pOutputSize = outputSize;
    pScalePercentage = 0.0f;

    //Override to be linear scale if the type is incorrect
    if (pType != ProcessType::LinearScale && pType != ProcessType::CubicScale && pType != ProcessType::SampleScale)
        pType = ProcessType::LinearScale;

#ifdef Q_OS_WIN
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

//Constructor
ScaleProcess::ScaleProcess(ImageProcess::ProcessType type, float scalePercentage) : pType(type)
{
    Q_ASSERT(scalePercentage > 0.0f);
    Q_ASSERT(type == ProcessType::LinearScale || type == ProcessType::CubicScale || type == ProcessType::SampleScale); //Should ensure it's actually a scaler here
    pScalePercentage = scalePercentage;
    pOutputSize = ImageSize();

    //Override to be linear scale if the type is incorrect
    if (pType != ProcessType::LinearScale && pType != ProcessType::CubicScale && pType != ProcessType::SampleScale)
        pType = ProcessType::LinearScale;

#ifdef Q_OS_WIN
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
#ifdef Q_OS_WIN
    if (pWICFactory)
        pWICFactory->Release();
    if (pWICScaler)
        pWICScaler->Release();
#endif
}

//Scales image using the selected scaler type
#ifdef Q_OS_WIN
//For Windows, we'll use the WIC library to do the scaling
QImage ScaleProcess::process(QImage inputImage)
{
    //Dynamically set ImageSize if percentage scaling is enabled
    if (pScalePercentage > 0.0f)
    {
        pOutputSize.height = qCeil(static_cast<float>(inputImage.height()) * pScalePercentage);
        pOutputSize.width = qCeil(static_cast<float>(inputImage.width()) * pScalePercentage);
    }

    //Create bitmap from screen data
    IWICBitmap *WICBitmap = nullptr;
    HRESULT hr = pWICFactory->CreateBitmapFromMemory(inputImage.width(), inputImage.height(), GUID_WICPixelFormat32bppBGRA, inputImage.bytesPerLine(), inputImage.sizeInBytes(), inputImage.bits(), &WICBitmap);  //TODO: bits() does deep-copy
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
    hr = pWICScaler->CopyPixels(NULL, outStride, outSize, outImage.bits());  //TODO: bits() does deep-copy
    Q_ASSERT(SUCCEEDED(hr));

    //Relase
    WICBitmap->Release();

    return outImage;
}
#else
//For everything else, use QImage scalers
QImage ScaleProcess::process(QImage inputImage)
{
    //Dynamically set ImageSize if percentage scaling is enabled
    if (pScalePercentage > 0.0f)
    {
        pOutputSize.height = qCeil(static_cast<float>(inputImage.height()) * pScalePercentage);
        pOutputSize.width = qCeil(static_cast<float>(inputImage.width()) * pScalePercentage);
    }

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
