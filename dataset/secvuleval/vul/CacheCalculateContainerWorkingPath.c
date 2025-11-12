extern "C" HRESULT CacheCalculateContainerWorkingPath(
    __in_z LPCWSTR wzBundleId,
    __in BURN_CONTAINER* pContainer,
    __deref_out_z LPWSTR* psczWorkingPath
    )
{
    HRESULT hr = S_OK;

    hr = CalculateWorkingFolder(wzBundleId, psczWorkingPath);
    ExitOnFailure(hr, "Failed to get working folder for container.");

    hr = StrAllocConcat(psczWorkingPath, pContainer->sczHash, 0);
    ExitOnFailure(hr, "Failed to append SHA1 hash as container unverified path.");

LExit:
    return hr;
}