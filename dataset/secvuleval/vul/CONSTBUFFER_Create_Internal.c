static CONSTBUFFER_HANDLE CONSTBUFFER_Create_Internal(const unsigned char* source, size_t size)
{
    CONSTBUFFER_HANDLE result;
    /*Codes_SRS_CONSTBUFFER_02_005: [The non-NULL handle returned by CONSTBUFFER_Create shall have its ref count set to "1".]*/
    /*Codes_SRS_CONSTBUFFER_02_010: [The non-NULL handle returned by CONSTBUFFER_CreateFromBuffer shall have its ref count set to "1".]*/
    result = (CONSTBUFFER_HANDLE)calloc(1, (sizeof(CONSTBUFFER_HANDLE_DATA) + size));
    if (result == NULL)
    {
        /*Codes_SRS_CONSTBUFFER_02_003: [If creating the copy fails then CONSTBUFFER_Create shall return NULL.]*/
        /*Codes_SRS_CONSTBUFFER_02_008: [If copying the content fails, then CONSTBUFFER_CreateFromBuffer shall fail and return NULL.] */
        LogError("unable to malloc");
        /*return as is*/
    }
    else
    {
        INIT_REF_VAR(result->count);

        /*Codes_SRS_CONSTBUFFER_02_002: [Otherwise, CONSTBUFFER_Create shall create a copy of the memory area pointed to by source having size bytes.]*/
        result->alias.size = size;
        if (size == 0)
        {
            result->alias.buffer = NULL;
        }
        else
        {
            unsigned char* temp = (unsigned char*)(result + 1);
            /*Codes_SRS_CONSTBUFFER_02_004: [Otherwise CONSTBUFFER_Create shall return a non-NULL handle.]*/
            /*Codes_SRS_CONSTBUFFER_02_007: [Otherwise, CONSTBUFFER_CreateFromBuffer shall copy the content of buffer.]*/
            /*Codes_SRS_CONSTBUFFER_02_009: [Otherwise, CONSTBUFFER_CreateFromBuffer shall return a non-NULL handle.]*/
            (void)memcpy(temp, source, size);
            result->alias.buffer = temp;
        }

        result->buffer_type = CONSTBUFFER_TYPE_COPIED;
    }
    return result;
}