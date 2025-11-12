static HTTPAPI_RESULT InitiateWinhttpRequest(HTTP_HANDLE_DATA* handleData, HTTPAPI_REQUEST_TYPE requestType, const char* relativePath, HINTERNET *requestHandle)
{
    HTTPAPI_RESULT result;
    const wchar_t* requestTypeString;
    size_t requiredCharactersForRelativePath;
    wchar_t* relativePathTemp = NULL;
    size_t malloc_size;

    if ((requestTypeString = GetHttpRequestString(requestType)) == NULL)
    {
        result = HTTPAPI_INVALID_ARG;
        LogError("requestTypeString was NULL (result = %" PRI_MU_ENUM ")", MU_ENUM_VALUE(HTTPAPI_RESULT, result));
    }
    else if ((requiredCharactersForRelativePath = MultiByteToWideChar(CP_ACP, 0, relativePath, -1, NULL, 0)) == 0)
    {
        result = HTTPAPI_STRING_PROCESSING_ERROR;
        LogError("MultiByteToWideChar failed, GetLastError=0x%08x", GetLastError());
    }
    else if ((malloc_size = safe_multiply_size_t(safe_add_size_t(requiredCharactersForRelativePath, 1), sizeof(wchar_t))) == SIZE_MAX)
    {
        LogError("malloc invalid size");
        result = HTTPAPI_ALLOC_FAILED;
    }
    else if ((relativePathTemp = (wchar_t*)malloc(malloc_size)) == NULL)
    {
        result = HTTPAPI_ALLOC_FAILED;
        LogError("malloc failed (result = %" PRI_MU_ENUM ")", MU_ENUM_VALUE(HTTPAPI_RESULT, result));
    }
    else if (MultiByteToWideChar(CP_ACP, 0, relativePath, -1, relativePathTemp, (int)requiredCharactersForRelativePath) == 0)
    {
        result = HTTPAPI_STRING_PROCESSING_ERROR;
        LogError("MultiByteToWideChar was 0. (result = %" PRI_MU_ENUM ")", MU_ENUM_VALUE(HTTPAPI_RESULT, result));
    }
    else if ((*requestHandle = WinHttpOpenRequest(
            handleData->ConnectionHandle,
            requestTypeString,
            relativePathTemp,
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE)) == NULL)
    {
        result = HTTPAPI_OPEN_REQUEST_FAILED;
        LogErrorWinHTTPWithGetLastErrorAsString("WinHttpOpenRequest failed (result = %" PRI_MU_ENUM ").", MU_ENUM_VALUE(HTTPAPI_RESULT, result));
    }
    else if ((handleData->x509SchannelHandle != NULL) &&
            !WinHttpSetOption(
                *requestHandle,
                WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
                (void*)x509_schannel_get_certificate_context(handleData->x509SchannelHandle),
                sizeof(CERT_CONTEXT)
    ))
    {
        result = HTTPAPI_SET_X509_FAILURE;
        LogErrorWinHTTPWithGetLastErrorAsString("unable to WinHttpSetOption (WINHTTP_OPTION_CLIENT_CERT_CONTEXT)");
        (void)WinHttpCloseHandle(*requestHandle);
        *requestHandle = NULL;
    }
    else
    {
        result = HTTPAPI_OK;
    }

    free(relativePathTemp);
    return result;
}