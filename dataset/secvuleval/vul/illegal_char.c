illegal_char(char *errbuf, int c)
{
    if (errbuf == NULL)
	return "";
    sprintf((char *)errbuf, _(e_illegal_character_str), (char *)transchar(c));
    return errbuf;
}