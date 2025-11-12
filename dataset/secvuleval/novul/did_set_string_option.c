did_set_string_option(
    int		opt_idx,		// index in options[] table
    char_u	**varp,			// pointer to the option variable
    char_u	*oldval,		// previous value of the option
    char_u	*value,			// new value of the option
    char	*errbuf,		// buffer for errors, or NULL
    int		errbuflen,		// length of error buffer
    int		opt_flags,		// OPT_LOCAL and/or OPT_GLOBAL
    set_op_T    op,			// OP_ADDING/OP_PREPENDING/OP_REMOVING
    int		*value_checked)		// value was checked to be safe, no
					// need to set P_INSECURE
{
    char	*errmsg = NULL;
    long_u	free_oldval = (get_option_flags(opt_idx) & P_ALLOCED);
    opt_did_set_cb_T did_set_cb = get_option_did_set_cb(opt_idx);
    optset_T	args;

    // 'ttytype' is an alias for 'term'.  Both 'term' and 'ttytype' point to
    // T_NAME.  If 'term' or 'ttytype' is modified, then use the index for the
    // 'term' option.  Only set the P_ALLOCED flag on 'term'.
    if (varp == &T_NAME)
    {
	opt_idx = findoption((char_u *)"term");
	if (opt_idx >= 0)
	{
	    free_oldval = (get_option_flags(opt_idx) & P_ALLOCED);
	    did_set_cb = get_option_did_set_cb(opt_idx);
	}
    }

    CLEAR_FIELD(args);

    // Disallow changing some options from secure mode
    if ((secure
#ifdef HAVE_SANDBOX
		|| sandbox != 0
#endif
		) && (get_option_flags(opt_idx) & P_SECURE))
	errmsg = e_not_allowed_here;
    // Check for a "normal" directory or file name in some options.
    else if (check_illegal_path_names(opt_idx, varp))
	errmsg = e_invalid_argument;
    else if (did_set_cb != NULL)
    {
	args.os_varp = (char_u *)varp;
	args.os_idx = opt_idx;
	args.os_flags = opt_flags;
	args.os_op = op;
	args.os_oldval.string = oldval;
	args.os_newval.string = value;
	args.os_errbuf = errbuf;
	args.os_errbuflen = errbuflen;
	// Invoke the option specific callback function to validate and apply
	// the new option value.
	errmsg = did_set_cb(&args);

	// The 'keymap', 'filetype' and 'syntax' option callback functions
	// may change the os_value_checked field.
	*value_checked = args.os_value_checked;
    }

    // If an error is detected, restore the previous value.
    if (errmsg != NULL)
    {
	free_string_option(*varp);
	*varp = oldval;
	// When resetting some values, need to act on it.
	if (args.os_restore_chartab)
	    (void)init_chartab();
	if (varp == &p_hl)
	    (void)highlight_changed();
    }
    else
    {
#ifdef FEAT_EVAL
	// Remember where the option was set.
	set_option_sctx_idx(opt_idx, opt_flags, current_sctx);
#endif
	// Free string options that are in allocated memory.
	// Use "free_oldval", because recursiveness may change the flags under
	// our fingers (esp. init_highlight()).
	if (free_oldval)
	    free_string_option(oldval);
	set_option_flag(opt_idx, P_ALLOCED);

	if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0
		&& is_global_local_option(opt_idx))
	{
	    // global option with local value set to use global value; free
	    // the local value and make it empty
	    char_u *p = get_option_varp_scope(opt_idx, OPT_LOCAL);
	    free_string_option(*(char_u **)p);
	    *(char_u **)p = empty_option;
	}

	// May set global value for local option.
	else if (!(opt_flags & OPT_LOCAL) && opt_flags != OPT_GLOBAL)
	    set_string_option_global(opt_idx, varp);

	// Trigger the autocommand only after setting the flags.
#ifdef FEAT_SYN_HL
	if (varp == &(curbuf->b_p_syn))
	    do_syntax_autocmd(args.os_value_changed);
#endif
	else if (varp == &(curbuf->b_p_ft))
	    do_filetype_autocmd(varp, opt_flags, args.os_value_changed);
#ifdef FEAT_SPELL
	if (varp == &(curwin->w_s->b_p_spl))
	    do_spelllang_source();
#endif
    }

    if (varp == &p_mouse)
    {
	if (*p_mouse == NUL)
	    mch_setmouse(FALSE);    // switch mouse off
	else
	    setmouse();		    // in case 'mouse' changed
    }

#if defined(FEAT_LUA) || defined(PROTO)
    if (varp == &p_rtp)
	update_package_paths_in_lua();
#endif

#if defined(FEAT_LINEBREAK)
    // Changing Formatlistpattern when briopt includes the list setting:
    // redraw
    if ((varp == &p_flp || varp == &(curbuf->b_p_flp))
	    && curwin->w_briopt_list)
	redraw_all_later(UPD_NOT_VALID);
#endif

    if (curwin->w_curswant != MAXCOL
		   && (get_option_flags(opt_idx) & (P_CURSWANT | P_RALL)) != 0)
	curwin->w_set_curswant = TRUE;

    if ((opt_flags & OPT_NO_REDRAW) == 0)
    {
#ifdef FEAT_GUI
	// set when changing an option that only requires a redraw in the GUI
	int	redraw_gui_only = FALSE;

	if (varp == &p_go			// 'guioptions'
		|| varp == &p_guifont		// 'guifont'
# ifdef FEAT_GUI_TABLINE
		|| varp == &p_gtl		// 'guitablabel'
		|| varp == &p_gtt		// 'guitabtooltip'
# endif
# ifdef FEAT_XFONTSET
		|| varp == &p_guifontset	// 'guifontset'
# endif
		|| varp == &p_guifontwide	// 'guifontwide'
# ifdef FEAT_GUI_GTK
		|| varp == &p_guiligatures	// 'guiligatures'
# endif
	    )
	    redraw_gui_only = TRUE;

	// check redraw when it's not a GUI option or the GUI is active.
	if (!redraw_gui_only || gui.in_use)
#endif
	    check_redraw(get_option_flags(opt_idx));
    }

#if defined(FEAT_VTP) && defined(FEAT_TERMGUICOLORS)
    if (args.os_did_swaptcap)
    {
	set_termname((char_u *)"win32");
	init_highlight(TRUE, FALSE);
    }
#endif

    return errmsg;
}