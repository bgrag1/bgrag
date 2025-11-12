static HTTPAPI_RESULT ReceiveResponseHeaders(HINTERNET requestHandle, HTTP_HEADERS_HANDLE responseHeadersHandle)
{
    HTTPAPI_RESULT result;
    wchar_t* responseHeadersTemp = NULL;
    DWORD responseHeadersTempLength = 0;

    // Don't explicictly check return code here - since this should fail (to determine length)
    // and checking return code means an explicit GetLastError() comparison.  Instead
    // rely on subsequent WinHttpQueryHeaders() to fail.
    WinHttpQueryHeaders(
        requestHandle,
        WINHTTP_QUERY_RAW_HEADERS_CRLF,
        WINHTTP_HEADER_NAME_BY_INDEX,
        WINHTTP_NO_OUTPUT_BUFFER,
        &responseHeadersTempLength,
        WINHTTP_NO_HEADER_INDEX);

    if ((responseHeadersTemp = (wchar_t*)malloc((size_t)responseHeadersTempLength + 2)) == NULL)
    {
        result = HTTPAPI_ALLOC_FAILED;
        LogError("malloc failed: (result = %" PRI_MU_ENUM ")", MU_ENUM_VALUE(HTTPAPI_RESULT, result));
    }
    else if (! WinHttpQueryHeaders(
            requestHandle,
            WINHTTP_QUERY_RAW_HEADERS_CRLF,
            WINHTTP_HEADER_NAME_BY_INDEX,
            responseHeadersTemp,
            &responseHeadersTempLength,
            WINHTTP_NO_HEADER_INDEX))
    {
        result = HTTPAPI_QUERY_HEADERS_FAILED;
        LogErrorWinHTTPWithGetLastErrorAsString("WinHttpQueryHeaders failed (result = %" PRI_MU_ENUM ")", MU_ENUM_VALUE(HTTPAPI_RESULT, result));
    }
    else    
    {
        wchar_t *next_token = NULL;
        wchar_t* token = wcstok_s(responseHeadersTemp, L"\r\n", &next_token);
        char* tokenTemp = NULL;     

        result = HTTPAPI_OK;

        while ((token != NULL) &&
            (token[0] != L'\0'))
        {
            size_t tokenTemp_size;

            tokenTemp_size = WideCharToMultiByte(CP_ACP, 0, token, -1, NULL, 0, NULL, NULL);
            if (tokenTemp_size == 0)
            {
                result = HTTPAPI_STRING_PROCESSING_ERROR;
                LogError("WideCharToMultiByte failed");
                break;
            }
            else if ((tokenTemp = (char*)malloc(sizeof(char) * tokenTemp_size)) == NULL)
            {
                result = HTTPAPI_ALLOC_FAILED;
                LogError("malloc failed");
                break;
            }
            else if (WideCharToMultiByte(CP_ACP, 0, token, -1, tokenTemp, (int)tokenTemp_size, NULL, NULL) == 0)
            {
                result = HTTPAPI_STRING_PROCESSING_ERROR;
                LogError("WideCharToMultiByte failed");
                break;
            }
            else
            {
                /*breaking the token in 2 parts: everything before the first ":" and everything after the first ":"*/
                /*if there is no such character, then skip it*/
                /*if there is a : then replace is by a '\0' and so it breaks the original string in name and value*/
                char* whereIsColon = strchr(tokenTemp, ':');
                if (whereIsColon != NULL)
                {
                    *whereIsColon = '\0';
                    if (HTTPHeaders_AddHeaderNameValuePair(responseHeadersHandle, tokenTemp, whereIsColon + 1) != HTTP_HEADERS_OK)
                    {
                        LogError("HTTPHeaders_AddHeaderNameValuePair failed");
                        result = HTTPAPI_HTTP_HEADERS_FAILED;
                        break;
                    }
                }
            }
            free(tokenTemp);
            tokenTemp = NULL;

            token = wcstok_s(NULL, L"\r\n", &next_token);
        }

        free(tokenTemp);
        tokenTemp = NULL;
    }

    free(responseHeadersTemp);
    return result;
}