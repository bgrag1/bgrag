did_set_langmap(optset_T *args UNUSED)
{
    char_u  *p;
    char_u  *p2;
    int	    from, to;

    ga_clear(&langmap_mapga);		    // clear the previous map first
    langmap_init();			    // back to one-to-one map

    for (p = p_langmap; p[0] != NUL; )
    {
	for (p2 = p; p2[0] != NUL && p2[0] != ',' && p2[0] != ';';
							       MB_PTR_ADV(p2))
	{
	    if (p2[0] == '\\' && p2[1] != NUL)
		++p2;
	}
	if (p2[0] == ';')
	    ++p2;	    // abcd;ABCD form, p2 points to A
	else
	    p2 = NULL;	    // aAbBcCdD form, p2 is NULL
	while (p[0])
	{
	    if (p[0] == ',')
	    {
		++p;
		break;
	    }
	    if (p[0] == '\\' && p[1] != NUL)
		++p;
	    from = (*mb_ptr2char)(p);
	    to = NUL;
	    if (p2 == NULL)
	    {
		MB_PTR_ADV(p);
		if (p[0] != ',')
		{
		    if (p[0] == '\\')
			++p;
		    to = (*mb_ptr2char)(p);
		}
	    }
	    else
	    {
		if (p2[0] != ',')
		{
		    if (p2[0] == '\\')
			++p2;
		    to = (*mb_ptr2char)(p2);
		}
	    }
	    if (to == NUL)
	    {
		sprintf(args->os_errbuf,
			_(e_langmap_matching_character_missing_for_str),
			transchar(from));
		return args->os_errbuf;
	    }

	    if (from >= 256)
		langmap_set_entry(from, to);
	    else
		langmap_mapchar[from & 255] = to;

	    // Advance to next pair
	    MB_PTR_ADV(p);
	    if (p2 != NULL)
	    {
		MB_PTR_ADV(p2);
		if (*p == ';')
		{
		    p = p2;
		    if (p[0] != NUL)
		    {
			if (p[0] != ',')
			{
			    snprintf(args->os_errbuf, args->os_errbuflen,
				    _(e_langmap_extra_characters_after_semicolon_str),
				    p);
			    return args->os_errbuf;
			}
			++p;
		    }
		    break;
		}
	    }
	}
    }

    return NULL;
}