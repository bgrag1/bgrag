static UINT ExtractRunLengthMegaMega(const BYTE* pbOrderHdr, const BYTE* pbEnd, UINT32* advance)
{
	UINT runLength = 0;

	WINPR_ASSERT(pbOrderHdr);
	WINPR_ASSERT(pbEnd);
	WINPR_ASSERT(advance);

	if (!buffer_within_range(pbOrderHdr, 2, pbEnd))
	{
		*advance = 0;
		return 0;
	}

	runLength = ((UINT16)pbOrderHdr[1]) | (((UINT16)pbOrderHdr[2]) << 8);
	(*advance) += 2;

	return runLength;
}