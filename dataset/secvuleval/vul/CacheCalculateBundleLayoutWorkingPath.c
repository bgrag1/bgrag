extern "C" HRESULT CacheCalculateBundleLayoutWorkingPath(
    __in_z LPCWSTR wzBundleId,
    __deref_out_z LPWSTR* psczWorkingPath
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczWorkingFolder = NULL;

    hr = CalculateWorkingFolder(wzBundleId, psczWorkingPath);
    ExitOnFailure(hr, "Failed to get working folder for bundle layout.");

    hr = StrAllocConcat(psczWorkingPath, wzBundleId, 0);
    ExitOnFailure(hr, "Failed to append bundle id for bundle layout working path.");

LExit:
    ReleaseStr(sczWorkingFolder);

    return hr;
}