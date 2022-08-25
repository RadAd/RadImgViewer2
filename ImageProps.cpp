#include "stdafx.h"

#include "ImageProps.h"

#include <cmath>

CFileName::CFileName() : m_lpstrFilePath(NULL)
{ }

void CFileName::Init(HWND hWnd, LPCTSTR lpstrName)
{
    const UINT nToolTipID = 1313;

    ATLASSERT(::IsWindow(hWnd));
    SubclassWindow(hWnd);

    // Set tooltip
    m_tooltip.Create(m_hWnd);
    ATLASSERT(m_tooltip.IsWindow());
    RECT rect;
    GetClientRect(&rect);
    CToolInfo ti(0, m_hWnd, nToolTipID, &rect, NULL);
    m_tooltip.AddTool(&ti);

    // set text
    m_lpstrFilePath = lpstrName;
    if (m_lpstrFilePath == NULL)
        return;

    CClientDC dc(m_hWnd);	// will not really paint
    HFONT hFontOld = dc.SelectFont(GetFont());

    RECT rcText = rect;
    dc.DrawText(m_lpstrFilePath, -1, &rcText, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_CALCRECT);
    BOOL bTooLong = (rcText.right > rect.right);
    if (bTooLong)
        m_tooltip.UpdateTipText(m_lpstrFilePath, m_hWnd, nToolTipID);
    m_tooltip.Activate(bTooLong);

    dc.SelectFont(hFontOld);

    Invalidate();
    UpdateWindow();
}

void CFileName::OnPaint(CDCHandle /*dc*/)
{
    CPaintDC dc(m_hWnd);
    if (m_lpstrFilePath != NULL)
    {
        RECT rect;
        GetClientRect(&rect);

        dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
        dc.SetBkMode(TRANSPARENT);
        HFONT hFontOld = dc.SelectFont(GetFont());

        dc.DrawText(m_lpstrFilePath, -1, &rect, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_PATH_ELLIPSIS);

        dc.SelectFont(hFontOld);
    }
}

LRESULT CFileName::OnMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_tooltip.IsWindow())
    {
        MSG msg = { m_hWnd, uMsg, wParam, lParam };
        m_tooltip.RelayEvent(&msg);
    }
    bHandled = FALSE;
    return 1;
}

CPageOne::CPageOne() : m_lpstrFilePath(NULL)
{ }

void CPageOne::Init(LPCTSTR lpstrName)
{
    m_lpstrFilePath = lpstrName;
}

BOOL CPageOne::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    if (m_lpstrFilePath != NULL)	// get and set file properties
    {
        m_filename.Init(GetDlgItem(IDC_FILELOCATION), m_lpstrFilePath);

        WIN32_FIND_DATA fd;
        HANDLE hFind = ::FindFirstFile(m_lpstrFilePath, &fd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            int nSizeK = (int) (fd.nFileSizeLow / 1024);	// assume it not bigger than 2GB
            if (nSizeK == 0 && fd.nFileSizeLow != 0)
                nSizeK = 1;
            TCHAR szBuff[100];
            wsprintf(szBuff, _T("%i KB"), nSizeK);
            SetDlgItemText(IDC_FILESIZE, szBuff);
            SYSTEMTIME st;
            ::FileTimeToSystemTime(&fd.ftCreationTime, &st);
            ::GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, _T("dddd, MMMM dd',' yyyy',' "), szBuff, sizeof(szBuff) / sizeof(szBuff[0]));
            TCHAR szBuff1[50];
            ::GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, _T("hh':'mm':'ss tt"), szBuff1, sizeof(szBuff1) / sizeof(szBuff1[0]));
            lstrcat(szBuff, szBuff1);
            SetDlgItemText(IDC_FILEDATE, szBuff);

            szBuff[0] = _T('\0');
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0)
                lstrcat(szBuff, _T("Archive, "));
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0)
                lstrcat(szBuff, _T("Read-only, "));
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0)
                lstrcat(szBuff, _T("Hidden, "));
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0)
                lstrcat(szBuff, _T("System"));
            int nLen = lstrlen(szBuff);
            if (nLen >= 2 && szBuff[nLen - 2] == _T(','))
                szBuff[nLen - 2] = _T('\0');
            SetDlgItemText(IDC_FILEATTRIB, szBuff);

            ::FindClose(hFind);
        }
    }
    else
    {
        SetDlgItemText(IDC_FILELOCATION, _T("(Clipboard)"));
        SetDlgItemText(IDC_FILESIZE, _T("N/A"));
        SetDlgItemText(IDC_FILEDATE, _T("N/A"));
        SetDlgItemText(IDC_FILEATTRIB, _T("N/A"));
    }

    return TRUE;
}

CPageTwo::CPageTwo()
{ }

void CPageTwo::Init(const Image& image)
{
    m_image = image;
}

BOOL CPageTwo::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    const Image::Size size = m_image.GetFrameSize();
    const Image::Resolution resolution = m_image.GetFrameResolution();
    const WICPixelFormatGUID pixelFormat = m_image.GetFramePixelFormat();

    // get and display bitmap properties
    SetDlgItemText(IDC_TYPE, m_image.GetType().c_str());
    SetDlgItemInt(IDC_WIDTH, size.nWidth, FALSE);
    SetDlgItemInt(IDC_HEIGHT, size.nHeight, FALSE);
    SetDlgItemInt(IDC_HORRES, std::lround(resolution.dWidth));
    SetDlgItemInt(IDC_VERTRES, std::lround(resolution.dHeight));
    SetDlgItemInt(IDC_BITDEPTH, GetBitsPerPixel(pixelFormat));

    return TRUE;
}

BOOL CPageThree::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    // get and set screen properties
    CClientDC dc(NULL);
    SetDlgItemInt(IDC_WIDTH, dc.GetDeviceCaps(HORZRES));
    SetDlgItemInt(IDC_HEIGHT, dc.GetDeviceCaps(VERTRES));
    SetDlgItemInt(IDC_HORRES, dc.GetDeviceCaps(LOGPIXELSX));
    SetDlgItemInt(IDC_VERTRES, dc.GetDeviceCaps(LOGPIXELSY));
    SetDlgItemInt(IDC_BITDEPTH, dc.GetDeviceCaps(BITSPIXEL));

    return TRUE;
}

CImageProperties::CImageProperties()
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;

    AddPage(m_page1);
    AddPage(m_page2);
    AddPage(m_page3);
    SetActivePage(1);
    SetTitle(_T("Bitmap Properties"));
}

void CImageProperties::SetFileInfo(LPCTSTR lpstrFilePath, const Image& image)
{
    m_page1.Init(lpstrFilePath);
    m_page2.Init(image);
}

void CImageProperties::OnSheetInitialized()
{
    CPropertySheetImpl<CImageProperties>::OnSheetInitialized();

    // Special - remove unused buttons, move Close button, center wizard
    CancelToClose();
    RECT rect;
    CButton btnCancel = GetDlgItem(IDCANCEL);
    btnCancel.GetWindowRect(&rect);
    ScreenToClient(&rect);
    btnCancel.ShowWindow(SW_HIDE);
    CButton btnClose = GetDlgItem(IDOK);
    btnClose.SetWindowPos(NULL, &rect, SWP_NOZORDER | SWP_NOSIZE);
    CenterWindow(GetParent());

    ModifyStyleEx(WS_EX_CONTEXTHELP, 0);
}
