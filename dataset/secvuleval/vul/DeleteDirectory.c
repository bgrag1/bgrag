bool DeleteDirectory(const wchar_t* szDir)
{
        size_t cchDir = wcslen(szDir);
        size_t cchPathBuf = cchDir + 3 + MAX_PATH;
        wchar_t* szPath = (wchar_t*) _alloca(cchPathBuf * sizeof(wchar_t));
        if (szPath == NULL) return false;
        StringCchCopy(szPath, cchPathBuf, szDir);
        StringCchCat(szPath, cchPathBuf, L"\\*");
        WIN32_FIND_DATA fd;
        HANDLE hSearch = FindFirstFile(szPath, &fd);
        while (hSearch != INVALID_HANDLE_VALUE)
        {
                StringCchCopy(szPath + cchDir + 1, cchPathBuf - (cchDir + 1), fd.cFileName);
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                {
                        if (wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0)
                        {
                                DeleteDirectory(szPath);
                        }
                }
                else
                {
                        DeleteFile(szPath);
                }
                if (!FindNextFile(hSearch, &fd))
                {
                        FindClose(hSearch);
                        hSearch = INVALID_HANDLE_VALUE;
                }
        }
        return RemoveDirectory(szDir) != 0;
}