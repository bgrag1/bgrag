extern "C" HRESULT CacheRemoveWorkingFolder(
    __in_z_opt LPCWSTR wzBundleId
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczWorkingFolder = NULL;

    if (vfInitializedCache)
    {
        hr = CalculateWorkingFolder(wzBundleId, &sczWorkingFolder);
        ExitOnFailure(hr, "Failed to calculate the working folder to remove it.");

        // Try to clean out everything in the working folder.
        hr = DirEnsureDeleteEx(sczWorkingFolder, DIR_DELETE_FILES | DIR_DELETE_RECURSE | DIR_DELETE_SCHEDULE);
        TraceError(hr, "Could not delete bundle engine working folder.");
    }

LExit:
    ReleaseStr(sczWorkingFolder);

    return hr;
}