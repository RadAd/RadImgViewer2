#include "stdafx.h"

#include "BitmapView.h"

#include <atltypes.h>

#include "resource.h"

#include <cmath>
#include <algorithm>

CBitmapView::CBitmapView()
    : m_bPanning(false)
{
}

BOOL CBitmapView::PreTranslateMessage(MSG* /*pMsg*/)
{
    return FALSE;
}

void CBitmapView::ClearBitmap(bool bResetOffset)
{
    m_image.Clear();
    UpdateScrollSize(bResetOffset);
}

void CBitmapView::SetBitmap(Image image, bool bResetOffset)
{
    m_image = image;
    UpdateScrollSize(bResetOffset);
}

void CBitmapView::SetBackground(HBRUSH hBackground)
{
    m_hBackground = hBackground;
    if (m_image.IsLoaded())
        InvalidateRect(nullptr, FALSE);
}

void CBitmapView::SetFrame(UINT nFrame)
{
    m_image.SetFrame(nFrame);
    const Image::Size framesize = m_image.GetFrameSize();
    SetZoomScale((float) m_sizeAll.cx / framesize.nWidth);
    UpdateScrollSize(FALSE);
}

void CBitmapView::SetFlipRotate(WICBitmapTransformOptions FlipRotate)
{
    m_image.m_FlipRotate = FlipRotate;
    UpdateScrollSize(true);
}

void CBitmapView::ZoomToFit()
{
    CRect r;
    GetClientRect(r);
    const float zx = r.Width() / (float) m_sizeLogAll.cx;
    const float zy = r.Height() / (float) m_sizeLogAll.cy;
    Zoom(std::min(zx, zy));
}

void CBitmapView::InvalidateCursor()
{
    POINT pt; // Screen coordinates!
    GetCursorPos(&pt);
    SetCursorPos(pt.x, pt.y);
}

void CBitmapView::UpdateScrollSize(bool bResetOffset)
{
    SIZE size;
    if (m_image.IsLoaded())
    {
        Image::Size framesize = m_image.GetFrameSize();
        size.cx = framesize.nWidth;
        size.cy = framesize.nHeight;
    }
    else
        size.cx = size.cy = 1;

    //SetScrollOffset(0, 0, FALSE);
    SetScrollSize(size, TRUE, bResetOffset);
    //ZoomDefault();
    //EnableWindow(!m_bmp.IsNull());
}

BOOL CBitmapView::OnEraseBackground(CDCHandle dc)
{
    RECT rect;
    GetClientRect(&rect);
    const int Color = COLOR_APPWORKSPACE;

    const CRect rc(-CPoint(m_ptOffset), m_sizeAll);

    if (rect.top < rc.top)
    {
        RECT rectTop = rect;
        rectTop.bottom = rc.top;
        dc.FillRect(&rectTop, Color);
    }
    if (rect.bottom > rc.bottom)
    {
        RECT rectBottom = rect;
        rectBottom.top = rc.bottom;
        dc.FillRect(&rectBottom, Color);
    }
    if (rect.left < rc.left)
    {
        RECT rectLeft = rect;
        rectLeft.top = rc.top;
        rectLeft.bottom = rc.bottom;
        rectLeft.right = rc.left;
        dc.FillRect(&rectLeft, Color);
    }
    if (rect.right > rc.right)
    {
        RECT rectRight = rect;
        rectRight.top = rc.top;
        rectRight.bottom = rc.bottom;
        rectRight.left = rc.right;
        dc.FillRect(&rectRight, Color);
    }

    //dc.FillRect(&rect, Color);

    return FALSE;
}

void CBitmapView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT /*nFlags*/)
{
    switch (nChar)
    {
    case VK_UP:
        ScrollPageUp();
        break;

    case VK_DOWN:
        ScrollPageDown();
        break;

    case VK_LEFT:
        ScrollPageLeft();
        break;

    case VK_RIGHT:
        ScrollPageRight();
        break;

    case VK_HOME:
        if (GetKeyState(VK_SHIFT) & 0x8000)
            ScrollAllLeft();
        else
            ScrollTop();
        break;

    case VK_END:
        if (GetKeyState(VK_SHIFT) & 0x8000)
            ScrollAllRight();
        else
            ScrollBottom();
        break;

    case VK_CONTROL:
        if (nRepCnt == 1 && !m_bPanning)
        {
            SetZoomMode(GetKeyState(VK_SHIFT) & 0x8000 ? ZOOMMODE_OUT : ZOOMMODE_IN);
            InvalidateCursor();
        }
        break;

    case VK_SHIFT:
        if (nRepCnt == 1 && !m_bPanning && (GetKeyState(VK_CONTROL) & 0x8000))
            SetZoomMode(ZOOMMODE_OUT);
        break;

    default:
        SetMsgHandled(FALSE);
        break;
    }
}

