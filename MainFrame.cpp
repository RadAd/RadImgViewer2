#include "stdafx.h"

#include "MainFrame.h"

#include <atldlgs.h>

#include <string>
#include <cmath>

#include "ImageProps.h"
#include "RegSettings.h"
#include "Utils.h"
#include "StrUtils.h"

// TODO
// Toggle button for auto-update
// Toggle button for auto-play frames
// Select interpolation type
// Check out L"/grctlext/Disposal" in  https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/Win7Samples/multimedia/wic/wicanimatedgif/WicAnimatedGif.cpp

#define FILE_MENU_POSITION	0
#define RECENT_MENU_POSITION	7
#define POPUP_MENU_POSITION	0

#define TIMER_RELOAD 15
#define TIMER_PLAY 12

BOOL IsBandVisible(CReBarCtrl rebar, int nBand)
{
    REBARBANDINFO rbi;
    rbi.cbSize = sizeof(rbi);
    rbi.fMask = RBBIM_STYLE;
    rebar.GetBandInfo(nBand, &rbi);

    return (rbi.fStyle & RBBS_HIDDEN) ? FALSE : TRUE;
}

int GetBandId(int nID)
{
    switch (nID)
    {
    case ID_VIEW_TOOLBAR: return ATL_IDW_BAND_FIRST + 1;
    case ID_VIEW_ZOOMTOOLBAR: return ATL_IDW_BAND_FIRST + 2;
    case ID_VIEW_FRAMETOOLBAR: return ATL_IDW_BAND_FIRST + 3;
    default: return -1;
    }
}

FILETIME GetFileWriteTime(LPCTSTR lpFilename)
{
    FILETIME ft = {};
    HANDLE hFile = ::CreateFile(lpFilename, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        ::GetFileTime(hFile, nullptr, nullptr, &ft);
        ::CloseHandle(hFile);
    }
    return ft;
}

LPCTSTR g_lpcstrMRURegKey = _T("Software\\RadSoft\\RadImgViewer2");
LPCTSTR g_lpcstrRegMain = _T("Main");
LPCTSTR g_lpcstrRegToolBar = _T("ToolBar");
LPCTSTR g_lpcstrRegStatusBar = _T("StatusBar");
LPCTSTR g_lpcstrRegBackground = _T("Background");

CMainFrame::CMainFrame() : m_bPrintPreview(false), m_bMsgHandled(false), m_FileTime({})
{
    ::SetRect(&m_rcMargin, 1000, 1000, 1000, 1000);
    m_printer.OpenDefaultPrinter();
    m_devmode.CopyFromPrinter(m_printer);
    m_szFilePath[0] = _T('\0');
}

void CMainFrame::UpdateLayout(BOOL bResizeBars)
{
    CFrameWindowImpl<CMainFrame>::UpdateLayout(bResizeBars);
    UpdateStatusBarPositions();
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
    if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
        return TRUE;

    return m_view.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
    BOOL bEnable = m_view.GetImage().IsLoaded();
    UIEnable(ID_FILE_SAVE_AS, bEnable);
    UIEnable(ID_FILE_PRINT, bEnable);
    UIEnable(ID_FILE_PRINT_PREVIEW, bEnable);
    UISetCheck(ID_FILE_PRINT_PREVIEW, m_bPrintPreview);
    UIEnable(ID_EDIT_COPY, bEnable);
    UIEnable(ID_EDIT_PASTE, ::IsClipboardFormatAvailable(CF_BITMAP));
    UIEnable(ID_EDIT_CLEAR, bEnable);
    UIEnable(ID_ZOOM_IN, bEnable && !m_bPrintPreview);
    UIEnable(ID_ZOOM_OUT, bEnable && !m_bPrintPreview);
    UIEnable(ID_ZOOM_DEFAULT, bEnable && !m_bPrintPreview);
    UIEnable(ID_ZOOM_TOFIT, bEnable && !m_bPrintPreview);
    UIEnable(ID_FRAME_NEXT, bEnable && (m_view.GetImage().GetFrame() + 1) < m_view.GetImage().GetFrameCount());
    UIEnable(ID_FRAME_PREV, bEnable && m_view.GetImage().GetFrame() > 0);
    UIEnable(ID_VIEW_PROPERTIES, bEnable);
    UIUpdateToolBar();

    CString szText;
    ATLVERIFY(szText.LoadString(ATL_IDS_IDLEMESSAGE));
    CStatusBarCtrl statusBar(m_hWndStatusBar);
    statusBar.SetText(0, szText, SBT_NOBORDERS);

    return FALSE;
}

