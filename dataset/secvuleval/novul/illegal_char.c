illegal_char(char *errbuf, int errbuflen, int c)
{
    if (errbuf == NULL)
	return "";
    snprintf((char *)errbuf, errbuflen, _(e_illegal_character_str),
		    (char *)transchar(c));
    return errbuf;
}