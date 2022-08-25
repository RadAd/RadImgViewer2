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
