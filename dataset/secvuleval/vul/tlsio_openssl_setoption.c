int tlsio_openssl_setoption(CONCRETE_IO_HANDLE tls_io, const char* optionName, const void* value)
{
    int result;

    if (tls_io == NULL || optionName == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;

        if (strcmp(OPTION_TRUSTED_CERT, optionName) == 0)
        {
            const char* cert = (const char*)value;
            size_t len;

            if (tls_io_instance->certificate != NULL)
            {
                // Free the memory if it has been previously allocated
                free(tls_io_instance->certificate);
                tls_io_instance->certificate = NULL;
            }

            // Store the certificate
            len = strlen(cert);
            tls_io_instance->certificate = malloc(len + 1);
            if (tls_io_instance->certificate == NULL)
            {
                result = MU_FAILURE;
            }
            else
            {
                strcpy(tls_io_instance->certificate, cert);
                result = 0;
            }

            // If we're previously connected then add the cert to the context
            if (tls_io_instance->ssl_context != NULL)
            {
                result = add_certificate_to_store(tls_io_instance, cert);
            }
        }
        else if (strcmp(OPTION_OPENSSL_CIPHER_SUITE, optionName) == 0)
        {
            if (tls_io_instance->cipher_list != NULL)
            {
                // Free the memory if it has been previously allocated
                free(tls_io_instance->cipher_list);
                tls_io_instance->cipher_list = NULL;
            }

            // Store the cipher suites
            if (mallocAndStrcpy_s((char**)&tls_io_instance->cipher_list, value) != 0)
            {
                LogError("unable to mallocAndStrcpy_s %s", optionName);
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        else if (strcmp(SU_OPTION_X509_CERT, optionName) == 0 || strcmp(OPTION_X509_ECC_CERT, optionName) == 0)
        {
            if (tls_io_instance->x509_certificate != NULL)
            {
                LogError("unable to set x509 options more than once");
                result = MU_FAILURE;
            }
            else
            {
                /*let's make a copy of this option*/
                if (mallocAndStrcpy_s((char**)&tls_io_instance->x509_certificate, value) != 0)
                {
                    LogError("unable to mallocAndStrcpy_s %s", optionName);
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                }
            }
        }
        else if (strcmp(SU_OPTION_X509_PRIVATE_KEY, optionName) == 0 || strcmp(OPTION_X509_ECC_KEY, optionName) == 0)
        {
            if (tls_io_instance->x509_private_key != NULL)
            {
                LogError("unable to set more than once x509 options");
                result = MU_FAILURE;
            }
            else
            {
                /*let's make a copy of this option*/
                if (mallocAndStrcpy_s((char**)&tls_io_instance->x509_private_key, value) != 0)
                {
                    LogError("unable to mallocAndStrcpy_s %s", optionName);
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                }
            }
        }
        #ifndef OPENSSL_NO_ENGINE
        else if (strcmp(OPTION_OPENSSL_ENGINE, optionName) == 0)
        {
            ENGINE_load_builtin_engines();

            if (mallocAndStrcpy_s((char**)&tls_io_instance->engine_id, value) != 0)
            {
                LogError("unable to mallocAndStrcpy_s %s", optionName);
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        #endif // OPENSSL_NO_ENGINE
        else if (strcmp(OPTION_OPENSSL_PRIVATE_KEY_TYPE, optionName) == 0)
        {
            const OPTION_OPENSSL_KEY_TYPE type = *(const OPTION_OPENSSL_KEY_TYPE*)value;
            switch (type)
            {
            case KEY_TYPE_DEFAULT:
            case KEY_TYPE_ENGINE:
                tls_io_instance->x509_private_key_type = type;
                result = 0;
                break;

            default:
                LogError("Unknown x509PrivatekeyType type %d", type);
                result = MU_FAILURE;
            }
        }
        else if (strcmp("tls_validation_callback", optionName) == 0)
        {
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4055)
#endif // WIN32
            tls_io_instance->tls_validation_callback = (TLS_CERTIFICATE_VALIDATION_CALLBACK)value;
#ifdef WIN32
#pragma warning(pop)
#endif // WIN32

            if (tls_io_instance->ssl_context != NULL)
            {
                SSL_CTX_set_cert_verify_callback(tls_io_instance->ssl_context, tls_io_instance->tls_validation_callback, tls_io_instance->tls_validation_callback_data);
            }

            result = 0;
        }
        else if (strcmp("tls_validation_callback_data", optionName) == 0)
        {
            tls_io_instance->tls_validation_callback_data = (void*)value;

            if (tls_io_instance->ssl_context != NULL)
            {
                SSL_CTX_set_cert_verify_callback(tls_io_instance->ssl_context, tls_io_instance->tls_validation_callback, tls_io_instance->tls_validation_callback_data);
            }

            result = 0;
        }
        else if (strcmp(OPTION_TLS_VERSION, optionName) == 0)
        {
            if (tls_io_instance->ssl_context != NULL)
            {
                LogError("Unable to set the tls version after the tls connection is established");
                result = MU_FAILURE;
            }
            else
            {
                const int version_option = *(const int*)value;
                if (version_option == 0 || version_option == 10)
                {
                    tls_io_instance->tls_version = VERSION_1_0;
                }
                else if (version_option == 11)
                {
                    tls_io_instance->tls_version = VERSION_1_1;
                }
                else if (version_option == 12)
                {
                    tls_io_instance->tls_version = VERSION_1_2;
                }
                else
                {
                    LogInfo("Value of TLS version option %d is not found shall default to version 1.2", version_option);
                    tls_io_instance->tls_version = VERSION_1_2;
                }
                result = 0;
            }
        }
        else if (strcmp(optionName, OPTION_UNDERLYING_IO_OPTIONS) == 0)
        {
            if (OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)value, (void*)tls_io_instance->underlying_io) != OPTIONHANDLER_OK)
            {
                LogError("failed feeding options to underlying I/O instance");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        else if (strcmp(optionName, OPTION_SET_TLS_RENEGOTIATION) == 0)
        {
            // No need to do anything for Openssl
            result = 0;
        }
        else if (strcmp("ignore_host_name_check", optionName) == 0)
        {
            bool* server_name_check = (bool*)value;
            tls_io_instance->ignore_host_name_check = *server_name_check;
            result = 0;
        }
        else
        {
            if (tls_io_instance->underlying_io == NULL)
            {
                result = MU_FAILURE;
            }
            else
            {
                result = xio_setoption(tls_io_instance->underlying_io, optionName, value);
            }
        }
    }

    return result;
}