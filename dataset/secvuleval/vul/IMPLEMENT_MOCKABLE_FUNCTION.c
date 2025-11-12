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
        result = (wchar_t*)malloc(((unsigned long long)neededSize + 1)*sizeof(wchar_t));
        if (result == NULL)
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