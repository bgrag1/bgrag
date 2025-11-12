extern "C" void CacheCleanup(
    __in BOOL fPerMachine,
    __in_z LPCWSTR wzBundleId
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczFolder = NULL;
    LPWSTR sczFiles = NULL;
    LPWSTR sczDelete = NULL;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW wfd = { };
    DWORD cFileName = 0;

    hr = CacheGetCompletedPath(fPerMachine, UNVERIFIED_CACHE_FOLDER_NAME, &sczFolder);
    if (SUCCEEDED(hr))
    {
        hr = DirEnsureDeleteEx(sczFolder, DIR_DELETE_FILES | DIR_DELETE_RECURSE | DIR_DELETE_SCHEDULE);
    }

    if (!fPerMachine)
    {
        hr = CalculateWorkingFolder(wzBundleId, &sczFolder, NULL);
        if (SUCCEEDED(hr))
        {
            hr = PathConcat(sczFolder, L"*.*", &sczFiles);
            if (SUCCEEDED(hr))
            {
                hFind = ::FindFirstFileW(sczFiles, &wfd);
                if (INVALID_HANDLE_VALUE != hFind)
                {
                    do
                    {
                        // Skip directories.
                        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        {
                            continue;
                        }

                        // For extra safety and to silence OACR.
                        wfd.cFileName[MAX_PATH - 1] = L'\0';

                        // Skip resume files (they end with ".R").
                        cFileName = lstrlenW(wfd.cFileName);
                        if (2 < cFileName && L'.' == wfd.cFileName[cFileName - 2] && (L'R' == wfd.cFileName[cFileName - 1] || L'r' == wfd.cFileName[cFileName - 1]))
                        {
                            continue;
                        }

                        hr = PathConcat(sczFolder, wfd.cFileName, &sczDelete);
                        if (SUCCEEDED(hr))
                        {
                            hr = FileEnsureDelete(sczDelete);
                        }
                    } while (::FindNextFileW(hFind, &wfd));
                }
            }
        }
    }

    if (INVALID_HANDLE_VALUE != hFind)
    {
        ::FindClose(hFind);
    }

    ReleaseStr(sczDelete);
    ReleaseStr(sczFiles);
    ReleaseStr(sczFolder);
}