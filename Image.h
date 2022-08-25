#pragma once

#include <string>
#include <vector>
#include <wincodec.h>

std::wstring GetDecoderFilter();
UINT GetBitsPerPixel(WICPixelFormatGUID pixelFormat);

class Image
{
public:
    struct Size
    {
        UINT nWidth, nHeight;
    };
    struct Resolution
    {
        double dWidth, dHeight;
    };

    void Clear()
    {
        m_pBitmap.clear();
        m_delay.clear();
        m_nFrame = 0;
        m_type.clear();
        m_FlipRotate = WICBitmapTransformRotate0;
    }
    BOOL IsLoaded() const { return !m_pBitmap.empty();  }
    void Load(LPCTSTR lpFilename);
    void CreateFrom(HBITMAP hBitmap, HPALETTE hPalette);
    const std::wstring& GetType() const { return m_type; }
    UINT GetFrameCount() const { return (UINT) m_pBitmap.size(); }
    void SetFrame(UINT nFrame) { ATLASSERT(nFrame < GetFrameCount());  m_nFrame = nFrame; }
    UINT GetFrame() const { return m_nFrame; }
    Size GetFrameSize() const;
    Resolution GetFrameResolution() const;
    WICPixelFormatGUID GetFramePixelFormat() const;
    BOOL FrameHasAlpha() const;
    UINT GetFrameDelay() const { return m_delay[m_nFrame]; }

    void RenderFrame(HDC hDC, UINT x, UINT y, UINT cx, UINT cy) const;
    HBITMAP ConvertToBitmap(HBRUSH hBackground) const;

    WICBitmapTransformOptions m_FlipRotate = WICBitmapTransformRotate0;

private:
    std::vector<CComPtr<IWICBitmapSource>> m_pBitmap;
    std::vector<UINT> m_delay;
    UINT m_nFrame = 0;
    std::wstring m_type;
};
