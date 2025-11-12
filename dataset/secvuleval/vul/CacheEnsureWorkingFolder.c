extern "C" HRESULT CacheEnsureWorkingFolder(
    __in_z LPCWSTR wzBundleId,
    __deref_out_z_opt LPWSTR* psczWorkingFolder
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczWorkingFolder = NULL;

    hr = CalculateWorkingFolder(wzBundleId, &sczWorkingFolder);
    ExitOnFailure(hr, "Failed to calculate working folder to ensure it exists.");

    hr = DirEnsureExists(sczWorkingFolder, NULL);
    ExitOnFailure(hr, "Failed create working folder.");

    // Best effort to ensure our working folder is not encrypted.
    ::DecryptFileW(sczWorkingFolder, 0);

    if (psczWorkingFolder)
    {
        hr = StrAllocString(psczWorkingFolder, sczWorkingFolder, 0);
        ExitOnFailure(hr, "Failed to copy working folder.");
    }

LExit:
    ReleaseStr(sczWorkingFolder);

    return hr;
}