static HTTPAPI_RESULT ConstructHeadersString(HTTP_HEADERS_HANDLE httpHeadersHandle, wchar_t** httpHeaders)
{
    HTTPAPI_RESULT result;
    size_t headersCount;

    if (HTTPHeaders_GetHeaderCount(httpHeadersHandle, &headersCount) != HTTP_HEADERS_OK)
    {
        result = HTTPAPI_ERROR;
        LogError("HTTPHeaders_GetHeaderCount failed (result = %" PRI_MU_ENUM ").", MU_ENUM_VALUE(HTTPAPI_RESULT, result));
    }
    else
    {
        size_t i;

        /*the total size of all the headers is given by sumof(lengthof(everyheader)+2)*/
        size_t toAlloc = 0;
        for (i = 0; i < headersCount; i++)
        {
            char *temp;
            if (HTTPHeaders_GetHeader(httpHeadersHandle, i, &temp) == HTTP_HEADERS_OK)
            {
                toAlloc += strlen(temp);
                toAlloc += 2;
                free(temp);
            }
            else
            {
                LogError("HTTPHeaders_GetHeader failed");
                break;
            }
        }

        if (i < headersCount)
        {
            result = HTTPAPI_ERROR;
        }
        else
        {
            char *httpHeadersA;
            size_t requiredCharactersForHeaders;

            if ((httpHeadersA = ConcatHttpHeaders(httpHeadersHandle, toAlloc, headersCount)) == NULL)
            {
                result = HTTPAPI_ERROR;
                LogError("Cannot concatenate headers");
            }
            else if ((requiredCharactersForHeaders = MultiByteToWideChar(CP_ACP, 0, httpHeadersA, -1, NULL, 0)) == 0)
            {
                result = HTTPAPI_STRING_PROCESSING_ERROR;
                LogError("MultiByteToWideChar failed, GetLastError=0x%08x (result = %" PRI_MU_ENUM ")", GetLastError(), MU_ENUM_VALUE(HTTPAPI_RESULT, result));
            }
            else if ((*httpHeaders = (wchar_t*)malloc((requiredCharactersForHeaders + 1) * sizeof(wchar_t))) == NULL)
            {
                result = HTTPAPI_ALLOC_FAILED;
                LogError("Cannot allocate memory (result = %" PRI_MU_ENUM ")", MU_ENUM_VALUE(HTTPAPI_RESULT, result));
            }
            else if (MultiByteToWideChar(CP_ACP, 0, httpHeadersA, -1, *httpHeaders, (int)requiredCharactersForHeaders) == 0)
            {
                result = HTTPAPI_STRING_PROCESSING_ERROR;
                LogError("MultiByteToWideChar failed, GetLastError=0x%08x (result = %" PRI_MU_ENUM ")", GetLastError(), MU_ENUM_VALUE(HTTPAPI_RESULT, result));
                free(*httpHeaders);
                *httpHeaders = NULL;
            }
            else
            {
                result = HTTPAPI_OK;
            }

            free(httpHeadersA);
        }
    }

    return result;
}