BOOL CMainFrame::LoadImage(LPCTSTR lpFilename, LPCTSTR lpName)
{
    if (lpName == nullptr)
    {
        const int nFileNamePos = StrRevFind(lpFilename, _T('\\')) + 1;
        lpName = lpFilename + nFileNamePos;
    }

    HCURSOR OldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    CStatusBarCtrl statusBar(m_hWndStatusBar);
    try
    {
        statusBar.SetText(0, _T("Loading..."), SBT_NOBORDERS);

        Image img;
        img.Load(lpFilename);

        statusBar.SetText(0, _T(""), SBT_NOBORDERS);
        SetCursor(OldCursor);

        if (img.IsLoaded())
        {
            m_view.SetBitmap(img);
            m_view.ZoomToFit();
            UpdateTitleBar(lpName);
            lstrcpy(m_szFilePath, lpFilename);
            m_FileTime = GetFileWriteTime(m_szFilePath);
            UpdateStatusBar();

            SetTimer(TIMER_RELOAD, 100);
            UINT uFrameDelay = m_view.GetImage().GetFrameDelay();
            if (uFrameDelay > 0)
                SetTimer(TIMER_PLAY, uFrameDelay);
            else
                KillTimer(TIMER_PLAY);

            return TRUE;
        }
        else
            return FALSE;
    }
    catch (const CAtlException&)
    {
        statusBar.SetText(0, _T(""), SBT_NOBORDERS);
        SetCursor(OldCursor);
        return FALSE;
    }
}

BOOL CMainFrame::SaveImage(LPCTSTR lpFilename, LPCTSTR lpName)
{
    if (lpName == nullptr)
    {
        const int nFileNamePos = StrRevFind(lpFilename, _T('\\')) + 1;
        lpName = lpFilename + nFileNamePos;
    }

    HCURSOR OldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    CStatusBarCtrl statusBar(m_hWndStatusBar);
    try
    {
        statusBar.SetText(0, _T("Saving..."), SBT_NOBORDERS);

        m_view.SaveImage(lpFilename);

        UpdateTitleBar(lpName);
        lstrcpy(m_szFilePath, lpFilename);
        m_FileTime = GetFileWriteTime(m_szFilePath);
        UpdateStatusBar();

        statusBar.SetText(0, _T(""), SBT_NOBORDERS);
        SetCursor(OldCursor);

        return TRUE;
    }
    catch (const CAtlException&)
    {
        statusBar.SetText(0, _T(""), SBT_NOBORDERS);
        SetCursor(OldCursor);
        return FALSE;
    }
}

void CMainFrame::MessageError(LPCTSTR lpstrMessage)
{
    CString strTitle;
    ATLVERIFY(strTitle.LoadString(IDR_MAINFRAME));
    MessageBox(lpstrMessage, strTitle, MB_OK | MB_ICONERROR);
}

void CMainFrame::TogglePrintPreview()
{
    if (m_bPrintPreview)	// close it
    {
        SetTimer(TIMER_RELOAD, 100);
        m_view.SetFrame(m_wndPreview.m_nCurPage);
        UINT uFrameDelay = m_view.GetImage().GetFrameDelay();
        if (uFrameDelay > 0)
            SetTimer(TIMER_PLAY, uFrameDelay);
        else
            KillTimer(TIMER_PLAY);

        ATLASSERT(m_hWndClient == m_wndPreview.m_hWnd);

        m_hWndClient = m_view;
        m_view.ShowWindow(SW_SHOW);
        m_wndPreview.DestroyWindow();
    }
    else			// display it
    {
        KillTimer(TIMER_RELOAD);
        KillTimer(TIMER_PLAY);

        ATLASSERT(m_hWndClient == m_view.m_hWnd);

        m_wndPreview.SetPrintPreviewInfo(m_printer, m_devmode.m_pDevMode, this, 0, m_view.GetImage().GetFrameCount() - 1);
        m_wndPreview.SetPage(m_view.GetImage().GetFrame());

        m_wndPreview.Create(m_hWnd, rcDefault, NULL, 0, WS_EX_CLIENTEDGE);
        m_view.ShowWindow(SW_HIDE);
        m_hWndClient = m_wndPreview;
    }

    m_bPrintPreview = !m_bPrintPreview;
    UpdateLayout();
}

void CMainFrame::UpdateTitleBar(LPCTSTR lpstrTitle)
{
    CString strDefault;
    ATLVERIFY(strDefault.LoadString(IDR_MAINFRAME));
    CString strTitle = strDefault;
    if (lpstrTitle != NULL)
    {
        strTitle = lpstrTitle;
        strTitle += _T(" - ");
        strTitle += strDefault;
    }
    SetWindowText(strTitle);
}

