static HRESULT CalculateWorkingFolder(
    __in_z LPCWSTR /*wzBundleId*/,
    __deref_out_z LPWSTR* psczWorkingFolder,
    __out_opt BOOL* pfWorkingFolderElevated
    )
{
    HRESULT hr = S_OK;
    RPC_STATUS rs = RPC_S_OK;
    BOOL fElevated = FALSE;
    WCHAR wzTempPath[MAX_PATH] = { };
    UUID guid = {};
    WCHAR wzGuid[39];

    if (!vsczWorkingFolder)
    {
        ProcElevated(::GetCurrentProcess(), &fElevated);

        if (fElevated)
        {
            if (!::GetWindowsDirectoryW(wzTempPath, countof(wzTempPath)))
            {
                ExitWithLastError(hr, "Failed to get windows path for working folder.");
            }

            hr = PathFixedBackslashTerminate(wzTempPath, countof(wzTempPath));
            ExitOnFailure(hr, "Failed to ensure windows path for working folder ended in backslash.");

            hr = ::StringCchCatW(wzTempPath, countof(wzTempPath), L"Temp\\");
            ExitOnFailure(hr, "Failed to concat Temp directory on windows path for working folder.");
        }
        else if (0 == ::GetTempPathW(countof(wzTempPath), wzTempPath))
        {
            ExitWithLastError(hr, "Failed to get temp path for working folder.");
        }

        rs = ::UuidCreate(&guid);
        hr = HRESULT_FROM_RPC(rs);
        ExitOnFailure(hr, "Failed to create working folder guid.");

        if (!::StringFromGUID2(guid, wzGuid, countof(wzGuid)))
        {
            hr = E_OUTOFMEMORY;
            ExitOnRootFailure(hr, "Failed to convert working folder guid into string.");
        }

        hr = StrAllocFormatted(&vsczWorkingFolder, L"%ls%ls\\", wzTempPath, wzGuid);
        ExitOnFailure(hr, "Failed to append bundle id on to temp path for working folder.");

        vfWorkingFolderElevated = fElevated;
    }

    hr = StrAllocString(psczWorkingFolder, vsczWorkingFolder, 0);
    ExitOnFailure(hr, "Failed to copy working folder path.");

    if (pfWorkingFolderElevated)
    {
        *pfWorkingFolderElevated = vfWorkingFolderElevated;
    }

LExit:
    return hr;
}