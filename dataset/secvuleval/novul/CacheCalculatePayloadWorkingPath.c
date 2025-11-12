extern "C" HRESULT CacheCalculatePayloadWorkingPath(
    __in_z LPCWSTR wzBundleId,
    __in BURN_PAYLOAD* pPayload,
    __deref_out_z LPWSTR* psczWorkingPath
    )
{
    HRESULT hr = S_OK;

    hr = CalculateWorkingFolder(wzBundleId, psczWorkingPath, NULL);
    ExitOnFailure(hr, "Failed to get working folder for payload.");

    hr = StrAllocConcat(psczWorkingPath, pPayload->sczKey, 0);
    ExitOnFailure(hr, "Failed to append SHA1 hash as payload unverified path.");

LExit:
    return hr;
}