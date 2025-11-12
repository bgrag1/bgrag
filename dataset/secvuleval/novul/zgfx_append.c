static BOOL zgfx_append(ZGFX_CONTEXT* zgfx, BYTE** ppConcatenated, size_t uncompressedSize,
                        size_t* pUsed)
{
	WINPR_ASSERT(zgfx);
	WINPR_ASSERT(ppConcatenated);
	WINPR_ASSERT(pUsed);

	const size_t used = *pUsed;
	if (zgfx->OutputCount > UINT32_MAX - used)
		return FALSE;

	if (used + zgfx->OutputCount > uncompressedSize)
		return FALSE;

	BYTE* tmp = realloc(*ppConcatenated, used + zgfx->OutputCount + 64ull);
	if (!tmp)
		return FALSE;
	*ppConcatenated = tmp;
	CopyMemory(&tmp[used], zgfx->OutputBuffer, zgfx->OutputCount);
	*pUsed = used + zgfx->OutputCount;
	return TRUE;
}