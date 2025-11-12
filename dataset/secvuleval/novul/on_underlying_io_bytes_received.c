static void on_underlying_io_bytes_received(void *context, const unsigned char *buffer, size_t size)
{
    if (context != NULL)
    {
        unsigned char* new_socket_io_read_bytes;
        TLS_IO_INSTANCE *tls_io_instance = (TLS_IO_INSTANCE *)context;

        size_t realloc_size = safe_add_size_t(tls_io_instance->socket_io_read_byte_count, size);
        if (realloc_size == SIZE_MAX)
        {
            LogError("Invalid realloc size");
            tls_io_instance->tlsio_state = TLSIO_STATE_ERROR;
            indicate_error(tls_io_instance);
        }
        else if ((new_socket_io_read_bytes = (unsigned char*)realloc(tls_io_instance->socket_io_read_bytes, realloc_size)) == NULL)
        {
            LogError("realloc failed");
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
        LogError("NULL value passed in context");
    }
}