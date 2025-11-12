static char* ConcatHttpHeaders(HTTP_HEADERS_HANDLE httpHeadersHandle, size_t toAlloc, size_t headersCount)
{
    size_t i;
    char* result;

    size_t malloc_size = safe_multiply_size_t(toAlloc, sizeof(char));
    malloc_size = safe_add_size_t(malloc_size, 1);
    if (malloc_size == SIZE_MAX)
    {
        LogError("Invalid malloc size");
        result = NULL;
    }
    else
    {
        result = (char*)malloc(malloc_size);
    }
    
    if (result == NULL)
    {
        LogError("unable to malloc");
    }
    else
    {
        result[0] = '\0';
        for (i = 0; i < headersCount; i++)
        {
            char* temp;
            if (HTTPHeaders_GetHeader(httpHeadersHandle, i, &temp) != HTTP_HEADERS_OK)
            {
                LogError("unable to HTTPHeaders_GetHeader");
                break;
            }
            else
            {
                (void)strcat(result, temp);
                (void)strcat(result, "\r\n");
                free(temp);
            }
        }
    
        if (i < headersCount)
        {
            free(result);
            result = NULL;
        }
        else
        {
            /*all is good*/
        }
    }

    return result;
}