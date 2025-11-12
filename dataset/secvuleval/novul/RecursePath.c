static HRESULT RecursePath(
    __in_z LPCWSTR wzPath,
    __in_z LPCWSTR wzId,
    __in_z LPCWSTR wzComponent,
    __in_z LPCWSTR wzProperty,
    __in int iMode,
    __inout DWORD* pdwCounter,
    __inout MSIHANDLE* phTable,
    __inout MSIHANDLE* phColumns
    )
{
    HRESULT hr = S_OK;
    DWORD er;
    LPWSTR sczSearch = NULL;
    LPWSTR sczProperty = NULL;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW wfd;
    LPWSTR sczNext = NULL;

    // Do NOT follow junctions.
    DWORD dwAttributes = ::GetFileAttributesW(wzPath);
    if (dwAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        WcaLog(LOGMSG_STANDARD, "Path is a junction; skipping: %ls", wzPath);
        ExitFunction();
    }

    // First recurse down to all the child directories.
    hr = StrAllocFormatted(&sczSearch, L"%s*", wzPath);
    ExitOnFailure1(hr, "Failed to allocate file search string in path: %S", wzPath);

    hFind = ::FindFirstFileW(sczSearch, &wfd);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        er = ::GetLastError();
        if (ERROR_PATH_NOT_FOUND == er)
        {
            WcaLog(LOGMSG_STANDARD, "Search path not found: %ls", sczSearch);
            ExitFunction1(hr = S_FALSE);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(er);
        }
        ExitOnFailure1(hr, "Failed to find all files in path: %S", wzPath);
    }

    do
    {
        // Skip files and the dot directories.
        if (FILE_ATTRIBUTE_DIRECTORY != (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || L'.' == wfd.cFileName[0] && (L'\0' == wfd.cFileName[1] || (L'.' == wfd.cFileName[1] && L'\0' == wfd.cFileName[2])))
        {
            continue;
        }

        hr = StrAllocFormatted(&sczNext, L"%s%s\\", wzPath, wfd.cFileName);
        ExitOnFailure2(hr, "Failed to concat filename '%S' to string: %S", wfd.cFileName, wzPath);

        hr = RecursePath(sczNext, wzId, wzComponent, wzProperty, iMode, pdwCounter, phTable, phColumns);
        ExitOnFailure1(hr, "Failed to recurse path: %S", sczNext);
    } while (::FindNextFileW(hFind, &wfd));

    er = ::GetLastError();
    if (ERROR_NO_MORE_FILES == er)
    {
        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(er);
        ExitOnFailure1(hr, "Failed while looping through files in directory: %S", wzPath);
    }

    // Finally, set a property that points at our path.
    hr = StrAllocFormatted(&sczProperty, L"_%s_%u", wzProperty, *pdwCounter);
    ExitOnFailure1(hr, "Failed to allocate Property for RemoveFile table with property: %S.", wzProperty);

    ++(*pdwCounter);

    hr = WcaSetProperty(sczProperty, wzPath);
    ExitOnFailure2(hr, "Failed to set Property: %S with path: %S", sczProperty, wzPath);

    // Add the row to remove any files and another row to remove the folder.
    hr = WcaAddTempRecord(phTable, phColumns, L"RemoveFile", NULL, 1, 5, L"RfxFiles", wzComponent, L"*.*", sczProperty, iMode);
    ExitOnFailure2(hr, "Failed to add row to remove all files for WixRemoveFolderEx row: %S under path:", wzId, wzPath);

    hr = WcaAddTempRecord(phTable, phColumns, L"RemoveFile", NULL, 1, 5, L"RfxFolder", wzComponent, NULL, sczProperty, iMode);
    ExitOnFailure2(hr, "Failed to add row to remove folder for WixRemoveFolderEx row: %S under path: %S", wzId, wzPath);

LExit:
    if (INVALID_HANDLE_VALUE != hFind)
    {
        ::FindClose(hFind);
    }

    ReleaseStr(sczNext);
    ReleaseStr(sczProperty);
    ReleaseStr(sczSearch);
    return hr;
}