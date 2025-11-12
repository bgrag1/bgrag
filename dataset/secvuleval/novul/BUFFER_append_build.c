int BUFFER_append_build(BUFFER_HANDLE handle, const unsigned char* source, size_t size)
{
    int result;
    if (handle == NULL || source == NULL || size == 0)
    {
        /* Codes_SRS_BUFFER_07_029: [ BUFFER_append_build shall return nonzero if handle or source are NULL or if size is 0. ] */
        LogError("BUFFER_append_build failed invalid parameter handle: %p, source: %p, size: %lu", handle, source, (unsigned long)size);
        result = MU_FAILURE;
    }
    else
    {
        if (handle->buffer == NULL)
        {
            /* Codes_SRS_BUFFER_07_030: [ if handle->buffer is NULL BUFFER_append_build shall allocate the a buffer of size bytes... ] */
            if (BUFFER_safemalloc(handle, size) != 0 || handle->buffer == NULL)
            {
                /* Codes_SRS_BUFFER_07_035: [ If any error is encountered BUFFER_append_build shall return a non-null value. ] */
                LogError("Failure with BUFFER_safemalloc");
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_BUFFER_07_031: [ ... and copy the contents of source to handle->buffer. ] */
                (void)memcpy(handle->buffer, source, size);
                /* Codes_SRS_BUFFER_07_034: [ On success BUFFER_append_build shall return 0 ] */
                result = 0;
            }
        }
        else
        {
            /* Codes_SRS_BUFFER_07_032: [ if handle->buffer is not NULL BUFFER_append_build shall realloc the buffer to be the handle->size + size ] */
            unsigned char* temp;
            size_t malloc_size = safe_add_size_t(handle->size, size);
            if (malloc_size == SIZE_MAX || 
                (temp = (unsigned char*)realloc(handle->buffer, malloc_size)) == NULL)
            {
                /* Codes_SRS_BUFFER_07_035: [ If any error is encountered BUFFER_append_build shall return a non-null value. ] */
                LogError("Failure reallocating temporary buffer, size:%zu", malloc_size);
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_BUFFER_07_033: [ ... and copy the contents of source to the end of the buffer. ] */
                handle->buffer = temp;
                // Append the BUFFER
                (void)memcpy(&handle->buffer[handle->size], source, size);
                handle->size += size;
                /* Codes_SRS_BUFFER_07_034: [ On success BUFFER_append_build shall return 0 ] */
                result = 0;
            }
        }
    }
    return result;
}