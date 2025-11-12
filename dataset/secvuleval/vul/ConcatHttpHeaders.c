static char* ConcatHttpHeaders(HTTP_HEADERS_HANDLE httpHeadersHandle, size_t toAlloc, size_t headersCount)
{
    char *result = (char*)malloc(toAlloc * sizeof(char) + 1);
    size_t i;
    
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