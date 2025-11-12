did_set_whichwrap(optset_T *args)
{
    char_u	**varp = (char_u **)args->os_varp;

    // Add ',' to the list flags because 'whichwrap' is a flag
    // list that is comma-separated.
    return did_set_option_listflag(*varp, (char_u *)(WW_ALL ","),
		    args->os_errbuf, args->os_errbuflen);
}