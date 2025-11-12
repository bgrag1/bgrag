set_string_option(
    int		opt_idx,
    char_u	*value,
    int		opt_flags,	// OPT_LOCAL and/or OPT_GLOBAL
    char	*errbuf,
    int		errbuflen)
{
    char_u	*s;
    char_u	**varp;
    char_u	*oldval;
#if defined(FEAT_EVAL)
    char_u	*oldval_l = NULL;
    char_u	*oldval_g = NULL;
    char_u	*saved_oldval = NULL;
    char_u	*saved_oldval_l = NULL;
    char_u	*saved_oldval_g = NULL;
    char_u	*saved_newval = NULL;
#endif
    char	*errmsg = NULL;
    int		value_checked = FALSE;

    if (is_hidden_option(opt_idx))	// don't set hidden option
	return NULL;

    s = vim_strsave(value == NULL ? (char_u *)"" : value);
    if (s == NULL)
	return NULL;

    varp = (char_u **)get_option_varp_scope(opt_idx,
	    (opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0
	    ? (is_global_local_option(opt_idx)
		? OPT_GLOBAL : OPT_LOCAL)
	    : opt_flags);
    oldval = *varp;
#if defined(FEAT_EVAL)
    if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0)
    {
	oldval_l = *(char_u **)get_option_varp_scope(opt_idx, OPT_LOCAL);
	oldval_g = *(char_u **)get_option_varp_scope(opt_idx, OPT_GLOBAL);
    }
#endif
    *varp = s;

#if defined(FEAT_EVAL)
    if (!starting
# ifdef FEAT_CRYPT
	    && !is_crypt_key_option(opt_idx)
# endif
       )
    {
	if (oldval_l != NULL)
	    saved_oldval_l = vim_strsave(oldval_l);
	if (oldval_g != NULL)
	    saved_oldval_g = vim_strsave(oldval_g);
	saved_oldval = vim_strsave(oldval);
	saved_newval = vim_strsave(s);
    }
#endif
    if ((errmsg = did_set_string_option(opt_idx, varp, oldval, value, errbuf,
		    errbuflen, opt_flags, OP_NONE, &value_checked)) == NULL)
	did_set_option(opt_idx, opt_flags, TRUE, value_checked);

#if defined(FEAT_EVAL)
    // call autocommand after handling side effects
    if (errmsg == NULL)
	trigger_optionset_string(opt_idx, opt_flags,
		saved_oldval, saved_oldval_l,
		saved_oldval_g, saved_newval);
    vim_free(saved_oldval);
    vim_free(saved_oldval_l);
    vim_free(saved_oldval_g);
    vim_free(saved_newval);
#endif
    return errmsg;
}