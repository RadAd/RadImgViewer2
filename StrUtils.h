#pragma once
#include <atlstr.h>
#include <atlcoll.h>

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

inline void SplitString(const CString& strInputString, const CString& strDelimiter, CAtlArray<CString>& arrStringArray)
{
    const int sizeS2 = strDelimiter.GetLength();
    const int isize = strInputString.GetLength();

    int newPos = strInputString.Find(strDelimiter, 0);

    //if (newPos < 0)
        //return;

    CAtlArray<INT> positions;
    int iPos = 0;
    while (newPos > iPos)
    {
        positions.Add(newPos);
        iPos = newPos;
        newPos = strInputString.Find(strDelimiter, iPos + sizeS2);
    }

    for (int i = 0; i <= positions.GetCount(); i++)
    {
        CString s;
        if (i == 0)
        {
            if (i == positions.GetCount())
                s = strInputString;
            else
                s = strInputString.Mid(i, positions[i]);
        }
        else
        {
            int offset = positions[i - 1] + sizeS2;
            if (offset < isize)
            {
                if (i == positions.GetCount())
                    s = strInputString.Mid(offset);
                else
                    s = strInputString.Mid(positions[i - 1] + sizeS2, positions[i] - positions[i - 1] - sizeS2);
            }
        }
        arrStringArray.Add(s);
    }
}

inline size_t Find(const CAtlArray<CString>& arrStringArray, const CString& strFind)
{
    for (size_t i = 0; i < arrStringArray.GetCount(); ++i)
    {
        if (arrStringArray[i] == strFind)
            return i;
    }
    return SIZE_T_MAX;
}
