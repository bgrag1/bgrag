bool ExtractToTempDirectory(__in MSIHANDLE hSession, __in HMODULE hModule,
        __out_ecount_z(cchTempDirBuf) wchar_t* szTempDir, DWORD cchTempDirBuf)
{
        wchar_t szModule[MAX_PATH];
        DWORD cchCopied = GetModuleFileName(hModule, szModule, MAX_PATH - 1);
        if (cchCopied == 0)
        {
                Log(hSession, L"Failed to get module path. Error code %d.", GetLastError());
                return false;
        }
        else if (cchCopied == MAX_PATH - 1)
        {
                Log(hSession, L"Failed to get module path -- path is too long.");
                return false;
        }

        if (szTempDir == NULL || cchTempDirBuf < wcslen(szModule) + 1)
        {
                Log(hSession, L"Temp directory buffer is NULL or too small.");
                return false;
        }
        StringCchCopy(szTempDir, cchTempDirBuf, szModule);
        StringCchCat(szTempDir, cchTempDirBuf, L"-");

        DWORD cchTempDir = (DWORD) wcslen(szTempDir);
        for (int i = 0; DirectoryExists(szTempDir); i++)
        {
                swprintf_s(szTempDir + cchTempDir, cchTempDirBuf - cchTempDir, L"%d", i);
        }

        if (!CreateDirectory(szTempDir, NULL))
        {
                cchCopied = GetTempPath(cchTempDirBuf, szTempDir);
                if (cchCopied == 0 || cchCopied >= cchTempDirBuf)
                {
                        Log(hSession, L"Failed to get temp directory. Error code %d", GetLastError());
                        return false;
                }

                wchar_t* szModuleName = wcsrchr(szModule, L'\\');
                if (szModuleName == NULL) szModuleName = szModule;
                else szModuleName = szModuleName + 1;
                StringCchCat(szTempDir, cchTempDirBuf, szModuleName);
                StringCchCat(szTempDir, cchTempDirBuf, L"-");

                cchTempDir = (DWORD) wcslen(szTempDir);
                for (int i = 0; DirectoryExists(szTempDir); i++)
                {
                        swprintf_s(szTempDir + cchTempDir, cchTempDirBuf - cchTempDir, L"%d", i);
                }

                if (!CreateDirectory(szTempDir, NULL))
                {
                        Log(hSession, L"Failed to create temp directory. Error code %d", GetLastError());
                        return false;
                }
        }

        Log(hSession, L"Extracting custom action to temporary directory: %s\\", szTempDir);
        int err = ExtractCabinet(szModule, szTempDir);
        if (err != 0)
        {
                Log(hSession, L"Failed to extract to temporary directory. Cabinet error code %d.", err);
                DeleteDirectory(szTempDir);
                return false;
        }
        return true;
}