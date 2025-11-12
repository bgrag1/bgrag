did_set_comments(optset_T *args)
{
    char_u	**varp = (char_u **)args->os_varp;
    char_u	*s;
    char	*errmsg = NULL;

    for (s = *varp; *s; )
    {
	while (*s && *s != ':')
	{
	    if (vim_strchr((char_u *)COM_ALL, *s) == NULL
		    && !VIM_ISDIGIT(*s) && *s != '-')
	    {
		errmsg = illegal_char(args->os_errbuf, *s);
		break;
	    }
	    ++s;
	}
	if (*s++ == NUL)
	    errmsg = e_missing_colon;
	else if (*s == ',' || *s == NUL)
	    errmsg = e_zero_length_string;
	if (errmsg != NULL)
	    break;
	while (*s && *s != ',')
	{
	    if (*s == '\\' && s[1] != NUL)
		++s;
	    ++s;
	}
	s = skip_to_option_part(s);
    }

    return errmsg;
}