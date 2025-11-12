did_set_concealcursor(optset_T *args)
{
    char_u	**varp = (char_u **)args->os_varp;

    return did_set_option_listflag(*varp, (char_u *)COCU_ALL, args->os_errbuf);
}