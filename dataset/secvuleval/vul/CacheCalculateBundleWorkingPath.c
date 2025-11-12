extern "C" HRESULT CacheCalculateBundleWorkingPath(
    __in_z LPCWSTR wzBundleId,
    __in LPCWSTR wzExecutableName,
    __deref_out_z LPWSTR* psczWorkingPath
    )
{
    Assert(vfInitializedCache);

    HRESULT hr = S_OK;
    LPWSTR sczWorkingFolder = NULL;

    // If the bundle is running out of the package cache then we use that as the
    // working folder since we feel safe in the package cache.
    if (vfRunningFromCache)
    {
        hr = PathForCurrentProcess(psczWorkingPath, NULL);
        ExitOnFailure(hr, "Failed to get current process path.");
    }
    else // Otherwise, use the real working folder.
    {
        hr = CalculateWorkingFolder(wzBundleId, &sczWorkingFolder);
        ExitOnFailure(hr, "Failed to get working folder for bundle.");

        hr = StrAllocFormatted(psczWorkingPath, L"%ls%ls\\%ls", sczWorkingFolder, BUNDLE_WORKING_FOLDER_NAME, wzExecutableName);
        ExitOnFailure(hr, "Failed to calculate the bundle working path.");
    }

LExit:
    ReleaseStr(sczWorkingFolder);

    return hr;
}