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
    pWICBitmap = nullptr;
    pWICScaler = nullptr;
    //Create WIC factory
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
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
    pWICBitmap = nullptr;
    pWICScaler = nullptr;
    //Create WIC factory
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
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
    if (pWICBitmap)
        pWICBitmap->Release();
    if (pWICScaler)
        pWICScaler->Release();
    if (pWICFactory)
        pWICFactory->Release();
#endif
}

//Scales image using the selected scaler type
#ifdef Q_OS_WIN
//For Windows, we'll use the WIC library to do the scaling
QImage ScaleProcess::process(const QImage& inputImage)
{
    //Dynamically set ImageSize if percentage scaling is enabled
    if (pScalePercentage > 0.0f)
    {
        pOutputSize.height = qCeil(static_cast<float>(inputImage.height()) * pScalePercentage);
        pOutputSize.width = qCeil(static_cast<float>(inputImage.width()) * pScalePercentage);
    }

    ImageSize inputSize{inputImage.width(), inputImage.height()};

    //Recreate WIC bitmap only when input dimensions change
    if (!pWICBitmap || inputSize != pWICBitmapSize)
    {
        if (pWICBitmap)
            pWICBitmap->Release();
        HRESULT hr = pWICFactory->CreateBitmapFromMemory(inputImage.width(), inputImage.height(), GUID_WICPixelFormat32bppBGRA, inputImage.bytesPerLine(), inputImage.sizeInBytes(), const_cast<BYTE*>(inputImage.constBits()), &pWICBitmap);
        Q_ASSERT(SUCCEEDED(hr));
        pWICBitmapSize = inputSize;
    }
    else
    {
        //Reuse existing bitmap — update pixel data in-place
        IWICBitmapLock *lock = nullptr;
        WICRect rect{0, 0, inputImage.width(), inputImage.height()};
        HRESULT hr = pWICBitmap->Lock(&rect, WICBitmapLockWrite, &lock);
        Q_ASSERT(SUCCEEDED(hr));
        UINT bufferSize = 0;
        BYTE *data = nullptr;
        lock->GetDataPointer(&bufferSize, &data);
        memcpy(data, inputImage.constBits(), inputImage.sizeInBytes());
        lock->Release();
    }

    //Create scaler if it doesn't exist
    if (!pWICScaler)
    {
        HRESULT hr = pWICFactory->CreateBitmapScaler(&pWICScaler);
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
    HRESULT hr = pWICScaler->Initialize(pWICBitmap, pOutputSize.width, pOutputSize.height, interpolationMode);
    Q_ASSERT(SUCCEEDED(hr));

    //Pre-allocate output image only when output dimensions change
    if (pOutImage.width() != pOutputSize.width || pOutImage.height() != pOutputSize.height || pOutImage.format() != inputImage.format())
        pOutImage = QImage(pOutputSize.width, pOutputSize.height, inputImage.format());

    //Scale directly into the pre-allocated buffer
    quint64 outStride = pOutputSize.width * (inputImage.depth() >> 3); //Depth is in bits, so divide by 8 for bytes
    quint64 outSize = pOutputSize.width * pOutputSize.height * (inputImage.depth() >> 3);
    hr = pWICScaler->CopyPixels(nullptr, outStride, outSize, pOutImage.bits());
    Q_ASSERT(SUCCEEDED(hr));

    return pOutImage;
}
#else
//For everything else, use QImage scalers
QImage ScaleProcess::process(const QImage& inputImage)
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
    case LinearScale:
    case CubicScale:
        return inputImage.scaled(pOutputSize.width, pOutputSize.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    default:
        return inputImage.scaled(pOutputSize.width, pOutputSize.height, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }
}
#endif