void CMainFrame::UpdateStatusBarPositions()
{
    if (m_hWndStatusBar != NULL)
    {
        CRect rect;
        GetWindowRect(&rect);

        CStatusBarCtrl statusBar(m_hWndStatusBar);
        const int partWidths[] = { 0, 100, 100, 100 };
        int parts[ARRAYSIZE(partWidths)] = { };
        LONG r = rect.Width();
        if (statusBar.GetStyle() & SBARS_SIZEGRIP)
            r -= GetSystemMetrics(SM_CXVSCROLL);
        for (int i = ARRAYSIZE(partWidths); i > 0; --i)
        {
            parts[i - 1] = r;
            r -= partWidths[i - 1];
        }
        statusBar.SetParts(ARRAYSIZE(parts), parts);
    }
}

void CMainFrame::UpdateStatusBar()
{
    TCHAR szBuff[100];
    CStatusBarCtrl statusBar(m_hWndStatusBar);
    if (!m_view.GetImage().IsLoaded())
    {
        statusBar.SetText(1, _T(""));
        statusBar.SetText(2, _T(""));
    }
    else
    {
        const UINT frameCount = m_view.GetImage().GetFrameCount();
        const Image::Size size = m_view.GetImage().GetFrameSize();
        const WICPixelFormatGUID pixelFormat = m_view.GetImage().GetFramePixelFormat();
        const UINT nBitsPixel = GetBitsPerPixel(pixelFormat);

        swprintf(szBuff, ARRAYSIZE(szBuff), _T("%d/%d"), (m_bPrintPreview ? m_wndPreview.m_nCurPage : m_view.GetImage().GetFrame()) + 1, frameCount);
        statusBar.SetText(1, szBuff);

        swprintf(szBuff, ARRAYSIZE(szBuff), _T("%d x %d x %d"), size.nWidth, size.nHeight, nBitsPixel);
        statusBar.SetText(2, szBuff);
    }
    swprintf(szBuff, ARRAYSIZE(szBuff), _T("%.0f%%"), m_view.GetZoomScale() * 100.0f);
    statusBar.SetText(3, szBuff);
}

// print job info callbacks

bool CMainFrame::IsValidPage(UINT nPage)
{
    return (nPage >= 0 && nPage < m_view.GetImage().GetFrameCount());
}

bool CMainFrame::PrintPage(UINT nPage, HDC hDC)
{
    ATLASSERT(m_view.GetImage().IsLoaded());

    const float mmscalex = ::GetDeviceCaps(hDC, PHYSICALWIDTH) / (::GetDeviceCaps(hDC, HORZSIZE) * 100.0f);
    const float mmscaley = ::GetDeviceCaps(hDC, PHYSICALHEIGHT) / (::GetDeviceCaps(hDC, VERTSIZE) * 100.0f);

    const RECT rcMargin =
    {
        std::lround(m_rcMargin.left * mmscalex), std::lround(m_rcMargin.top * mmscaley),
        std::lround(m_rcMargin.right * mmscalex), std::lround(m_rcMargin.bottom * mmscaley)
    };

    const CRect rcPage =
    {
        std::max(::GetDeviceCaps(hDC, PHYSICALOFFSETX), (int) rcMargin.left), std::max(::GetDeviceCaps(hDC, PHYSICALOFFSETY), (int) rcMargin.top),
        ::GetDeviceCaps(hDC, PHYSICALWIDTH) - std::max(::GetDeviceCaps(hDC, PHYSICALOFFSETX), (int) rcMargin.right),
        ::GetDeviceCaps(hDC, PHYSICALHEIGHT) - std::max(::GetDeviceCaps(hDC, PHYSICALOFFSETY), (int) rcMargin.bottom)
    };

    m_view.SetFrame(nPage);
    const Image::Size size = m_view.GetImage().GetFrameSize();

    // calc scaling factor, so that bitmap is not too small
    const float nScaleX = (float) rcPage.Width() / size.nWidth;
    const float nScaleY = (float) rcPage.Height() / size.nHeight;
    const float nScale = std::max(std::min(nScaleX, nScaleY), 1.0f);

    // calc margines to center bitmap
    const int xOff = std::max(std::lround((::GetDeviceCaps(hDC, PHYSICALWIDTH) - nScale * size.nWidth) / 2), rcPage.left);
    const int yOff = std::max(std::lround((::GetDeviceCaps(hDC, PHYSICALHEIGHT) - nScale * size.nHeight) / 2), rcPage.top);
    // ensure that preview doesn't go outside of the page
    const int cxBlt = std::min(std::lround(nScale * size.nWidth), rcPage.right - xOff);
    const int cyBlt = std::min(std::lround(nScale * size.nHeight), rcPage.bottom - yOff);

    CDCHandle dc = hDC;
    //dc.Rectangle(&rcPage);
    m_view.GetImage().RenderFrame(dc, xOff, yOff, cxBlt, cyBlt, NULL);

    return true;
}

