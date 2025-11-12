HTTPAPI_RESULT HTTPAPI_SetOption(HTTP_HANDLE handle, const char* optionName, const void* value)
{
    HTTPAPI_RESULT result;
    HTTP_HANDLE_DATA* http_instance = (HTTP_HANDLE_DATA*)handle;

    if (
        (http_instance == NULL) ||
        (optionName == NULL) ||
        (value == NULL)
        )
    {
        /*Codes_SRS_HTTPAPI_COMPACT_21_059: [ If the handle is NULL, the HTTPAPI_SetOption shall return HTTPAPI_INVALID_ARG. ]*/
        /*Codes_SRS_HTTPAPI_COMPACT_21_060: [ If the optionName is NULL, the HTTPAPI_SetOption shall return HTTPAPI_INVALID_ARG. ]*/
        /*Codes_SRS_HTTPAPI_COMPACT_21_061: [ If the value is NULL, the HTTPAPI_SetOption shall return HTTPAPI_INVALID_ARG. ]*/
        result = HTTPAPI_INVALID_ARG;
    }
    else if (strcmp(OPTION_TRUSTED_CERT, optionName) == 0)
    {
#ifdef DO_NOT_COPY_TRUSTED_CERTS_STRING
        result = HTTPAPI_OK;
        http_instance->certificate = (char*)value;
#else
        int len;

        if (http_instance->certificate)
        {
            free(http_instance->certificate);
        }

        len = (int)strlen((char*)value);
        http_instance->certificate = (char*)malloc((len + 1) * sizeof(char));
        if (http_instance->certificate == NULL)
        {
            /*Codes_SRS_HTTPAPI_COMPACT_21_062: [ If any memory allocation get fail, the HTTPAPI_SetOption shall return HTTPAPI_ALLOC_FAILED. ]*/
            result = HTTPAPI_ALLOC_FAILED;
            LogInfo("unable to allocate memory for the certificate in HTTPAPI_SetOption");
        }
        else
        {
            /*Codes_SRS_HTTPAPI_COMPACT_21_064: [ If the HTTPAPI_SetOption get success setting the option, it shall return HTTPAPI_OK. ]*/
            (void)strcpy(http_instance->certificate, (const char*)value);
            result = HTTPAPI_OK;
        }
#endif // DO_NOT_COPY_TRUSTED_CERTS_STRING
    }
    else if (strcmp(SU_OPTION_X509_CERT, optionName) == 0 || strcmp(OPTION_X509_ECC_CERT, optionName) == 0)
    {
        int len;
        if (http_instance->x509ClientCertificate)
        {
            free(http_instance->x509ClientCertificate);
        }

        len = (int)strlen((char*)value);
        http_instance->x509ClientCertificate = (char*)malloc((len + 1) * sizeof(char));
        if (http_instance->x509ClientCertificate == NULL)
        {
            /*Codes_SRS_HTTPAPI_COMPACT_21_062: [ If any memory allocation get fail, the HTTPAPI_SetOption shall return HTTPAPI_ALLOC_FAILED. ]*/
            result = HTTPAPI_ALLOC_FAILED;
            LogInfo("unable to allocate memory for the client certificate in HTTPAPI_SetOption");
        }
        else
        {
            /*Codes_SRS_HTTPAPI_COMPACT_21_064: [ If the HTTPAPI_SetOption get success setting the option, it shall return HTTPAPI_OK. ]*/
            (void)strcpy(http_instance->x509ClientCertificate, (const char*)value);
            result = HTTPAPI_OK;
        }
    }
    else if (strcmp(SU_OPTION_X509_PRIVATE_KEY, optionName) == 0 || strcmp(OPTION_X509_ECC_KEY, optionName) == 0)
    {
        int len;
        if (http_instance->x509ClientPrivateKey)
        {
            free(http_instance->x509ClientPrivateKey);
        }

        len = (int)strlen((char*)value);
        http_instance->x509ClientPrivateKey = (char*)malloc((len + 1) * sizeof(char));
        if (http_instance->x509ClientPrivateKey == NULL)
        {
            /*Codes_SRS_HTTPAPI_COMPACT_21_062: [ If any memory allocation get fail, the HTTPAPI_SetOption shall return HTTPAPI_ALLOC_FAILED. ]*/
            result = HTTPAPI_ALLOC_FAILED;
            LogInfo("unable to allocate memory for the client private key in HTTPAPI_SetOption");
        }
        else
        {
            /*Codes_SRS_HTTPAPI_COMPACT_21_064: [ If the HTTPAPI_SetOption get success setting the option, it shall return HTTPAPI_OK. ]*/
            (void)strcpy(http_instance->x509ClientPrivateKey, (const char*)value);
            result = HTTPAPI_OK;
        }
    }
    else if (strcmp(OPTION_HTTP_PROXY, optionName) == 0)
    {
        TLSIO_CONFIG tlsio_config;
        HTTP_PROXY_IO_CONFIG proxy_config;
        HTTP_PROXY_OPTIONS* proxy_options = (HTTP_PROXY_OPTIONS*)value;

        if (proxy_options->host_address == NULL)
        {
            LogError("NULL host_address in proxy options");
            result = HTTPAPI_ERROR;
        }
        else if (((proxy_options->username == NULL) || (proxy_options->password == NULL)) &&
                (proxy_options->username != proxy_options->password))
        {
            LogError("Only one of username and password for proxy settings was NULL");
            result = HTTPAPI_ERROR;
        }
        else
        {

            /* Workaround: xio interface is already created when HTTPAPI_CreateConnection is call without proxy support
             * need to destroy the interface and create a new one with proxy information
             */
            OPTIONHANDLER_HANDLE xio_options;
            if ((xio_options = xio_retrieveoptions(http_instance->xio_handle)) == NULL)
            {
                LogError("failed saving underlying I/O transport options");
                result = HTTPAPI_ERROR;
            }
            else
            {
                xio_destroy(http_instance->xio_handle);

                proxy_config.hostname = http_instance->hostName;
                proxy_config.proxy_hostname = proxy_options->host_address;
                proxy_config.password = proxy_options->password;
                proxy_config.username = proxy_options->username;
                proxy_config.proxy_port = proxy_options->port;
                proxy_config.port = 443;

                tlsio_config.hostname = http_instance->hostName;
                tlsio_config.port = 443;
                tlsio_config.underlying_io_interface =  http_proxy_io_get_interface_description();
                tlsio_config.underlying_io_parameters = &proxy_config;
                tlsio_config.invoke_on_send_complete_callback_for_fragments = true;

                http_instance->xio_handle = xio_create(platform_get_default_tlsio(), (void*)&tlsio_config);

                if (http_instance->xio_handle == NULL)
                {
                    LogError("Failed to create xio handle with proxy configuration");
                    result = HTTPAPI_ERROR;
                }
                else
                {
                    if (OptionHandler_FeedOptions(xio_options, http_instance->xio_handle) != OPTIONHANDLER_OK)
                    {
                        LogError("Failed feeding existing options to new xio instance.");
                        result = HTTPAPI_ERROR;
                    }
                    else
                    {
                        result = HTTPAPI_OK;
                    }
                }

                OptionHandler_Destroy(xio_options);
            }
        }
    }
    else if (strcmp(OPTION_SET_TLS_RENEGOTIATION, optionName) == 0)
    {
        bool tls_renegotiation = *(bool*)value;
        http_instance->tls_renegotiation = tls_renegotiation;
        result = HTTPAPI_OK;
    }
    else
    {
        /*Codes_SRS_HTTPAPI_COMPACT_21_063: [ If the HTTP do not support the optionName, the HTTPAPI_SetOption shall return HTTPAPI_INVALID_ARG. ]*/
        result = HTTPAPI_INVALID_ARG;
        LogInfo("unknown option %s", optionName);
    }

    return result;
}