void CBitmapView::OnKeyUp(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
    switch (nChar)
    {
    case VK_CONTROL:
        SetZoomMode(ZOOMMODE_OFF);
        InvalidateCursor();
        break;

    case VK_SHIFT:
        if (!m_bPanning && (GetKeyState(VK_CONTROL) & 0x8000))
            SetZoomMode(ZOOMMODE_IN);
        break;

    default:
        SetMsgHandled(FALSE);
        break;
    }
}

void CBitmapView::DoPaint(CDCHandle dc) const
{
    if (m_image.IsLoaded())
    {
        const int mm = dc.SetMapMode(MM_TEXT);
        CMemoryDC hMemDC(dc, CRect({ 0, 0 }, m_sizeAll));
        m_image.RenderFrame(hMemDC, hMemDC.m_rcPaint.left, hMemDC.m_rcPaint.top, hMemDC.m_rcPaint.right - hMemDC.m_rcPaint.left, hMemDC.m_rcPaint.bottom - hMemDC.m_rcPaint.top, m_hBackground);
        dc.SetMapMode(mm);
    }

    if (m_bTracking)
        const_cast<CBitmapView*>(this)->DrawTrackRect();
}

bool CBitmapView::AdjustScrollOffset(int& x, int& y)
{
    int xOld = x;
    int yOld = y;

    CZoomScrollWindowImpl<CBitmapView>::AdjustScrollOffset(x, y);

    if (m_sizeAll.cx < m_sizeClient.cx)
        x = (m_sizeAll.cx - m_sizeClient.cx) / 2;

    if (m_sizeAll.cy < m_sizeClient.cy)
        y = (m_sizeAll.cy - m_sizeClient.cy) / 2;

    return ((x != xOld) || (y != yOld));
}

void CBitmapView::OnLButtonDown(UINT /*nFlags*/, CPoint point)
{
    if ((GetKeyState(VK_CONTROL) & 0x8000))
    {
        SetMsgHandled(FALSE);
    }
    else
    {
        if (PtInDevRect(point))
        {
            SetCapture();
            m_bPanning = true;
            m_pointDrag = point;
            ::SetCursor(AtlLoadCursor(IDC_PANMOVE));
        }
    }
}

void CBitmapView::OnLButtonUp(UINT /*nFlags*/, CPoint /*point*/)
{
    if (m_bPanning)
    {
        m_bPanning = false;
        ::ReleaseCapture();
    }
    SetMsgHandled(FALSE);
}

void CBitmapView::OnMouseMove(UINT /*nFlags*/, CPoint point)
{
    if (m_bPanning)
    {
        CPoint ptScrollOffset;
        GetScrollOffset(ptScrollOffset);

        ptScrollOffset += m_pointDrag - point;

        SetScrollOffset(ptScrollOffset);

        m_pointDrag = point;
    }
    else
        SetMsgHandled(FALSE);
}

void CBitmapView::OnCaptureChanged(CWindow /*wnd*/)
{
    m_bPanning = false;
    SetMsgHandled(FALSE);
}

BOOL CBitmapView::OnSetCursor(CWindow wnd, UINT nHitTest, UINT /*message*/)
{
    if ((nHitTest == HTCLIENT) && (m_nZoomMode == ZOOMMODE_OFF))
    {
        if (wnd == *this)
        {
            DWORD dwPos = ::GetMessagePos();
            POINT pt = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
            ScreenToClient(&pt);
            if (PtInDevRect(pt))
            {
                ::SetCursor(AtlLoadCursor(IDC_PAN));
                return TRUE;
            }
        }
    }

    SetMsgHandled(FALSE);
    return FALSE;
}