int CMainFrame::OnCreate(LPCREATESTRUCT /*lpCreateStruct*/)
{
    CWindowSettings wndsettings;
    if (wndsettings.Load(g_lpcstrMRURegKey, g_lpcstrRegMain))
        wndsettings.ApplyTo(*this);

    ModifyStyleEx(0, WS_EX_ACCEPTFILES);
    AddClipboardFormatListener(*this);

    HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
    m_CmdBar.AttachMenu(GetMenu());
    SetMenu(NULL);

    UINT idToolbars[] = { IDR_MAINFRAME, IDR_ZOOM, IDR_FRAME };
    HWND hWndToolBar[ARRAYSIZE(idToolbars)];
    for (int i = 0; i < ARRAYSIZE(idToolbars); ++i)
    {
        hWndToolBar[i] = CreateSimpleToolBarCtrl(m_hWnd, idToolbars[i], FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);
        m_CmdBar.LoadImages(idToolbars[i]);
    }

    CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
    AddSimpleReBarBand(hWndCmdBar);
    for (int i = 0; i < ARRAYSIZE(idToolbars); ++i)
        AddSimpleReBarBand(hWndToolBar[i], NULL, i == 0);

    CreateSimpleStatusBar();

    m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);
    m_view.SetDlgCtrlID(0);
    m_view.ClearBitmap();
    m_view.SetFocus();

    _ASSERT(m_CmdBar.GetMenu().GetSubMenu(FILE_MENU_POSITION).GetSubMenu(RECENT_MENU_POSITION));
    m_mru.SetMenuHandle(m_CmdBar.GetMenu().GetSubMenu(FILE_MENU_POSITION).GetSubMenu(RECENT_MENU_POSITION));
    m_mru.SetMaxEntries(12);
    m_mru.ReadFromRegistry(g_lpcstrMRURegKey);
    UIAddMenu(m_CmdBar.GetMenu());

    for (int i = 0; i < ARRAYSIZE(idToolbars); ++i)
        UIAddToolBar(hWndToolBar[i]);

    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->AddMessageFilter(this);
    pLoop->AddIdleHandler(this);

    CReBarCtrl rebar = m_hWndToolBar;
    CReBarSettings rebarsettings;
    if (rebarsettings.Load(g_lpcstrMRURegKey, g_lpcstrRegToolBar))
        rebarsettings.ApplyTo(rebar);

    DWORD dwBackground = 1;
    CRegKey reg;
    if (reg.Create(HKEY_CURRENT_USER, g_lpcstrMRURegKey) == ERROR_SUCCESS)
    {
        DWORD dwStatusBar = 1;
        if (reg.QueryDWORDValue(g_lpcstrRegStatusBar, dwStatusBar) == ERROR_SUCCESS)
        {
            if (dwStatusBar == 0)
                ::ShowWindow(m_hWndStatusBar, SW_HIDE);
        }
        reg.QueryDWORDValue(g_lpcstrRegBackground, dwBackground);
    }

    for (int nID : { ID_VIEW_TOOLBAR, ID_VIEW_ZOOMTOOLBAR, ID_VIEW_FRAMETOOLBAR })
        UISetCheck(nID, IsBandVisible(rebar, rebar.IdToIndex(GetBandId(nID))));
    UISetCheck(ID_VIEW_STATUS_BAR, ::IsWindowVisible(m_hWndStatusBar));
    OnBackground(0, ID_BACKGROUND_BLACK + dwBackground, *this);
    OnViewFlipRotate(0, ID_VIEW_NORMAL, *this);

    UpdateStatusBarPositions();
    UpdateStatusBar();

    SetTimer(TIMER_RELOAD, 100);

    return 0;
}

