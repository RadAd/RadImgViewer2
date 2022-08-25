#include "stdafx.h"

#include "Image.h"

#include <atlstr.h>

#define IfFailedThrowHR(expr) { HRESULT hr = (expr); if (FAILED(hr)) throw CAtlException(hr); }

#define DIB_WIDTHBYTES(bits)    ((((bits) + 31)>>5)<<2)

struct FormatConverter
{
    IWICImagingFactory* pFactory;
    REFWICPixelFormatGUID dstFormat;
    WICBitmapDitherType dither = WICBitmapDitherTypeNone;
    IWICPalette* pIPalette = nullptr;
    double alphaThresholdPercent = 0.0f;
    WICBitmapPaletteType paletteTranslate = WICBitmapPaletteTypeCustom;
};

struct FlipRotator
{
    IWICImagingFactory* pFactory;
    WICBitmapTransformOptions options;
};

struct Scaler
{
    IWICImagingFactory* pFactory;
    UINT uiWidth;
    UINT uiHeight;
    WICBitmapInterpolationMode mode;
};

CComPtr<IWICFormatConverter> operator|(IWICBitmapSource* pISource, const FormatConverter& fc)
{
    CComPtr<IWICFormatConverter> pConvertedFrame;
    IfFailedThrowHR(fc.pFactory->CreateFormatConverter(&pConvertedFrame));

    IfFailedThrowHR(pConvertedFrame->Initialize(
        pISource,                          // Source frame to convert
        fc.dstFormat,     // The desired pixel format
        fc.dither,         // The desired dither pattern
        fc.pIPalette,                            // The desired palette
        fc.alphaThresholdPercent,                            // The desired alpha threshold
        fc.paletteTranslate       // Palette translation type
    ));

    return pConvertedFrame;
}

CComPtr<IWICBitmapFlipRotator> operator|(IWICBitmapSource* pISource, const FlipRotator& fr)
{
    CComPtr<IWICBitmapFlipRotator> pFlipRotator;
    IfFailedThrowHR(fr.pFactory->CreateBitmapFlipRotator(&pFlipRotator));

    IfFailedThrowHR(pFlipRotator->Initialize(pISource, fr.options));

    return pFlipRotator;
}

CComPtr<IWICBitmapScaler> operator|(IWICBitmapSource* pISource, const Scaler& s)
{
    CComPtr<IWICBitmapScaler> pScaler;
    IfFailedThrowHR(s.pFactory->CreateBitmapScaler(&pScaler));

    IfFailedThrowHR(pScaler->Initialize(pISource, s.uiWidth, s.uiHeight, s.mode));

    return pScaler;
}

std::wstring GetDecoderFilter()
{
    CComPtr<IWICImagingFactory> pFactory;
    if (FAILED(pFactory.CoCreateInstance(CLSID_WICImagingFactory)))
        throw CAtlException(WINCODEC_ERR_NOTINITIALIZED);

    std::wstring allfilter;
    std::wstring filter;

    CComPtr<IEnumUnknown> pEnum;
    if (SUCCEEDED(pFactory->CreateComponentEnumerator(WICDecoder, WICComponentEnumerateDefault, &pEnum)))
    {
        const UINT cbBuffer = 256; // if not enough will be truncated
        ULONG cbElement = 0;
        CComPtr<IUnknown> pElement = NULL;
        while (S_OK == pEnum->Next(1, &pElement, &cbElement))
        {
            CComQIPtr<IWICBitmapCodecInfo> pCodecInfo = pElement;

            CStringW strFriendlyName;
            CStringW strFileExtensions;

            UINT cbActual = 0;
            // Codec name
            pCodecInfo->GetFriendlyName(cbBuffer, strFriendlyName.GetBufferSetLength(cbBuffer), &cbActual);
            strFriendlyName.ReleaseBufferSetLength(cbActual);
            strFriendlyName.Replace(L"Decoder", L"Files");
            // File extensions
            pCodecInfo->GetFileExtensions(cbBuffer, strFileExtensions.GetBufferSetLength(cbBuffer), &cbActual);
            strFileExtensions.ReleaseBufferSetLength(cbActual);
            strFileExtensions.Replace(L".", L"*.");
            strFileExtensions.Replace(L",", L";");

            filter += strFriendlyName;
            filter += L" (";
            filter += strFileExtensions;
            filter += L")|";
            filter += strFileExtensions;
            filter += L"|";

            if (!allfilter.empty())
                allfilter += L';';
            allfilter += strFileExtensions;

            pElement = NULL;
        }
    }
    filter = L"All Image Files(" + allfilter + L") |" + allfilter + L"|" + filter;
    filter += L"All Files (*.*)|* .*|";
    for (auto& c : filter)
    {
        if (c == L'|')
            c = L'\0';
    }

    return filter;
}

