do_set_option_value(
    int		opt_idx,
    int		opt_flags,
    char_u	**argp,
    set_prefix_T prefix,
    set_op_T	op,
    long_u	flags,
    char_u	*varp,
    char_u	*key_name,
    int		nextchar,
    int		afterchar,
    int		cp_val,
    int		*stopopteval,
    char	*errbuf,
    size_t	errbuflen)
{
    int		value_checked = FALSE;
    char	*errmsg = NULL;
    char_u	*arg = *argp;

    if (flags & P_BOOL)
    {
	// boolean option
	errmsg = do_set_option_bool(opt_idx, opt_flags, prefix, flags, varp,
						nextchar, afterchar, cp_val);
	if (errmsg != NULL)
	    goto skip;
    }
    else
    {
	// numeric or string option
	if (vim_strchr((char_u *)"=:&<", nextchar) == NULL
		|| prefix != PREFIX_NONE)
	{
	    errmsg = e_invalid_argument;
	    goto skip;
	}

	if (flags & P_NUM)
	{
	    // numeric option
	    errmsg = do_set_option_numeric(opt_idx, opt_flags, &arg, nextchar,
						op, flags, cp_val, varp,
						errbuf, errbuflen);
	    if (errmsg != NULL)
		goto skip;
	}
	else if (opt_idx >= 0)
	{
	    // string option
	    if (do_set_option_string(opt_idx, opt_flags, &arg, nextchar, op,
					flags, cp_val, varp, errbuf, errbuflen,
					&value_checked, &errmsg) == FAIL)
	    {
		if (errmsg != NULL)
		    goto skip;
		*stopopteval = TRUE;
		goto skip;
	    }
	}
	else
	{
	    // key code option
	    errmsg = do_set_option_keycode(&arg, key_name, nextchar);
	    if (errmsg != NULL)
		goto skip;
	}
    }

    if (opt_idx >= 0)
	did_set_option(opt_idx, opt_flags, op == OP_NONE, value_checked);

skip:
    *argp = arg;
    return errmsg;
}