void CMainFrame::OnClose()
{
    SetMsgHandled(FALSE);

    CWindowSettings wndsettings;
    wndsettings.GetFrom(*this);
    ATLVERIFY(wndsettings.Save(g_lpcstrMRURegKey, g_lpcstrRegMain));

    CReBarCtrl rebar = m_hWndToolBar;
    CReBarSettings rebarsettings;
    rebarsettings.GetFrom(rebar);
    ATLVERIFY(rebarsettings.Save(g_lpcstrMRURegKey, g_lpcstrRegToolBar));

    CRegKey reg;
    if (reg.Create(HKEY_CURRENT_USER, g_lpcstrMRURegKey) == ERROR_SUCCESS)
    {
        ATLVERIFY(reg.SetDWORDValue(g_lpcstrRegStatusBar, ::IsWindowVisible(m_hWndStatusBar)) == ERROR_SUCCESS);
        const HBRUSH hBackground = m_view.GetBackground();
        DWORD dwBackground = 0;
        if (hBackground == (HBRUSH) GetStockObject(BLACK_BRUSH))
            dwBackground = 0;
        else if (hBackground == (HBRUSH) GetStockObject(WHITE_BRUSH))
            dwBackground = 1;
        else if (hBackground == (HBRUSH) GetStockObject(GRAY_BRUSH))
            dwBackground = 2;
        else // if (hBackground == (HBRUSH) GetStockObject(WHITE_BRUSH)) // TODO
            dwBackground = 3;
        ATLVERIFY(reg.SetDWORDValue(g_lpcstrRegBackground, dwBackground) == ERROR_SUCCESS);
    }
}

void CMainFrame::OnDestroy()
{
    RemoveClipboardFormatListener(*this);
    //SetMsgHandled(FALSE); // Default handler calls PostQuitMessage(1);

    PostQuitMessage(0);
}

void CMainFrame::OnContextMenu(CWindow wnd, CPoint point)
{
    if (wnd.m_hWnd == m_view.m_hWnd)
    {
        CMenu menu;
        menu.LoadMenu(IDR_CONTEXTMENU);
        CMenuHandle menuPopup = menu.GetSubMenu(POPUP_MENU_POSITION);
        m_CmdBar.TrackPopupMenu(menuPopup, TPM_RIGHTBUTTON | TPM_VERTICAL, point.x, point.y);
    }
    else
    {
        SetMsgHandled(FALSE);
    }
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent)
    {
    case TIMER_PLAY:
    {
        if (m_view.GetImage().IsLoaded())
        {
            UINT nFrame = m_view.GetImage().GetFrame();
            ++nFrame;
            if (nFrame >= m_view.GetImage().GetFrameCount())
                nFrame = 0;
            m_view.SetFrame(nFrame);
            UpdateStatusBar();

            UINT uFrameDelay = m_view.GetImage().GetFrameDelay();
            if (uFrameDelay > 0)
                SetTimer(TIMER_PLAY, uFrameDelay);
            else
                KillTimer(TIMER_PLAY);
        }
    }
    break;

    case TIMER_RELOAD:
        if (m_szFilePath[0] != _T('\0'))
        {
            FILETIME ft = GetFileWriteTime(m_szFilePath);
            if (CompareFileTime(&ft, &m_FileTime) != 0)
            {
                UINT nFrame = m_view.GetImage().GetFrame();
                Image image;
                image.Load(m_szFilePath);
                image.SetFrame(nFrame);
                m_view.SetBitmap(image);
                m_FileTime = ft;
                UpdateStatusBar();
            }
        }
        break;
    }
}

void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
    UINT nCount = DragQueryFile(hDropInfo, UINT(-1), nullptr, 0);
    for (UINT i = 0; i < nCount; ++i)
    {
        TCHAR strFileName[MAX_PATH];
        DragQueryFile(hDropInfo, i, strFileName, ARRAYSIZE(strFileName));
        if (LoadImage(strFileName, nullptr))
            break;
    }
    DragFinish(hDropInfo);
}

LRESULT CMainFrame::OnClipboardUpdate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (!m_bPrintPreview && m_szFilePath[0] == _T('\0') && m_view.GetImage().IsLoaded() && ::IsClipboardFormatAvailable(CF_BITMAP))
        OnEditPaste(0, 0, *this);
    return 0;
}

void CMainFrame::OnFileExit(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    PostMessage(WM_CLOSE);
}

void CMainFrame::OnFileOpen(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    const std::wstring filter(GetFilter(WICDecoder));
    CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter.c_str(), m_hWnd);
    TCHAR strInitialDir[MAX_PATH];
    lstrcpy(strInitialDir, m_szFilePath);
    {
        TCHAR* e = wcsrchr(strInitialDir, _T('\\'));
        if (e != nullptr)
            *e = _T('\0');
    }

    dlg.m_ofn.lpstrInitialDir = strInitialDir;
    if (dlg.DoModal() == IDOK)
    {
        if (m_bPrintPreview)
            TogglePrintPreview();

        if (LoadImage(dlg.m_szFileName, dlg.m_szFileTitle))
        {
            m_mru.AddToList(dlg.m_ofn.lpstrFile);
            m_mru.WriteToRegistry(g_lpcstrMRURegKey);
        }
        else
        {
            CString strMsg = _T("Can't load image from:\n");
            strMsg += dlg.m_szFileName;
            MessageError(strMsg);
        }
    }
}

