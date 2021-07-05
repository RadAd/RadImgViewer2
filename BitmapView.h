#ifndef __VIEW_H__
#define __VIEW_H__

#pragma once

#include <atlscrl.h>
#include <atlcrack.h>
#include <atltypes.h>

class CBitmapView : public CZoomScrollWindowImpl<CBitmapView>
{
public:
	friend CZoomScrollImpl<CBitmapView>;
	friend CScrollImpl<CBitmapView>;
	DECLARE_WND_CLASS_EX(NULL, CS_HREDRAW | CS_VREDRAW, -1)

	CBitmapView();

	BOOL PreTranslateMessage(MSG* pMsg);
	void SetBitmap(HBITMAP hBitmap, bool bResetOffset = true);
	const CBitmap& GetBitmap() const { return m_bmp; }

	void ZoomToFit();
private:
	void DoPaint(CDCHandle dc) const;

	bool AdjustScrollOffset(int& x, int& y);
	void InvalidateCursor();

	BEGIN_MSG_MAP(CBitmapView)
		MSG_WM_ERASEBKGND(OnEraseBackground)
		MSG_WM_KEYDOWN(OnKeyDown)
		MSG_WM_KEYUP(OnKeyUp)
		MSG_WM_LBUTTONDOWN(OnLButtonDown)
		MSG_WM_LBUTTONUP(OnLButtonUp)
		MSG_WM_MOUSEMOVE(OnMouseMove)
		MSG_WM_CAPTURECHANGED(OnCaptureChanged)
		MSG_WM_SETCURSOR(OnSetCursor)

		CHAIN_MSG_MAP(CZoomScrollWindowImpl<CBitmapView>);
	END_MSG_MAP()

private:
	CBitmap m_bmp;
	CPoint m_pointDrag;
	bool m_bPanning;

	BOOL OnEraseBackground(CDCHandle dc);
	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	void OnLButtonDown(UINT nFlags, CPoint point);
	void OnLButtonUp(UINT nFlags, CPoint point);
	void OnMouseMove(UINT nFlags, CPoint point);
	void OnCaptureChanged(CWindow wnd);
	BOOL OnSetCursor(CWindow wnd, UINT nHitTest, UINT message);
};

#endif // __VIEW_H__
