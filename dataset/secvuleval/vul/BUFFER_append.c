int BUFFER_append(BUFFER_HANDLE handle1, BUFFER_HANDLE handle2)
{
    int result;
    if ( (handle1 == NULL) || (handle2 == NULL) || (handle1 == handle2) )
    {
        /* Codes_SRS_BUFFER_07_023: [BUFFER_append shall return a nonzero upon any error that is encountered.] */
        result = MU_FAILURE;
    }
    else
    {
        BUFFER* b1 = (BUFFER*)handle1;
        BUFFER* b2 = (BUFFER*)handle2;
        if (b1->buffer == NULL)
        {
            /* Codes_SRS_BUFFER_07_023: [BUFFER_append shall return a nonzero upon any error that is encountered.] */
            result = MU_FAILURE;
        }
        else if (b2->buffer == NULL)
        {
            /* Codes_SRS_BUFFER_07_023: [BUFFER_append shall return a nonzero upon any error that is encountered.] */
            result = MU_FAILURE;
        }
        else
        {
            if (b2->size ==0)
            {
                // b2->size = 0, whatever b1->size is, do nothing
                result = 0;
            }
            else
            {
                // b2->size != 0, whatever b1->size is
                unsigned char* temp = (unsigned char*)realloc(b1->buffer, b1->size + b2->size);
                if (temp == NULL)
                {
                    /* Codes_SRS_BUFFER_07_023: [BUFFER_append shall return a nonzero upon any error that is encountered.] */
                    LogError("Failure: allocating temp buffer.");
                    result = MU_FAILURE;
                }
                else
                {
                    /* Codes_SRS_BUFFER_07_024: [BUFFER_append concatenates b2 onto b1 without modifying b2 and shall return zero on success.]*/
                    b1->buffer = temp;
                    // Append the BUFFER
                    (void)memcpy(&b1->buffer[b1->size], b2->buffer, b2->size);
                    b1->size += b2->size;
                    result = 0;
                }
            }
        }
    }
    return result;
}