void CMainFrame::OnFileSaveAs(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    const std::wstring filter(GetFilter(WICEncoder));
    CFileDialog dlg(FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_EXTENSIONDIFFERENT, filter.c_str(), m_hWnd);
    TCHAR strInitialDir[MAX_PATH];
    lstrcpy(strInitialDir, m_szFilePath);
    {
        TCHAR* e = wcsrchr(strInitialDir, _T('\\'));
        if (e != nullptr)
            *e = _T('\0');
    }

    dlg.m_ofn.lpstrInitialDir = strInitialDir;
    dlg.m_ofn.lpstrDefExt = _T("");
    if (dlg.DoModal() == IDOK)
    {
        if (!SaveImage(dlg.m_szFileName, dlg.m_szFileTitle))
        {
            CString strMsg = _T("Can't save image to:\n");
            strMsg += dlg.m_szFileName;
            MessageError(strMsg);
        }
    }
}

void CMainFrame::OnFileRecent(UINT /*uNotifyCode*/, int nID, CWindow /*wnd*/)
{
    if (m_bPrintPreview)
        TogglePrintPreview();

    // get file name from the MRU list
    TCHAR szFile[MAX_PATH];
    if (m_mru.GetFromList(nID, szFile, MAX_PATH))
    {
        if (LoadImage(szFile, nullptr))
        {
            m_mru.MoveToTop(nID);
        }
        else
        {
            m_mru.RemoveFromList(nID);
        }
        m_mru.WriteToRegistry(g_lpcstrMRURegKey);
    }
    else
    {
        ::MessageBeep(MB_ICONERROR);
    }
}

void CMainFrame::OnFilePrint(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    KillTimer(TIMER_RELOAD);
    KillTimer(TIMER_PLAY);

    CPrintDialog dlg(FALSE, PD_PAGENUMS | PD_USEDEVMODECOPIES | PD_NOSELECTION);
    dlg.m_pd.hDevMode = m_devmode.CopyToHDEVMODE();
    dlg.m_pd.hDevNames = m_printer.CopyToHDEVNAMES();
    dlg.m_pd.nMinPage = 1;
    dlg.m_pd.nMaxPage = (WORD) m_view.GetImage().GetFrameCount();
    dlg.m_pd.nFromPage = (WORD) m_view.GetImage().GetFrame() + 1;
    dlg.m_pd.nToPage = dlg.m_pd.nFromPage;

    if (dlg.DoModal() == IDOK)
    {
        m_devmode.CopyFromHDEVMODE(dlg.m_pd.hDevMode);
        m_printer.ClosePrinter();
        m_printer.OpenPrinter(dlg.m_pd.hDevNames, m_devmode.m_pDevMode);

        CPrintJob job;
        job.StartPrintJob(false, m_printer, m_devmode.m_pDevMode, this, _T("RadImgViewer2 Document"), dlg.m_pd.nFromPage, dlg.m_pd.nToPage, (dlg.PrintToFile() != FALSE));
    }

    ::GlobalFree(dlg.m_pd.hDevMode);
    ::GlobalFree(dlg.m_pd.hDevNames);

    SetTimer(TIMER_RELOAD, 100);
    UINT uFrameDelay = m_view.GetImage().GetFrameDelay();
    if (uFrameDelay > 0)
        SetTimer(TIMER_PLAY, uFrameDelay);
    else
        KillTimer(TIMER_PLAY);
}

void CMainFrame::OnFilePageSetup(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    CPageSetupDialog dlg;
    dlg.m_psd.hDevMode = m_devmode.CopyToHDEVMODE();
    dlg.m_psd.hDevNames = m_printer.CopyToHDEVNAMES();
    dlg.m_psd.rtMargin = m_rcMargin;

    if (dlg.DoModal() == IDOK)
    {
        if (m_bPrintPreview)
            TogglePrintPreview();

        m_devmode.CopyFromHDEVMODE(dlg.m_psd.hDevMode);
        m_printer.ClosePrinter();
        m_printer.OpenPrinter(dlg.m_psd.hDevNames, m_devmode.m_pDevMode);
        m_rcMargin = dlg.m_psd.rtMargin;
    }

    ::GlobalFree(dlg.m_psd.hDevMode);
    ::GlobalFree(dlg.m_psd.hDevNames);
}

void CMainFrame::OnFilePrintPreview(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    TogglePrintPreview();
}

void CMainFrame::OnEditCopy(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    if (::OpenClipboard(NULL))
    {
        CClientDC hDC(NULL);
        HBITMAP hBitmapCopy = m_view.CreateBitmap(hDC);
        if (hBitmapCopy != NULL)
            ::SetClipboardData(CF_BITMAP, hBitmapCopy);
        else
            MessageError(_T("Can't copy bitmap"));

        ::CloseClipboard();
    }
    else
    {
        MessageError(_T("Can't open clipboard to copy"));
    }
}

