HTTPAPI_RESULT HTTPAPI_ExecuteRequest(HTTP_HANDLE handle,
        HTTPAPI_REQUEST_TYPE requestType, const char* relativePath,
        HTTP_HEADERS_HANDLE httpHeadersHandle, const unsigned char* content,
        size_t contentLength, unsigned int* statusCode,
        HTTP_HEADERS_HANDLE responseHeadersHandle,
        BUFFER_HANDLE responseContent)
{
    HTTPCli_Handle cli = (HTTPCli_Handle) handle;
    int ret;
    int offset;
    size_t cnt;
    char contentBuf[CONTENT_BUF_LEN] = {0};
    char *hname;
    char *hvalue;
    const char *method;
    bool moreFlag;

    method = getHttpMethod(requestType);

    if ((cli == NULL) || (method == NULL) || (relativePath == NULL)
            || (statusCode == NULL) || (responseHeadersHandle == NULL)) {
        LogError("Invalid arguments: handle=%p, requestType=%d, relativePath=%p, statusCode=%p, responseHeadersHandle=%p",
            handle, (int)requestType, relativePath, statusCode, responseHeadersHandle);
        return (HTTPAPI_INVALID_ARG);
    }
    else if (HTTPHeaders_GetHeaderCount(httpHeadersHandle, &cnt)
            != HTTP_HEADERS_OK) {
        LogError("Cannot get header count");
        return (HTTPAPI_QUERY_HEADERS_FAILED);
    }

    /* Send the request line */
    ret = HTTPCli_sendRequest(cli, method, relativePath, true);
    if (ret < 0) {
        LogError("HTTPCli_sendRequest failed, ret=%d", ret);
        return (HTTPAPI_SEND_REQUEST_FAILED);
    }

    /* Send the request headers */
    while (cnt--) {
        ret = HTTPHeaders_GetHeader(httpHeadersHandle, cnt, &hname);
        if (ret != HTTP_HEADERS_OK) {
            LogError("Cannot get request header %d", cnt);
            return (HTTPAPI_QUERY_HEADERS_FAILED);
        }

        ret = splitHeader(hname, &hvalue);

        if (ret == 0) {
            ret = HTTPCli_sendField(cli, hname, hvalue, false);
        }

        free(hname);
        hname = NULL;

        if (ret < 0) {
            LogError("HTTP send field failed, ret=%d", ret);
            return (HTTPAPI_SEND_REQUEST_FAILED);
        }
    }

    /* Send the last header and request body */
    ret = HTTPCli_sendField(cli, NULL, NULL, true);
    if (ret < 0) {
        LogError("HTTP send empty field failed, ret=%d", ret);
        return (HTTPAPI_SEND_REQUEST_FAILED);
    }

    if (content && contentLength != 0) {
        ret = HTTPCli_sendRequestBody(cli, (const char *)content,
                contentLength);
        if (ret < 0) {
            LogError("HTTP send request body failed, ret=%d", ret);
            return (HTTPAPI_SEND_REQUEST_FAILED);
        }
    }

    /* Get the response status code */
    ret = HTTPCli_getResponseStatus(cli);
    if (ret < 0) {
        LogError("HTTP receive response failed, ret=%d", ret);
        return (HTTPAPI_RECEIVE_RESPONSE_FAILED);
    }
    *statusCode = (unsigned int)ret;

    /* Get the response headers */
    hname = NULL;
    cnt = 0;
    offset = 0;
    do {
        ret = HTTPCli_readResponseHeader(cli, contentBuf, CONTENT_BUF_LEN,
            &moreFlag);
        if (ret < 0) {
            LogError("HTTP read response header failed, ret=%d", ret);
            ret = HTTPAPI_RECEIVE_RESPONSE_FAILED;
            goto headersDone;
        }
        else if (ret == 0) {
            /* All headers read */
            goto headersDone;
        }

        if (cnt < offset + ret) {
            size_t malloc_size = safe_add_size_t(offset, ret);
            if (malloc_size == SIZE_MAX)
            {
                LogError("invalid realloc size");
                hname = NULL;
            }
            else
            {
                hname = (char*)realloc(hname, malloc_size);
            }

            if (hname == NULL) {
                LogError("Failed reallocating memory");
                ret = HTTPAPI_ALLOC_FAILED;
                goto headersDone;
            }
            cnt = offset + ret;
        }

        memcpy(hname + offset, contentBuf, ret);
        offset += ret;

        if (moreFlag) {
            continue;
        }

        ret = splitHeader(hname, &hvalue);
        if (ret < 0) {
            LogError("HTTP split header failed, ret=%d", ret);
            ret = HTTPAPI_HTTP_HEADERS_FAILED;
            goto headersDone;
        }

        ret = HTTPHeaders_AddHeaderNameValuePair(responseHeadersHandle,
                hname, hvalue);
        if (ret != HTTP_HEADERS_OK) {
            LogError("Adding the response header failed");
            ret = HTTPAPI_HTTP_HEADERS_FAILED;
            goto headersDone;
        }
        offset = 0;
    } while (1);

headersDone:
    free(hname);
    hname = NULL;
    if (ret != 0) {
        return ((HTTPAPI_RESULT)ret);
    }

    /* Get response body */
    if (responseContent != NULL) {
        offset = 0;
        cnt = 0;

        do {
            ret = HTTPCli_readResponseBody(cli, contentBuf, CONTENT_BUF_LEN,
                    &moreFlag);

            if (ret < 0) {
                LogError("HTTP read response body failed, ret=%d", ret);
                ret = HTTPAPI_RECEIVE_RESPONSE_FAILED;
                goto contentDone;
            }

            if (ret != 0) {
                cnt = ret;
                ret = BUFFER_enlarge(responseContent, cnt);
                if (ret != 0) {
                    LogError("Failed enlarging response buffer");
                    ret = HTTPAPI_ALLOC_FAILED;
                    goto contentDone;
                }

                ret = BUFFER_content(responseContent,
                        (const unsigned char **)&hname);
                if (ret != 0) {
                    LogError("Failed getting the response buffer content");
                    ret = HTTPAPI_ALLOC_FAILED;
                    goto contentDone;
                }

                memcpy(hname + offset, contentBuf, cnt);
                offset += cnt;
            }
        } while (moreFlag);

    contentDone:
        if (ret < 0) {
            BUFFER_unbuild(responseContent);
            return ((HTTPAPI_RESULT)ret);
        }
    }

    return (HTTPAPI_OK);
}