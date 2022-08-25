#ifndef __MAINFRM_H__
#define __MAINFRM_H__

#pragma once

#include <atlcrack.h>
#include <atlctrls.h>
#include <atlctrlw.h>
#include <atlframe.h>
#include <atlmisc.h>
#include <atlprint.h>

#include "BitmapView.h"
#include "Image.h"

#include "resource.h"

class CMainFrame : public CFrameWindowImpl<CMainFrame>, public CUpdateUI<CMainFrame>,
    public CMessageFilter, public CIdleHandler, public CPrintJobInfo
{
public:
    friend CFrameWindowImpl<CMainFrame>;

    DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

    CMainFrame();

    BOOL LoadImage(LPCTSTR lpFilename, LPCTSTR lpName);

private:
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;
    virtual BOOL OnIdle() override;

    void MessageError(LPCTSTR lpstrMessage);
    void TogglePrintPreview();
    void UpdateTitleBar(LPCTSTR lpstrTitle);
    void UpdateLayout(BOOL bResizeBars = TRUE);
    void UpdateStatusBarPositions();
    void UpdateStatusBar();

    // print job info callbacks
    virtual bool IsValidPage(UINT nPage) override;
    virtual bool PrintPage(UINT nPage, HDC hDC) override;

    BEGIN_MSG_MAP_EX(CMainFrame)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_CLOSE(OnClose)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_CONTEXTMENU(OnContextMenu)
        MSG_WM_TIMER(OnTimer)
        MSG_WM_DROPFILES(OnDropFiles)

        MESSAGE_HANDLER(WM_CLIPBOARDUPDATE, OnClipboardUpdate)

        COMMAND_ID_HANDLER_EX(ID_FILE_OPEN, OnFileOpen)
        COMMAND_RANGE_HANDLER_EX(ID_FILE_MRU_FIRST, ID_FILE_MRU_LAST, OnFileRecent)
        COMMAND_ID_HANDLER_EX(ID_FILE_PRINT, OnFilePrint);
        COMMAND_ID_HANDLER_EX(ID_FILE_PAGE_SETUP, OnFilePageSetup)
        COMMAND_ID_HANDLER_EX(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview);
        COMMAND_ID_HANDLER_EX(ID_APP_EXIT, OnFileExit)
        COMMAND_ID_HANDLER_EX(ID_EDIT_COPY, OnEditCopy)
        COMMAND_ID_HANDLER_EX(ID_EDIT_PASTE, OnEditPaste)
        COMMAND_ID_HANDLER_EX(ID_EDIT_CLEAR, OnEditClear)
        COMMAND_ID_HANDLER_EX(ID_VIEW_TOOLBAR, OnViewToolBar)
        COMMAND_ID_HANDLER_EX(ID_VIEW_ZOOMTOOLBAR, OnViewToolBar)
        COMMAND_ID_HANDLER_EX(ID_VIEW_FRAMETOOLBAR, OnViewToolBar)
        COMMAND_ID_HANDLER_EX(ID_VIEW_STATUS_BAR, OnViewStatusBar)
        COMMAND_ID_HANDLER_EX(ID_ZOOM_IN, OnZoomIn)
        COMMAND_ID_HANDLER_EX(ID_ZOOM_OUT, OnZoomOut)
        COMMAND_ID_HANDLER_EX(ID_ZOOM_DEFAULT, OnZoomDefault)
        COMMAND_ID_HANDLER_EX(ID_ZOOM_TOFIT, OnZoomToFit)
        COMMAND_ID_HANDLER_EX(ID_VIEW_PROPERTIES, OnViewProperties)
        COMMAND_ID_HANDLER_EX(ID_FRAME_NEXT, OnFrameNext)
        COMMAND_ID_HANDLER_EX(ID_FRAME_PREV, OnFramePrev)
        COMMAND_RANGE_HANDLER_EX(ID_BACKGROUND_BLACK, ID_BACKGROUND_CHECKERED, OnBackground)
        COMMAND_RANGE_HANDLER_EX(ID_VIEW_NORMAL, ID_VIEW_FLIPPEDVERTICALLY, OnViewFlipRotate)
        COMMAND_ID_HANDLER_EX(ID_APP_ABOUT, OnAppAbout)

        NOTIFY_HANDLER_EX(0, ZSN_ZOOMCHANGED, OnZoomChanged)

        CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
        CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
    END_MSG_MAP()

    BEGIN_UPDATE_UI_MAP(CMainFrame)
        UPDATE_ELEMENT(ID_FILE_PRINT, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_FILE_PRINT_PREVIEW, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_EDIT_COPY, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_EDIT_PASTE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_EDIT_CLEAR, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_ZOOMTOOLBAR, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_FRAMETOOLBAR, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_ZOOM_IN, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_ZOOM_OUT, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_ZOOM_DEFAULT, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_ZOOM_TOFIT, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_FRAME_NEXT, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_FRAME_PREV, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_BACKGROUND_BLACK, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_BACKGROUND_WHITE, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_BACKGROUND_GRAY, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_BACKGROUND_CHECKERED, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_NORMAL, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_ROTATE90, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_ROTATE180, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_ROTATE270, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_FLIPPEDHORIZONTALLY, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_FLIPPEDVERTICALLY, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_PROPERTIES, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
    END_UPDATE_UI_MAP()

private:
    CCommandBarCtrl m_CmdBar;
    CRecentDocumentList m_mru;
    CBitmapView m_view;

    TCHAR m_szFilePath[MAX_PATH];
    FILETIME m_FileTime;
    Image m_image;

    // printing support
    CPrinter m_printer;
    CDevMode m_devmode;
    CPrintPreviewWindow m_wndPreview;
    RECT m_rcMargin;
    bool m_bPrintPreview;

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnClose();
    void OnDestroy();
    void OnContextMenu(CWindow wnd, CPoint point);
    void OnTimer(UINT_PTR nIDEvent);
    void OnDropFiles(HDROP hDropInfo);

    LRESULT OnClipboardUpdate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    void OnFileExit(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnFileOpen(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnFileRecent(UINT /*uNotifyCode*/, int nID, CWindow /*wnd*/);
    void OnFilePrint(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnFilePageSetup(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnFilePrintPreview(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnEditCopy(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnEditPaste(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnEditClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnViewToolBar(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnViewStatusBar(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnZoomIn(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnZoomOut(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnZoomDefault(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnZoomToFit(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnFrameNext(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnFramePrev(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnBackground(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnViewFlipRotate(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnViewProperties(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    void OnAppAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/);
    LRESULT OnZoomChanged(LPNMHDR pnmh);
};

#endif // __MAINFRM_H__