void CMainFrame::OnEditPaste(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    if (m_bPrintPreview)
        TogglePrintPreview();

    Image image;

    if (::OpenClipboard(*this))
    {
        HBITMAP hBitmap = (HBITMAP)::GetClipboardData(CF_BITMAP);
        if (hBitmap != NULL)
        {
            HPALETTE hPalette = NULL;
            image.CreateFrom(hBitmap, hPalette);
        }
        ::CloseClipboard();
    }
    else
    {
        MessageError(_T("Can't open clipboard to paste"));
    }

    if (image.IsLoaded())
    {
        m_view.SetBitmap(image);
        UpdateTitleBar(_T("(Clipboard)"));
        m_szFilePath[0] = _T('\0');
        m_FileTime = {};
        UpdateStatusBar();
        KillTimer(TIMER_RELOAD);
        KillTimer(TIMER_PLAY);
    }
    else
    {
        MessageError(_T("Can't paste bitmap"));
    }
}

void CMainFrame::OnEditClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    if (m_bPrintPreview)
        TogglePrintPreview();

    m_view.ClearBitmap();
    m_view.ZoomDefault();
    m_view.ClearBitmap();
    UpdateTitleBar(NULL);
    m_szFilePath[0] = _T('\0');
    m_FileTime = {};
    UpdateStatusBar();
    KillTimer(TIMER_RELOAD);
    KillTimer(TIMER_PLAY);
}

void CMainFrame::OnViewToolBar(UINT /*uNotifyCode*/, int nID, CWindow /*wnd*/)
{
    CReBarCtrl rebar = m_hWndToolBar;
    const int nBandIndex = rebar.IdToIndex(GetBandId(nID));
    const BOOL bNew = !IsBandVisible(rebar, nBandIndex);
    rebar.ShowBand(nBandIndex, bNew);
    UISetCheck(nID, bNew);
    UpdateLayout();
}

void CMainFrame::OnViewStatusBar(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    const BOOL bNew = !::IsWindowVisible(m_hWndStatusBar);
    ::ShowWindow(m_hWndStatusBar, bNew ? SW_SHOWNOACTIVATE : SW_HIDE);
    UISetCheck(ID_VIEW_STATUS_BAR, bNew);
    UpdateLayout();
}

void CMainFrame::OnZoomIn(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    m_view.ZoomIn();
    m_view.NotifyParentZoomChanged();
}

void CMainFrame::OnZoomOut(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    m_view.ZoomOut();
    m_view.NotifyParentZoomChanged();
}

void CMainFrame::OnZoomDefault(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    m_view.ZoomDefault();
    m_view.NotifyParentZoomChanged();
}

void CMainFrame::OnZoomToFit(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    m_view.ZoomToFit();
    m_view.NotifyParentZoomChanged();
}

void CMainFrame::OnFrameNext(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    if (m_bPrintPreview)
    {
        m_wndPreview.NextPage();
        UpdateStatusBar();
    }
    else
    {
        const UINT nFrame = m_view.GetImage().GetFrame();
        if ((nFrame + 1) < m_view.GetImage().GetFrameCount())
        {
            m_view.SetFrame(nFrame + 1);
            UpdateStatusBar();
        }
    }
}

void CMainFrame::OnFramePrev(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    if (m_bPrintPreview)
    {
        m_wndPreview.PrevPage();
        UpdateStatusBar();
    }
    else
    {
        const UINT nFrame = m_view.GetImage().GetFrame();
        if (nFrame > 0)
        {
            m_view.SetFrame(nFrame - 1);
            UpdateStatusBar();
        }
    }
}

void CMainFrame::OnBackground(UINT /*uNotifyCode*/, int nID, CWindow /*wnd*/)
{
    switch (nID)
    {
    case ID_BACKGROUND_BLACK: m_view.SetBackground((HBRUSH) GetStockObject(BLACK_BRUSH)); break;
    case ID_BACKGROUND_WHITE: m_view.SetBackground((HBRUSH) GetStockObject(WHITE_BRUSH)); break;
    case ID_BACKGROUND_GRAY: m_view.SetBackground((HBRUSH) GetStockObject(GRAY_BRUSH)); break;
    case ID_BACKGROUND_CHECKERED:
        {
            CRect r(0, 0, 10, 10);
            const CSize sz = r.Size();
            CClientDC hDC(NULL);
            CBitmapDC hBmpDC(hDC, sz.cx * 2, sz.cy * 2);
            for (int y = 0; y < 2; ++y)
            {
                for (int x = 0; x < 2; ++x)
                {
                    r.MoveToXY(sz.cx * x, sz.cy * y);
                    hBmpDC.FillRect(r, (HBRUSH) GetStockObject(((x + y) % 2) == 0 ? LTGRAY_BRUSH : GRAY_BRUSH));
                }
            }
            m_view.SetBackground(CreatePatternBrush(hBmpDC.m_bmp));
        }
        break;
    }
    UISetRadioMenuItem(nID, ID_BACKGROUND_BLACK, ID_BACKGROUND_CHECKERED);
}

