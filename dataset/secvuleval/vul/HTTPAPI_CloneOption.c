HTTPAPI_RESULT HTTPAPI_CloneOption(const char* optionName, const void* value, const void** savedValue)
{
    HTTPAPI_RESULT result;
    size_t certLen;
    char* tempCert;

    if (
        (optionName == NULL) ||
        (value == NULL) ||
        (savedValue == NULL)
        )
    {
        /*Codes_SRS_HTTPAPI_COMPACT_21_067: [ If the optionName is NULL, the HTTPAPI_CloneOption shall return HTTPAPI_INVALID_ARG. ]*/
        /*Codes_SRS_HTTPAPI_COMPACT_21_068: [ If the value is NULL, the HTTPAPI_CloneOption shall return HTTPAPI_INVALID_ARG. ]*/
        /*Codes_SRS_HTTPAPI_COMPACT_21_069: [ If the savedValue is NULL, the HTTPAPI_CloneOption shall return HTTPAPI_INVALID_ARG. ]*/
        result = HTTPAPI_INVALID_ARG;
    }
    else if (strcmp(OPTION_TRUSTED_CERT, optionName) == 0)
    {
#ifdef DO_NOT_COPY_TRUSTED_CERTS_STRING
        *savedValue = (const void*)value;
        result = HTTPAPI_OK;
#else
        certLen = strlen((const char*)value);
        tempCert = (char*)malloc((certLen + 1) * sizeof(char));
        if (tempCert == NULL)
        {
            /*Codes_SRS_HTTPAPI_COMPACT_21_070: [ If any memory allocation get fail, the HTTPAPI_CloneOption shall return HTTPAPI_ALLOC_FAILED. ]*/
            result = HTTPAPI_ALLOC_FAILED;
        }
        else
        {
            /*Codes_SRS_HTTPAPI_COMPACT_21_072: [ If the HTTPAPI_CloneOption get success setting the option, it shall return HTTPAPI_OK. ]*/
            (void)strcpy(tempCert, (const char*)value);
            *savedValue = tempCert;
            result = HTTPAPI_OK;
        }
#endif // DO_NOT_COPY_TRUSTED_CERTS_STRING
    }
    else if (strcmp(SU_OPTION_X509_CERT, optionName) == 0 || strcmp(OPTION_X509_ECC_CERT, optionName) == 0)
    {
        certLen = strlen((const char*)value);
        tempCert = (char*)malloc((certLen + 1) * sizeof(char));
        if (tempCert == NULL)
        {
            /*Codes_SRS_HTTPAPI_COMPACT_21_070: [ If any memory allocation get fail, the HTTPAPI_CloneOption shall return HTTPAPI_ALLOC_FAILED. ]*/
            result = HTTPAPI_ALLOC_FAILED;
        }
        else
        {
            /*Codes_SRS_HTTPAPI_COMPACT_21_072: [ If the HTTPAPI_CloneOption get success setting the option, it shall return HTTPAPI_OK. ]*/
            (void)strcpy(tempCert, (const char*)value);
            *savedValue = tempCert;
            result = HTTPAPI_OK;
        }
    }
    else if (strcmp(SU_OPTION_X509_PRIVATE_KEY, optionName) == 0 || strcmp(OPTION_X509_ECC_KEY, optionName) == 0)
    {
        certLen = strlen((const char*)value);
        tempCert = (char*)malloc((certLen + 1) * sizeof(char));
        if (tempCert == NULL)
        {
            /*Codes_SRS_HTTPAPI_COMPACT_21_070: [ If any memory allocation get fail, the HTTPAPI_CloneOption shall return HTTPAPI_ALLOC_FAILED. ]*/
            result = HTTPAPI_ALLOC_FAILED;
        }
        else
        {
            /*Codes_SRS_HTTPAPI_COMPACT_21_072: [ If the HTTPAPI_CloneOption get success setting the option, it shall return HTTPAPI_OK. ]*/
            (void)strcpy(tempCert, (const char*)value);
            *savedValue = tempCert;
            result = HTTPAPI_OK;
        }
    }
    else if (strcmp(OPTION_HTTP_PROXY, optionName) == 0)
    {
        HTTP_PROXY_OPTIONS* proxy_data = (HTTP_PROXY_OPTIONS*)value;

        HTTP_PROXY_OPTIONS* new_proxy_info = malloc(sizeof(HTTP_PROXY_OPTIONS));
        if (new_proxy_info == NULL)
        {
            LogError("unable to allocate proxy option information");
            result = HTTPAPI_ERROR;
        }
        else
        {
            new_proxy_info->host_address = proxy_data->host_address;
            new_proxy_info->port = proxy_data->port;
            new_proxy_info->password = proxy_data->password;
            new_proxy_info->username = proxy_data->username;
            *savedValue = new_proxy_info;
            result = HTTPAPI_OK;
        }
    }
    else if (strcmp(OPTION_SET_TLS_RENEGOTIATION, optionName) == 0)
    {
        bool* temp = (bool*)malloc(sizeof(bool)); /*shall be freed by HTTPAPIEX_Destroy*/
        if (temp == NULL)
        {
            result = HTTPAPI_ERROR;
            LogError("malloc failed (result = %" PRI_MU_ENUM ")", MU_ENUM_VALUE(HTTPAPI_RESULT, result));
        }
        else
        {
            *temp = *(bool*)value;
            *savedValue = temp;
            result = HTTPAPI_OK;
        }
    }
    else
    {
        /*Codes_SRS_HTTPAPI_COMPACT_21_071: [ If the HTTP do not support the optionName, the HTTPAPI_CloneOption shall return HTTPAPI_INVALID_ARG. ]*/
        result = HTTPAPI_INVALID_ARG;
        LogInfo("unknown option %s", optionName);
    }
    return result;
}