UINT GetBitsPerPixel(WICPixelFormatGUID pixelFormat)
{
    CComPtr<IWICImagingFactory> pFactory;
    IfFailedThrowHR(pFactory.CoCreateInstance(CLSID_WICImagingFactory));

    CComPtr<IWICComponentInfo> pIComponentInfo;
    IfFailedThrowHR(pFactory->CreateComponentInfo(pixelFormat, &pIComponentInfo));

    CComQIPtr<IWICPixelFormatInfo> pIPixelFormatInfo = pIComponentInfo;

    UINT bitsPerPixel;
    IfFailedThrowHR(pIPixelFormatInfo->GetBitsPerPixel(&bitsPerPixel));
    return bitsPerPixel;
}

void Image::Load(LPCTSTR lpFilename)
{
    Clear();

    CComPtr<IWICImagingFactory> pFactory;
    IfFailedThrowHR(pFactory.CoCreateInstance(CLSID_WICImagingFactory));

    CComPtr<IWICBitmapDecoder> pDecoder;
    IfFailedThrowHR(pFactory->CreateDecoderFromFilename(lpFilename, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder));

    CComPtr<IWICBitmapDecoderInfo> pCodecInfo;
    IfFailedThrowHR(pDecoder->GetDecoderInfo(&pCodecInfo));

    CStringW strFriendlyName;

    const UINT cbBuffer = 256; // if not enough will be truncated
    UINT cbActual = 0;
    // Codec name
    pCodecInfo->GetFriendlyName(cbBuffer, strFriendlyName.GetBufferSetLength(cbBuffer), &cbActual);
    strFriendlyName.ReleaseBufferSetLength(cbActual);
    strFriendlyName.Replace(L" Decoder", L"");

    m_type = strFriendlyName.GetString();

    UINT nCount = 0;
    IfFailedThrowHR(pDecoder->GetFrameCount(&nCount));

    for (UINT i = 0; i < nCount; ++i)
    {
        CComPtr<IWICBitmapFrameDecode> pFrame;
        IfFailedThrowHR(pDecoder->GetFrame(i, &pFrame));

        UINT uFrameDelay = 0;

        CComPtr<IWICMetadataQueryReader> pFrameMetadataQueryReader;
        if (SUCCEEDED(pFrame->GetMetadataQueryReader(&pFrameMetadataQueryReader)))
        {
            PROPVARIANT propValue;
            PropVariantInit(&propValue);

            if (SUCCEEDED(pFrameMetadataQueryReader->GetMetadataByName(L"/grctlext/Delay", &propValue)))
            {
                if (propValue.vt == VT_UI2)
                {
                    // Convert the delay retrieved in 10 ms units to a delay in 1 ms units
                    IfFailedThrowHR(UIntMult(propValue.uiVal, 10, &uFrameDelay));
                }
                PropVariantClear(&propValue);
            }
        }

        // Copy so it releases the decoder and therefore releases the file handle
        CComPtr<IWICBitmap> pBitmap;
        IfFailedThrowHR(pFactory->CreateBitmapFromSource(pFrame, WICBitmapCacheOnLoad, &pBitmap));

        m_pBitmap.emplace_back(pBitmap);
        m_delay.push_back(uFrameDelay);
    }
}

void Image::CreateFrom(HBITMAP hBitmap, HPALETTE hPalette)
{
    Clear();

    CComPtr<IWICImagingFactory> pFactory;
    IfFailedThrowHR(pFactory.CoCreateInstance(CLSID_WICImagingFactory));

    CComPtr<IWICBitmap> pBitmap;
    IfFailedThrowHR(pFactory->CreateBitmapFromHBITMAP(hBitmap, hPalette, WICBitmapIgnoreAlpha, &pBitmap));

    m_pBitmap.emplace_back(pBitmap);
}

Image::Size Image::GetFrameSize() const
{
    Size nSize;
    IfFailedThrowHR(m_pBitmap[m_nFrame]->GetSize(&nSize.nWidth, &nSize.nHeight));
    if (m_FlipRotate == WICBitmapTransformRotate90 || m_FlipRotate == WICBitmapTransformRotate270)
    {
        std::swap(nSize.nWidth, nSize.nHeight);
    }
    return nSize;
}

Image::Resolution Image::GetFrameResolution() const
{
    Resolution dResolution;
    IfFailedThrowHR(m_pBitmap[m_nFrame]->GetResolution(&dResolution.dWidth, &dResolution.dHeight));
    return dResolution;
}

WICPixelFormatGUID Image::GetFramePixelFormat() const
{
    WICPixelFormatGUID pixelFormat;
    IfFailedThrowHR(m_pBitmap[m_nFrame]->GetPixelFormat(&pixelFormat));
    return pixelFormat;
}