void CMainFrame::OnViewFlipRotate(UINT /*uNotifyCode*/, int nID, CWindow /*wnd*/)
{
    WICBitmapTransformOptions FlipRotate = WICBitmapTransformRotate0;
    switch (nID)
    {
    case ID_VIEW_NORMAL: FlipRotate = WICBitmapTransformRotate0; break;
    case ID_VIEW_ROTATE90: FlipRotate = WICBitmapTransformRotate90; break;
    case ID_VIEW_ROTATE180: FlipRotate = WICBitmapTransformRotate180; break;
    case ID_VIEW_ROTATE270: FlipRotate = WICBitmapTransformRotate270; break;
    case ID_VIEW_FLIPPEDHORIZONTALLY: FlipRotate = WICBitmapTransformFlipHorizontal; break;
    case ID_VIEW_FLIPPEDVERTICALLY: FlipRotate = WICBitmapTransformFlipVertical; break;
    }
    UISetRadioMenuItem(nID, ID_VIEW_NORMAL, ID_VIEW_FLIPPEDVERTICALLY);
    m_view.SetFlipRotate(FlipRotate);
}

void CMainFrame::OnViewProperties(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    CImageProperties prop;
    prop.SetFileInfo(m_szFilePath, m_view.GetImage());
    prop.DoModal();
}

class CAboutDlg : public CSimpleDialog<IDD_ABOUT>
{
private:
    BEGIN_MSG_MAP(CAboutDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
        COMMAND_RANGE_HANDLER(IDC_ABOUT_WEBSITE, IDC_ABOUT_MAIL, OnButton)
    END_MSG_MAP()

    LRESULT OnInitDialog(
        _In_ UINT /*uMsg*/,
        _In_ WPARAM /*wParam*/,
        _In_ LPARAM /*lParam*/,
        _In_ BOOL& /*bHandled*/)
    {
        HICON hAppIcon = LoadIcon(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
        GetDlgItem(IDC_ABOUT_APPICON).SendMessage(STM_SETICON, (WPARAM) hAppIcon);

        TCHAR	FileName[1024];
        GetModuleFileName(_Module.GetModuleInstance(), FileName, 1024);

        DWORD	Dummy = 0;
        DWORD	Size = GetFileVersionInfoSize(FileName, &Dummy);
        void* Info = malloc(Size);

        if (Size > 0 && Info != nullptr)
        {
            // VS_VERSION_INFO   VS_VERSIONINFO  VS_FIXEDFILEINFO

            Dummy = 0;
            GetFileVersionInfo(FileName, Dummy, Size, Info);

            TCHAR*  String = nullptr;
            UINT	Length = 0;
            VerQueryValue(Info, TEXT("\\StringFileInfo\\040904b0\\FileVersion"), (LPVOID*) &String, &Length);
            SetDlgItemText(IDC_ABOUT_VERSION, String);
            VerQueryValue(Info, TEXT("\\StringFileInfo\\040904b0\\ProductName"), (LPVOID*) &String, &Length);
            SetDlgItemText(IDC_ABOUT_PRODUCT, String);
        }
        else
        {
            SetDlgItemText(IDC_ABOUT_VERSION, TEXT("Unknown"));
        }

        free(Info);

        return TRUE;
    }

    LRESULT OnButton(
        _In_ WORD wNotifyCode,
        _In_ WORD wID,
        _In_ HWND /*hWndCtl*/,
        _In_ BOOL& /*bHandled*/)
    {
        if (wNotifyCode == BN_CLICKED)
        {
            TCHAR	Url[1024];
            GetDlgItemText(wID, Url, 1024);
            //WinExec(Url, SW_SHOW);
            ShellExecute(*this, TEXT("open"), Url, NULL, NULL, SW_SHOW);
        }
        return 0;
    }
};

void CMainFrame::OnAppAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
{
    CAboutDlg dlg;
    dlg.DoModal();
}

LRESULT CMainFrame::OnZoomChanged(LPNMHDR /*pnmh*/)
{
    UpdateStatusBar();
    SetMsgHandled(TRUE);
    return 0;
}
