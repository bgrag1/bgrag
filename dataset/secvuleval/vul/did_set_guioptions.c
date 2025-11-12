did_set_guioptions(optset_T *args)
{
    char_u	**varp = (char_u **)args->os_varp;
    char *errmsg;

    errmsg = did_set_option_listflag(*varp, (char_u *)GO_ALL, args->os_errbuf);
    if (errmsg != NULL)
	return errmsg;

    gui_init_which_components(args->os_oldval.string);
    return NULL;
}