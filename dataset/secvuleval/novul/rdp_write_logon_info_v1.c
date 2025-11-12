static BOOL rdp_write_logon_info_v1(wStream* s, logon_info* info)
{
	const size_t charLen = 52 / sizeof(WCHAR);
	const size_t userCharLen = 512 / sizeof(WCHAR);

	size_t sz = 4 + 52 + 4 + 512 + 4;
	size_t len = 0;

	if (!Stream_EnsureRemainingCapacity(s, sz))
		return FALSE;

	/* domain */
	WINPR_ASSERT(info);
	if (!info->domain || !info->username)
		return FALSE;

	len = strnlen(info->domain, charLen + 1);
	if (len > charLen)
		return FALSE;

	Stream_Write_UINT32(s, len * sizeof(WCHAR));
	if (Stream_Write_UTF16_String_From_UTF8(s, charLen, info->domain, len, TRUE) < 0)
		return FALSE;

	/* username */
	len = strnlen(info->username, userCharLen + 1);
	if (len > userCharLen)
		return FALSE;

	Stream_Write_UINT32(s, len * sizeof(WCHAR));
	if (Stream_Write_UTF16_String_From_UTF8(s, userCharLen, info->username, len, TRUE) < 0)
		return FALSE;

	/* sessionId */
	Stream_Write_UINT32(s, info->sessionId);
	return TRUE;
}