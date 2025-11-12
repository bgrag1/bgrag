static void on_bytes_received(void* context, const unsigned char* buffer, size_t size)
{
    unsigned char* new_received_bytes;
    HTTP_HANDLE_DATA* http_instance = (HTTP_HANDLE_DATA*)context;

    if (http_instance != NULL)
    {

        if (buffer == NULL)
        {
            http_instance->is_io_error = 1;
            LogError("NULL pointer error");
        }
        else
        {
            /* Here we got some bytes so we'll buffer them so the receive functions can consumer it */
            size_t malloc_size = http_instance->received_bytes_count + size;
            if (malloc_size < size)
            {
                // check for int overflow
                new_received_bytes = NULL;
                LogError("Invalid size parameter");
            }
            else
            {
                new_received_bytes = (unsigned char*)realloc(http_instance->received_bytes, malloc_size);
            }

            if (new_received_bytes == NULL)
            {
                http_instance->is_io_error = 1;
                LogError("Error allocating memory for received data");
            }
            else
            {
                http_instance->received_bytes = new_received_bytes;
                if (memcpy(http_instance->received_bytes + http_instance->received_bytes_count, buffer, size) == NULL)
                {
                    http_instance->is_io_error = 1;
                    LogError("Error copping received data to the HTTP bufffer");
                }
                else
                {
                    http_instance->received_bytes_count += size;
                }
            }
        }
    }
}