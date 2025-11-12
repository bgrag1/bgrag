did_set_complete(optset_T *args)
{
    char_u	**varp = (char_u **)args->os_varp;
    char_u	*s;

    // check if it is a valid value for 'complete' -- Acevedo
    for (s = *varp; *s;)
    {
	while (*s == ',' || *s == ' ')
	    s++;
	if (!*s)
	    break;
	if (vim_strchr((char_u *)".wbuksid]tU", *s) == NULL)
	    return illegal_char(args->os_errbuf, *s);
	if (*++s != NUL && *s != ',' && *s != ' ')
	{
	    if (s[-1] == 'k' || s[-1] == 's')
	    {
		// skip optional filename after 'k' and 's'
		while (*s && *s != ',' && *s != ' ')
		{
		    if (*s == '\\' && s[1] != NUL)
			++s;
		    ++s;
		}
	    }
	    else
	    {
		if (args->os_errbuf != NULL)
		{
		    sprintf((char *)args->os_errbuf,
			    _(e_illegal_character_after_chr), *--s);
		    return args->os_errbuf;
		}
		return "";
	    }
	}
    }

    return NULL;
}