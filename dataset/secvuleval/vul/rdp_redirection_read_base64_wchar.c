static BOOL rdp_redirection_read_base64_wchar(UINT32 flag, wStream* s, UINT32* pLength,
                                              BYTE** pData)
{
	BOOL rc = FALSE;
	char buffer[64] = { 0 };
	const BYTE* ptr = NULL;

	if (!rdp_redirection_get_data(s, pLength, &ptr))
		return FALSE;
	const WCHAR* wchar = (const WCHAR*)ptr;

	size_t utf8_len = 0;
	char* utf8 = ConvertWCharNToUtf8Alloc(wchar, *pLength, &utf8_len);
	if (!utf8)
		goto fail;

	redirection_free_data(pData, NULL);

	utf8_len = strnlen(utf8, utf8_len);
	*pData = calloc(utf8_len, sizeof(BYTE));
	if (!*pData)
		goto fail;

	size_t rlen = utf8_len;
	size_t wpos = 0;
	char* tok = strtok(utf8, "\r\n");
	while (tok)
	{
		const size_t len = strnlen(tok, rlen);
		rlen -= len;

		size_t bplen = 0;
		BYTE* bptr = NULL;
		crypto_base64_decode(tok, len, &bptr, &bplen);
		if (!bptr)
			goto fail;
		memcpy(&(*pData)[wpos], bptr, bplen);
		wpos += bplen;
		free(bptr);

		tok = strtok(NULL, "\r\n");
	}
	*pLength = wpos;

	WLog_DBG(TAG, "%s:", rdp_redirection_flags_to_string(flag, buffer, sizeof(buffer)));

	rc = TRUE;
fail:
	if (!rc)
		WLog_ERR(TAG, "failed to read base64 data");
	free(utf8);
	return rc;
}