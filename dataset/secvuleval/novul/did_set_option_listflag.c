did_set_option_listflag(
	char_u *val,
	char_u *flags,
	char *errbuf,
	int errbuflen)
{
    char_u	*s;

    for (s = val; *s; ++s)
	if (vim_strchr(flags, *s) == NULL)
	    return illegal_char(errbuf, errbuflen, *s);

    return NULL;
}