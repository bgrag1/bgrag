static UINT ExtractRunLengthRegularFgBg(const BYTE* pbOrderHdr, const BYTE* pbEnd, UINT32* advance)
{
	UINT runLength = 0;

	WINPR_ASSERT(pbOrderHdr);
	WINPR_ASSERT(pbEnd);
	WINPR_ASSERT(advance);

	runLength = (*pbOrderHdr) & g_MaskRegularRunLength;
	if (runLength == 0)
	{
		if (!buffer_within_range(pbOrderHdr, 2, pbEnd))
		{
			*advance = 0;
			return 0;
		}
		runLength = *(pbOrderHdr + 1) + 1;
		(*advance)++;
	}
	else
		runLength = runLength * 8;

	return runLength;
}