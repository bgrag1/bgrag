HTTP_HANDLE HTTPAPI_CreateConnection(const char* hostName)
{
    HTTP_HANDLE_DATA* result;
    if (g_HTTPAPIState != HTTPAPI_INITIALIZED)
    {
        LogError("g_HTTPAPIState not HTTPAPI_INITIALIZED");
        result = NULL;
    }
    else
    {
        result = (HTTP_HANDLE_DATA*)malloc(sizeof(HTTP_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("malloc returned NULL.");
        }
        else
        {
            memset(result, 0, sizeof(*result));
            wchar_t* hostNameTemp;
            size_t hostNameTemp_size = MultiByteToWideChar(CP_ACP, 0, hostName, -1, NULL, 0);
            if (hostNameTemp_size == 0)
            {
                LogError("MultiByteToWideChar failed");
                free(result);
                result = NULL;
            }
            else
            {
                size_t malloc_size = safe_multiply_size_t(sizeof(wchar_t), hostNameTemp_size);
                if (malloc_size == SIZE_MAX)
                {
                    LogError("Invalid malloc size");
                    hostNameTemp = NULL;
                }
                else
                {
                    hostNameTemp = (wchar_t*)malloc(malloc_size);
                }

                if (hostNameTemp == NULL)
                {
                    LogError("malloc failed");
                    free(result);
                    result = NULL;
                }
                else
                {
                    if (MultiByteToWideChar(CP_ACP, 0, hostName, -1, hostNameTemp, (int)hostNameTemp_size) == 0)
                    {
                        LogError("MultiByteToWideChar failed");
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        result->ConnectionHandle = WinHttpConnect(
                            g_SessionHandle,
                            hostNameTemp,
                            INTERNET_DEFAULT_HTTPS_PORT,
                            0);

                        if (result->ConnectionHandle == NULL)
                        {
                            LogErrorWinHTTPWithGetLastErrorAsString("WinHttpConnect returned NULL.");
                            free(result);
                            result = NULL;
                        }
                        else
                        {
                            result->timeout = 60000;
                        }
                    }
                    free(hostNameTemp);
                }
            }
        }
    }

    return (HTTP_HANDLE)result;
}