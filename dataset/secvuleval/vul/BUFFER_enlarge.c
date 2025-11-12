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
        BUFFER* b = (BUFFER*)handle;
        unsigned char* temp = (unsigned char*)realloc(b->buffer, b->size + enlargeSize);
        if (temp == NULL)
        {
            /* Codes_SRS_BUFFER_07_018: [BUFFER_enlarge shall return a nonzero result if any error is encountered.] */
            LogError("Failure: allocating temp buffer.");
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