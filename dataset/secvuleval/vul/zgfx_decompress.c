int zgfx_decompress(ZGFX_CONTEXT* zgfx, const BYTE* pSrcData, UINT32 SrcSize, BYTE** ppDstData,
                    UINT32* pDstSize, UINT32 flags)
{
	int status = -1;
	BYTE descriptor = 0;
	wStream sbuffer = { 0 };
	wStream* stream = Stream_StaticConstInit(&sbuffer, pSrcData, SrcSize);

	WINPR_ASSERT(zgfx);
	WINPR_ASSERT(stream);

	if (!Stream_CheckAndLogRequiredLength(TAG, stream, 1))
		goto fail;

	Stream_Read_UINT8(stream, descriptor); /* descriptor (1 byte) */

	if (descriptor == ZGFX_SEGMENTED_SINGLE)
	{
		if (!zgfx_decompress_segment(zgfx, stream, Stream_GetRemainingLength(stream)))
			goto fail;

		*ppDstData = NULL;

		if (zgfx->OutputCount > 0)
			*ppDstData = aligned_zgfx_malloc(zgfx->OutputCount);

		if (!*ppDstData)
			goto fail;

		*pDstSize = zgfx->OutputCount;
		CopyMemory(*ppDstData, zgfx->OutputBuffer, zgfx->OutputCount);
	}
	else if (descriptor == ZGFX_SEGMENTED_MULTIPART)
	{
		UINT32 segmentSize = 0;
		UINT16 segmentNumber = 0;
		UINT16 segmentCount = 0;
		UINT32 uncompressedSize = 0;
		BYTE* pConcatenated = NULL;
		size_t used = 0;

		if (!Stream_CheckAndLogRequiredLength(TAG, stream, 6))
			goto fail;

		Stream_Read_UINT16(stream, segmentCount);     /* segmentCount (2 bytes) */
		Stream_Read_UINT32(stream, uncompressedSize); /* uncompressedSize (4 bytes) */

		if (!Stream_CheckAndLogRequiredLengthOfSize(TAG, stream, segmentCount, sizeof(UINT32)))
			goto fail;

		pConcatenated = aligned_zgfx_malloc(uncompressedSize);

		if (!pConcatenated)
			goto fail;

		*ppDstData = pConcatenated;
		*pDstSize = uncompressedSize;

		for (segmentNumber = 0; segmentNumber < segmentCount; segmentNumber++)
		{
			if (!Stream_CheckAndLogRequiredLength(TAG, stream, sizeof(UINT32)))
				goto fail;

			Stream_Read_UINT32(stream, segmentSize); /* segmentSize (4 bytes) */

			if (!zgfx_decompress_segment(zgfx, stream, segmentSize))
				goto fail;

			if (zgfx->OutputCount > UINT32_MAX - used)
				goto fail;

			if (used + zgfx->OutputCount > uncompressedSize)
				goto fail;

			CopyMemory(pConcatenated, zgfx->OutputBuffer, zgfx->OutputCount);
			pConcatenated += zgfx->OutputCount;
			used += zgfx->OutputCount;
		}
	}
	else
	{
		goto fail;
	}

	status = 1;
fail:
	return status;
}