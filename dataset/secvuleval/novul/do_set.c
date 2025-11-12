do_set(
    char_u	*arg_start,	// option string (may be written to!)
    int		opt_flags)
{
    char_u	*arg = arg_start;
    int		i;
    int		did_show = FALSE;   // already showed one value

    if (*arg == NUL)
    {
	showoptions(0, opt_flags);
	did_show = TRUE;
	goto theend;
    }

    while (*arg != NUL)		// loop to process all options
    {
	if (STRNCMP(arg, "all", 3) == 0 && !ASCII_ISALPHA(arg[3])
						&& !(opt_flags & OPT_MODELINE))
	{
	    // ":set all"  show all options.
	    // ":set all&" set all options to their default value.
	    arg += 3;
	    if (*arg == '&')
	    {
		++arg;
		// Only for :set command set global value of local options.
		set_options_default(OPT_FREE | opt_flags);
		didset_options();
		didset_options2();
		redraw_all_later(UPD_CLEAR);
	    }
	    else
	    {
		showoptions(1, opt_flags);
		did_show = TRUE;
	    }
	}
	else if (STRNCMP(arg, "termcap", 7) == 0 && !(opt_flags & OPT_MODELINE))
	{
	    showoptions(2, opt_flags);
	    show_termcodes(opt_flags);
	    did_show = TRUE;
	    arg += 7;
	}
	else
	{
	    int		stopopteval = FALSE;
	    char	*errmsg = NULL;
	    char	errbuf[ERR_BUFLEN];
	    char_u	*startarg = arg;

	    errmsg = do_set_option(opt_flags, &arg, arg_start, &startarg,
					&did_show, &stopopteval, errbuf,
					ERR_BUFLEN);
	    if (stopopteval)
		break;

	    // Advance to next argument.
	    // - skip until a blank found, taking care of backslashes
	    // - skip blanks
	    // - skip one "=val" argument (for hidden options ":set gfn =xx")
	    for (i = 0; i < 2 ; ++i)
	    {
		while (*arg != NUL && !VIM_ISWHITE(*arg))
		    if (*arg++ == '\\' && *arg != NUL)
			++arg;
		arg = skipwhite(arg);
		if (*arg != '=')
		    break;
	    }

	    if (errmsg != NULL)
	    {
		vim_strncpy(IObuff, (char_u *)_(errmsg), IOSIZE - 1);
		i = (int)STRLEN(IObuff) + 2;
		if (i + (arg - startarg) < IOSIZE)
		{
		    // append the argument with the error
		    STRCAT(IObuff, ": ");
		    mch_memmove(IObuff + i, startarg, (arg - startarg));
		    IObuff[i + (arg - startarg)] = NUL;
		}
		// make sure all characters are printable
		trans_characters(IObuff, IOSIZE);

		++no_wait_return;		// wait_return() done later
		emsg((char *)IObuff);	// show error highlighted
		--no_wait_return;

		return FAIL;
	    }
	}

	arg = skipwhite(arg);
    }

theend:
    if (silent_mode && did_show)
    {
	// After displaying option values in silent mode.
	silent_mode = FALSE;
	info_message = TRUE;	// use mch_msg(), not mch_errmsg()
	msg_putchar('\n');
	cursor_on();		// msg_start() switches it off
	out_flush();
	silent_mode = TRUE;
	info_message = FALSE;	// use mch_msg(), not mch_errmsg()
    }

    return OK;
}