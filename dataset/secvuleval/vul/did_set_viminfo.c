did_set_viminfo(optset_T *args)
{
    char_u	*s;
    char	*errmsg = NULL;

    for (s = p_viminfo; *s;)
    {
	// Check it's a valid character
	if (vim_strchr((char_u *)"!\"%'/:<@cfhnrs", *s) == NULL)
	{
	    errmsg = illegal_char(args->os_errbuf, *s);
	    break;
	}
	if (*s == 'n')	// name is always last one
	    break;
	else if (*s == 'r') // skip until next ','
	{
	    while (*++s && *s != ',')
		;
	}
	else if (*s == '%')
	{
	    // optional number
	    while (vim_isdigit(*++s))
		;
	}
	else if (*s == '!' || *s == 'h' || *s == 'c')
	    ++s;		// no extra chars
	else		// must have a number
	{
	    while (vim_isdigit(*++s))
		;

	    if (!VIM_ISDIGIT(*(s - 1)))
	    {
		if (args->os_errbuf != NULL)
		{
		    sprintf(args->os_errbuf,
			    _(e_missing_number_after_angle_str_angle),
			    transchar_byte(*(s - 1)));
		    errmsg = args->os_errbuf;
		}
		else
		    errmsg = "";
		break;
	    }
	}
	if (*s == ',')
	    ++s;
	else if (*s)
	{
	    if (args->os_errbuf != NULL)
		errmsg = e_missing_comma;
	    else
		errmsg = "";
	    break;
	}
    }
    if (*p_viminfo && errmsg == NULL && get_viminfo_parameter('\'') < 0)
	errmsg = e_must_specify_a_value;

    return errmsg;
}