BOOL Image::FrameHasAlpha() const
{
    WICPixelFormatGUID pixelFormat  = GetFramePixelFormat();
    return (GUID_WICPixelFormat8bppAlpha == pixelFormat)
        || (GUID_WICPixelFormat16bppBGRA5551 == pixelFormat)
        || (GUID_WICPixelFormat32bppBGRA == pixelFormat)
        || (GUID_WICPixelFormat32bppPBGRA == pixelFormat)
        || (GUID_WICPixelFormat64bppRGBA == pixelFormat)
        || (GUID_WICPixelFormat64bppBGRA == pixelFormat)
        || (GUID_WICPixelFormat64bppPRGBA == pixelFormat)
        || (GUID_WICPixelFormat64bppPBGRA == pixelFormat)
        || (GUID_WICPixelFormat64bppRGBAFixedPoint == pixelFormat)
        || (GUID_WICPixelFormat64bppRGBAHalf == pixelFormat)
        || (GUID_WICPixelFormat40bppCMYKAlpha == pixelFormat)
        || (GUID_WICPixelFormat80bppCMYKAlpha == pixelFormat)
        || (GUID_WICPixelFormat128bppRGBAFloat == pixelFormat)
        || (GUID_WICPixelFormat128bppPRGBAFloat == pixelFormat)
        || (GUID_WICPixelFormat32bppRGBA1010102 == pixelFormat)
        || (GUID_WICPixelFormat32bppRGBA1010102XR == pixelFormat);
}

void Image::RenderFrame(HDC hDC, UINT x, UINT y, UINT cx, UINT cy) const
{
    CComPtr<IWICImagingFactory> pFactory;
    IfFailedThrowHR(pFactory.CoCreateInstance(CLSID_WICImagingFactory))

    const BOOL bAlpha = FrameHasAlpha();

    CComPtr<IWICBitmapSource> pBitmap = m_pBitmap[m_nFrame]
        | FormatConverter({ pFactory, bAlpha ? GUID_WICPixelFormat32bppPBGRA : GUID_WICPixelFormat32bppBGR })
        | FlipRotator({ pFactory, m_FlipRotate })
        | Scaler({ pFactory, cx, cy, WICBitmapInterpolationModeNearestNeighbor });

    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen)
        throw CAtlException(1);

    BITMAPINFO bminfo = {};
    bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bminfo.bmiHeader.biWidth = cx;
    bminfo.bmiHeader.biHeight = -(LONG) cy;
    bminfo.bmiHeader.biPlanes = 1;
    bminfo.bmiHeader.biBitCount = 32;
    bminfo.bmiHeader.biCompression = BI_RGB;

    void* pvImageBits = nullptr;	// Freed with DeleteObject(hDIBBitmap)
    CBitmap hDIBBitmap = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0);

    ReleaseDC(NULL, hdcScreen);

    if (!hDIBBitmap)
        throw CAtlException(2);

    const UINT nStride = DIB_WIDTHBYTES(cx * 32);
    const UINT nImage = nStride * cy;
    IfFailedThrowHR(pBitmap->CopyPixels(nullptr, nStride, nImage, reinterpret_cast<BYTE*>(pvImageBits)));

    if (bAlpha)
    {
        CDC hMemDC;
        hMemDC.CreateCompatibleDC(hDC);
        HBITMAP hOrgBMP = hMemDC.SelectBitmap(hDIBBitmap);
        BLENDFUNCTION bf{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
        ::AlphaBlend(hDC, x, y, cx, cy, hMemDC, 0, 0, cx, cy, bf);
        hMemDC.SelectBitmap(hOrgBMP);
    }
    else
        ::SetDIBitsToDevice(hDC, x, y, cx, cy, 0, 0, 0, cy, pvImageBits, &bminfo, DIB_RGB_COLORS);
}

HBITMAP Image::ConvertToBitmap(HBRUSH hBackground) const
{
    const Size nSize = GetFrameSize();

    CClientDC hDC(NULL);
    CDC hMemDC;
    hMemDC.CreateCompatibleDC(hDC);
    CBitmap hMemBmp;
    hMemBmp.CreateCompatibleBitmap(hDC, nSize.nWidth, nSize.nHeight);

    HBITMAP hOrgBMP = hMemDC.SelectBitmap(hMemBmp);
    const RECT r = { 0, 0, (LONG) nSize.nWidth, (LONG) nSize.nHeight };
    hMemDC.SetBrushOrg(POINT({ (LONG) nSize.nWidth / 2, (LONG) nSize.nHeight / 2 }));
    hMemDC.FillRect(&r, hBackground);
    RenderFrame(hMemDC, 0, 0, nSize.nWidth, nSize.nHeight);
    hMemDC.SelectBitmap(hOrgBMP);

    return hMemBmp.Detach();
}
