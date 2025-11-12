extern "C" HRESULT CacheEnsureWorkingFolder(
    __in_z LPCWSTR wzBundleId,
    __deref_out_z_opt LPWSTR* psczWorkingFolder
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczWorkingFolder = NULL;
    BOOL fElevatedWorkingFolder = FALSE;
    PSECURITY_DESCRIPTOR psd = NULL;
    LPSECURITY_ATTRIBUTES pWorkingFolderAcl = NULL;

    hr = CalculateWorkingFolder(wzBundleId, &sczWorkingFolder, &fElevatedWorkingFolder);
    ExitOnFailure(hr, "Failed to calculate working folder to ensure it exists.");

    // If elevated, allocate the pWorkingFolderAcl to protect the working folder to only Admins and SYSTEM.
    if (fElevatedWorkingFolder)
    {
        LPCWSTR wzSddl = L"D:PAI(A;;FA;;;BA)(A;OICIIO;GA;;;BA)(A;;FA;;;SY)(A;OICIIO;GA;;;SY)";
        if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(wzSddl, SDDL_REVISION_1, &psd, NULL))
        {
            ExitWithLastError(hr, "Failed to create the security descriptor for the working folder.");
        }

        pWorkingFolderAcl = reinterpret_cast<LPSECURITY_ATTRIBUTES>(MemAlloc(sizeof(SECURITY_ATTRIBUTES), TRUE));
        pWorkingFolderAcl->nLength = sizeof(SECURITY_ATTRIBUTES);
        pWorkingFolderAcl->lpSecurityDescriptor = psd;
        pWorkingFolderAcl->bInheritHandle = FALSE;
    }

    hr = DirEnsureExists(sczWorkingFolder, pWorkingFolderAcl);
    ExitOnFailure(hr, "Failed create working folder.");

    // Best effort to ensure our working folder is not encrypted.
    ::DecryptFileW(sczWorkingFolder, 0);

    if (psczWorkingFolder)
    {
        hr = StrAllocString(psczWorkingFolder, sczWorkingFolder, 0);
        ExitOnFailure(hr, "Failed to copy working folder.");
    }

LExit:
    ReleaseMem(pWorkingFolderAcl);
    if (psd)
    {
        ::LocalFree(psd);
    }
    ReleaseStr(sczWorkingFolder);

    return hr;
}