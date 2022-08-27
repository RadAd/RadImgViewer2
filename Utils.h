#pragma once

inline int StrRevFind(LPCTSTR lpStr, TCHAR c)
{
    for (int i = lstrlen(lpStr) - 1; i >= 0; i--)
    {
        if (lpStr[i] == c)
        {
            return i;
        }
    }
    return -1;
}

inline BOOL AlphaBlend(HDC hDC, int x, int y, int cx, int cy, HBITMAP hBitmap, BLENDFUNCTION bf)
{
    CDC hMemDC;
    hMemDC.CreateCompatibleDC(hDC);
    HBITMAP hOrgBMP = hMemDC.SelectBitmap(hBitmap);
    BOOL ret = ::AlphaBlend(hDC, x, y, cx, cy, hMemDC, 0, 0, cx, cy, bf);
    hMemDC.SelectBitmap(hOrgBMP);
    return ret;
}

class CBitmapDC : public CDC
{
public:
    // Data members
    CBitmap m_bmp;
    HBITMAP m_hBmpOld;

    // Constructor/destructor
    CBitmapDC(HDC hDC, int w, int h) : m_hBmpOld(NULL)
    {
        CreateCompatibleDC(hDC);
        ATLASSERT(m_hDC != NULL);
        m_bmp.CreateCompatibleBitmap(hDC, w, h);
        ATLASSERT(m_bmp.m_hBitmap != NULL);
        m_hBmpOld = SelectBitmap(m_bmp);
    }

    ~CBitmapDC()
    {
        SelectBitmap(m_hBmpOld);
    }
};
