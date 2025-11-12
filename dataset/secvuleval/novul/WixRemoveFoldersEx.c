extern "C" UINT WINAPI WixRemoveFoldersEx(
    __in MSIHANDLE hInstall
    )
{
    //AssertSz(FALSE, "debug WixRemoveFoldersEx");

    HRESULT hr = S_OK;
    PMSIHANDLE hView;
    PMSIHANDLE hRec;
    LPWSTR sczId = NULL;
    LPWSTR sczComponent = NULL;
    LPWSTR sczProperty = NULL;
    LPWSTR sczPath = NULL;
    LPWSTR sczExpandedPath = NULL;
    int iMode = 0;
    DWORD dwCounter = 0;
    DWORD_PTR cchLen = 0;
    MSIHANDLE hTable = NULL;
    MSIHANDLE hColumns = NULL;

    hr = WcaInitialize(hInstall, "WixRemoveFoldersEx");
    ExitOnFailure(hr, "Failed to initialize WixRemoveFoldersEx.");

    // anything to do?
    if (S_OK != WcaTableExists(L"WixRemoveFolderEx"))
    {
        WcaLog(LOGMSG_STANDARD, "WixRemoveFolderEx table doesn't exist, so there are no folders to remove.");
        ExitFunction();
    }

    // query and loop through all the remove folders exceptions
    hr = WcaOpenExecuteView(vcsRemoveFolderExQuery, &hView);
    ExitOnFailure(hr, "Failed to open view on WixRemoveFolderEx table");

    while (S_OK == (hr = WcaFetchRecord(hView, &hRec)))
    {
        hr = WcaGetRecordString(hRec, rfqId, &sczId);
        ExitOnFailure(hr, "Failed to get remove folder identity.");

        hr = WcaGetRecordString(hRec, rfqComponent, &sczComponent);
        ExitOnFailure(hr, "Failed to get remove folder component.");

        hr = WcaGetRecordString(hRec, rfqProperty, &sczProperty);
        ExitOnFailure(hr, "Failed to get remove folder property.");

        hr = WcaGetRecordInteger(hRec, feqMode, &iMode);
        ExitOnFailure(hr, "Failed to get remove folder mode");

        hr = WcaGetProperty(sczProperty, &sczPath);
        ExitOnFailure2(hr, "Failed to resolve remove folder property: %S for row: %S", sczProperty, sczId);

        // fail early if the property isn't set as you probably don't want your installers trying to delete SystemFolder
        // StringCchLengthW succeeds only if the string is zero characters plus 1 for the terminating null
        hr = ::StringCchLengthW(sczPath, 1, reinterpret_cast<UINT_PTR*>(&cchLen));
        if (SUCCEEDED(hr))
        {
            ExitOnFailure2(hr = E_INVALIDARG, "Missing folder property: %S for row: %S", sczProperty, sczId);
        }

        hr = PathExpand(&sczExpandedPath, sczPath, PATH_EXPAND_ENVIRONMENT);
        ExitOnFailure2(hr, "Failed to expand path: %S for row: %S", sczPath, sczId);

        hr = PathBackslashTerminate(&sczExpandedPath);
        ExitOnFailure1(hr, "Failed to backslash-terminate path: %S", sczExpandedPath);

        WcaLog(LOGMSG_STANDARD, "Recursing path: %S for row: %S.", sczExpandedPath, sczId);
        hr = RecursePath(sczExpandedPath, sczId, sczComponent, sczProperty, iMode, &dwCounter, &hTable, &hColumns);
        ExitOnFailure2(hr, "Failed while navigating path: %S for row: %S", sczPath, sczId);
    }

    // reaching the end of the list is actually a good thing, not an error
    if (E_NOMOREITEMS == hr)
    {
        hr = S_OK;
    }
    ExitOnFailure(hr, "Failure occured while processing WixRemoveFolderEx table");

LExit:
    if (hColumns)
    {
        ::MsiCloseHandle(hColumns);
    }

    if (hTable)
    {
        ::MsiCloseHandle(hTable);
    }

    ReleaseStr(sczExpandedPath);
    ReleaseStr(sczPath);
    ReleaseStr(sczProperty);
    ReleaseStr(sczComponent);
    ReleaseStr(sczId);

    DWORD er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
    return WcaFinalize(er);
}