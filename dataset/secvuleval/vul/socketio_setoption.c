int socketio_setoption(CONCRETE_IO_HANDLE socket_io, const char* optionName, const void* value)
{
    int result;

    if (socket_io == NULL ||
        optionName == NULL ||
        value == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        SOCKET_IO_INSTANCE* socket_io_instance = (SOCKET_IO_INSTANCE*)socket_io;

        if (strcmp(optionName, "tcp_keepalive") == 0)
        {
            result = setsockopt(socket_io_instance->socket, SOL_SOCKET, SO_KEEPALIVE, value, sizeof(int));
            if (result == -1) result = errno;
        }
        else if (strcmp(optionName, "tcp_keepalive_time") == 0)
        {
#ifdef __APPLE__
            result = setsockopt(socket_io_instance->socket, IPPROTO_TCP, TCP_KEEPALIVE, value, sizeof(int));
#else
            result = setsockopt(socket_io_instance->socket, SOL_TCP, TCP_KEEPIDLE, value, sizeof(int));
#endif
            if (result == -1) result = errno;
        }
        else if (strcmp(optionName, "tcp_keepalive_interval") == 0)
        {
            result = setsockopt(socket_io_instance->socket, SOL_TCP, TCP_KEEPINTVL, value, sizeof(int));
            if (result == -1) result = errno;
        }
        else if (strcmp(optionName, OPTION_NET_INT_MAC_ADDRESS) == 0)
        {
#ifdef __APPLE__
            LogError("option not supported.");
            result = MU_FAILURE;
#else
            if (strlen(value) == 0)
            {
                LogError("option value must be a valid mac address");
                result = MU_FAILURE;
            }
            else if ((socket_io_instance->target_mac_address = (char*)malloc(sizeof(char) * (strlen(value) + 1))) == NULL)
            {
                LogError("failed setting net_interface_mac_address option (malloc failed)");
                result = MU_FAILURE;
            }
            else
            {
                strcpy(socket_io_instance->target_mac_address, value);
                strtoup(socket_io_instance->target_mac_address);
                result = 0;
            }
#endif
        }
        else if (strcmp(optionName, OPTION_ADDRESS_TYPE) == 0)
        {
            result = socketio_setaddresstype_option(socket_io_instance, (const char*)value);
        }
        else
        {
            result = MU_FAILURE;
        }
    }

    return result;
}