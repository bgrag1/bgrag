CONCRETE_IO_HANDLE tlsio_schannel_create(void* io_create_parameters)
{
    TLSIO_CONFIG* tls_io_config = (TLSIO_CONFIG *) io_create_parameters;
    TLS_IO_INSTANCE* result;

    if (tls_io_config == NULL)
    {
        LogError("invalid argument detected: void* io_create_parameters = %p", tls_io_config);
        result = NULL;
    }
    else
    {
        result = (TLS_IO_INSTANCE*)malloc(sizeof(TLS_IO_INSTANCE));
        if (result == NULL)
        {
            LogError("malloc failed");
        }
        else
        {
            (void)memset(result, 0, sizeof(TLS_IO_INSTANCE));

            result->host_name = (SEC_TCHAR*)malloc(sizeof(SEC_TCHAR) * (1 + strlen(tls_io_config->hostname)));
            if (result->host_name == NULL)
            {
                LogError("malloc failed");
                free(result);
                result = NULL;
            }
            else
            {
                SOCKETIO_CONFIG socketio_config;
                const IO_INTERFACE_DESCRIPTION* underlying_io_interface;
                void* io_interface_parameters;

                (void)strcpy(result->host_name, tls_io_config->hostname);

                if (tls_io_config->underlying_io_interface != NULL)
                {
                    underlying_io_interface = tls_io_config->underlying_io_interface;
                    io_interface_parameters = tls_io_config->underlying_io_parameters;
                }
                else
                {
                    socketio_config.hostname = tls_io_config->hostname;
                    socketio_config.port = tls_io_config->port;
                    socketio_config.accepted_socket = NULL;

                    underlying_io_interface = socketio_get_interface_description();
                    io_interface_parameters = &socketio_config;
                }

                if (underlying_io_interface == NULL)
                {
                    LogError("socketio_get_interface_description failed");
                    free(result->host_name);
                    free(result);
                    result = NULL;
                }
                else
                {
                    result->socket_io = xio_create(underlying_io_interface, io_interface_parameters);
                    if (result->socket_io == NULL)
                    {
                        LogError("xio_create failed");
                        free(result->host_name);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        result->pending_io_list = singlylinkedlist_create();
                        if (result->pending_io_list == NULL)
                        {
                            LogError("Failed creating pending IO list.");
                            xio_destroy(result->socket_io);
                            free(result->host_name);
                            free(result);
                            result = NULL;
                        }
                        else
                        {
                            result->received_bytes = NULL;
                            result->received_byte_count = 0;
                            result->buffer_size = 0;
                            result->tlsio_state = TLSIO_STATE_NOT_OPEN;
                            result->x509certificate = NULL;
                            result->x509privatekey = NULL;
                            result->x509_schannel_handle = NULL;
                        }
                    }
                }
            }
        }
    }

    return result;
}