set_option_value(
    char_u	*name,
    long	number,
    char_u	*string,
    int		opt_flags)	// OPT_LOCAL or 0 (both)
{
    int		opt_idx;
    char_u	*varp;
    long_u	flags;
    static char	errbuf[80];

    opt_idx = findoption(name);
    if (opt_idx < 0)
    {
	int key;

	if (STRLEN(name) == 4 && name[0] == 't' && name[1] == '_'
		&& (key = find_key_option(name, FALSE)) != 0)
	{
	    char_u key_name[2];

	    if (key < 0)
	    {
		key_name[0] = KEY2TERMCAP0(key);
		key_name[1] = KEY2TERMCAP1(key);
	    }
	    else
	    {
		key_name[0] = KS_KEY;
		key_name[1] = (key & 0xff);
	    }
	    add_termcode(key_name, string, FALSE);
	    if (full_screen)
		ttest(FALSE);
	    redraw_all_later(UPD_CLEAR);
	    return NULL;
	}

	semsg(_(e_unknown_option_str_2), name);
    }
    else
    {
	flags = options[opt_idx].flags;
#ifdef HAVE_SANDBOX
	// Disallow changing some options in the sandbox
	if (sandbox > 0 && (flags & P_SECURE))
	{
	    emsg(_(e_not_allowed_in_sandbox));
	    return NULL;
	}
#endif
	if (flags & P_STRING)
	    return set_string_option(opt_idx, string, opt_flags, errbuf);

	varp = get_varp_scope(&(options[opt_idx]), opt_flags);
	if (varp != NULL)	// hidden option is not changed
	{
	    if (number == 0 && string != NULL)
	    {
		int idx;

		// Either we are given a string or we are setting option
		// to zero.
		for (idx = 0; string[idx] == '0'; ++idx)
		    ;
		if (string[idx] != NUL || idx == 0)
		{
		    // There's another character after zeros or the string
		    // is empty.  In both cases, we are trying to set a
		    // num option using a string.
		    semsg(_(e_number_required_after_str_equal_str),
								 name, string);
		    return NULL;     // do nothing as we hit an error

		}
	    }
	    if (flags & P_NUM)
	    {
		return set_num_option(opt_idx, varp, number,
					   errbuf, sizeof(errbuf), opt_flags);
	    }
	    else
		return set_bool_option(opt_idx, varp, (int)number, opt_flags);
	}

    }
    return NULL;
}