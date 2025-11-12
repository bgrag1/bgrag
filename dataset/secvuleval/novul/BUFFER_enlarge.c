int BUFFER_enlarge(BUFFER_HANDLE handle, size_t enlargeSize)
{
    int result;
    if (handle == NULL)
    {
        /* Codes_SRS_BUFFER_07_017: [BUFFER_enlarge shall return a nonzero result if any parameters are NULL or zero.] */
        LogError("Failure: handle is invalid.");
        result = MU_FAILURE;
    }
    else if (enlargeSize == 0)
    {
        /* Codes_SRS_BUFFER_07_017: [BUFFER_enlarge shall return a nonzero result if any parameters are NULL or zero.] */
        LogError("Failure: enlargeSize size is 0.");
        result = MU_FAILURE;
    }
    else
    {
        unsigned char* temp;
        BUFFER* b = (BUFFER*)handle;
        size_t malloc_size = safe_add_size_t(b->size, enlargeSize);
        if (malloc_size == SIZE_MAX ||
            (temp = (unsigned char*)realloc(b->buffer, malloc_size)) == NULL)
        {
            /* Codes_SRS_BUFFER_07_018: [BUFFER_enlarge shall return a nonzero result if any error is encountered.] */
            LogError("Failure: allocating temp buffer, size:%zu", malloc_size);
            result = MU_FAILURE;
        }
        else
        {
            b->buffer = temp;
            b->size += enlargeSize;
            result = 0;
        }
    }
    return result;
}