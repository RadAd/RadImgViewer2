#ifndef __PROPS_H__
#define __PROPS_H__

#pragma once

#include <atlctrls.h>
#include <atldlgs.h>
#include <atlcrack.h>

#include "Image.h"

#include "resource.h"

class CFileName : public CWindowImpl<CFileName>
{
public:
    DECLARE_WND_CLASS_EX(NULL, 0, COLOR_3DFACE)

    CFileName();
    void Init(HWND hWnd, LPCTSTR lpstrName);

    BEGIN_MSG_MAP(CFileName)
        MSG_WM_PAINT(OnPaint)
        MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseMessage)
    END_MSG_MAP()

private:
    LPCTSTR m_lpstrFilePath;
    CToolTipCtrl m_tooltip;

    void OnPaint(CDCHandle dc);
    LRESULT OnMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

class CPageOne : public CPropertyPageImpl<CPageOne>
{
public:
    enum { IDD = IDD_PROP_PAGE1 };

    CPageOne();
    void Init(LPCTSTR lpstrName);

    BEGIN_MSG_MAP(CPageOne)
        MSG_WM_INITDIALOG(OnInitDialog)
        CHAIN_MSG_MAP(CPropertyPageImpl<CPageOne>)
    END_MSG_MAP()

private:
    LPCTSTR m_lpstrFilePath;
    CFileName m_filename;

    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
};

class CPageTwo : public CPropertyPageImpl<CPageTwo>
{
public:
    enum { IDD = IDD_PROP_PAGE2 };

    CPageTwo();
    void Init(const Image& image);

    BEGIN_MSG_MAP(CPageTwo)
        MSG_WM_INITDIALOG(OnInitDialog)
        CHAIN_MSG_MAP(CPropertyPageImpl<CPageTwo>)
    END_MSG_MAP()

private:
    Image m_image;

    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
};

class CPageThree : public CPropertyPageImpl<CPageThree>
{
public:
    enum { IDD = IDD_PROP_PAGE3 };

    BEGIN_MSG_MAP(CPageThree)
        MSG_WM_INITDIALOG(OnInitDialog)
        CHAIN_MSG_MAP(CPropertyPageImpl<CPageThree>)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
};

class CImageProperties : public CPropertySheetImpl<CImageProperties>
{
public:
    CImageProperties();
    void SetFileInfo(LPCTSTR lpstrFilePath, const Image& image);

    void OnSheetInitialized();

    BEGIN_MSG_MAP(CImageProperties)
        CHAIN_MSG_MAP(CPropertySheetImpl<CImageProperties>)
    END_MSG_MAP()

private:
    CPageOne m_page1;
    CPageTwo m_page2;
    CPageThree m_page3;
};

#endif // __PROPS_H__
