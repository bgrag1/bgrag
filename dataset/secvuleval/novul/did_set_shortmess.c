did_set_shortmess(optset_T *args)
{
    char_u	**varp = (char_u **)args->os_varp;

    return did_set_option_listflag(*varp, (char_u *)SHM_ALL, args->os_errbuf,
		    args->os_errbuflen);
}