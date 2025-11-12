IMPLEMENT_MOCKABLE_FUNCTION(, wchar_t*, vsprintf_wchar, const wchar_t*, format, va_list, va)
{
    wchar_t* result;
    int neededSize = vswprintf(NULL, 0, format, va);
    if (neededSize < 0)
    {
        LogError("failure in swprintf");
        result = NULL;
    }
    else
    {
        size_t malloc_size = safe_add_size_t((unsigned long long)neededSize, 1);
        malloc_size = safe_multiply_size_t(malloc_size, sizeof(wchar_t));
        if (malloc_size == SIZE_MAX)
        {
            LogError("invalid malloc size");
            result = NULL;
            /*return as is*/
        }
        else if ((result = (wchar_t*)malloc(malloc_size)) == NULL)
        {
            LogError("failure in malloc");
            /*return as is*/
        }
        else
        {
            if (vswprintf(result, (unsigned long long)neededSize + 1, format, va) != neededSize)
            {
                LogError("inconsistent vswprintf behavior");
                free(result);
                result = NULL;
            }
        }
    }
    return result;
}