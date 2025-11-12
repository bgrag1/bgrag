static void on_underlying_io_bytes_received(void* context, const unsigned char* buffer, size_t size)
{
    if (context != NULL)
    {
        TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)context;

        unsigned char* new_socket_io_read_bytes = (unsigned char*)realloc(tls_io_instance->socket_io_read_bytes, tls_io_instance->socket_io_read_byte_count + size);
        if (new_socket_io_read_bytes == NULL)
        {
            LogError("Failed allocating memory for received bytes");
            tls_io_instance->tlsio_state = TLSIO_STATE_ERROR;
            indicate_error(tls_io_instance);
        }
        else
        {
            tls_io_instance->socket_io_read_bytes = new_socket_io_read_bytes;
            (void)memcpy(tls_io_instance->socket_io_read_bytes + tls_io_instance->socket_io_read_byte_count, buffer, size);
            tls_io_instance->socket_io_read_byte_count += size;
        }
    }
    else
    {
        LogInfo("Supplied context is NULL on bytes_received");
    }
}