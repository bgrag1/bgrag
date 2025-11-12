static void* socketio_CloneOption(const char* name, const void* value)
{
    void* result;

    if (name != NULL)
    {
        result = NULL;

        if (strcmp(name, OPTION_NET_INT_MAC_ADDRESS) == 0)
        {
            if (value == NULL)
            {
                LogError("Failed cloning option %s (value is NULL)", name);
            }
            else
            {
                size_t malloc_size = safe_add_size_t(strlen((char*)value), 1);
                malloc_size = safe_multiply_size_t(malloc_size, sizeof(char));
                if (malloc_size == SIZE_MAX)
                {
                    LogError("Invalid malloc size");
                }
                else if ((result = malloc(malloc_size)) == NULL)
                {
                    LogError("Failed cloning option %s (malloc failed)", name);
                }
                else
                {
                    strcpy((char *)result, (char *)value);
                }
            }
        }
        else
        {
            LogError("Cannot clone option %s (not suppported)", name);
        }
    }
    else
    {
        result = NULL;
    }
    return result;
}