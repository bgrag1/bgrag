BOOL freerdp_image_copy(BYTE* pDstData, DWORD DstFormat, UINT32 nDstStep, UINT32 nXDst,
                        UINT32 nYDst, UINT32 nWidth, UINT32 nHeight, const BYTE* pSrcData,
                        DWORD SrcFormat, UINT32 nSrcStep, UINT32 nXSrc, UINT32 nYSrc,
                        const gdiPalette* palette, UINT32 flags)
{
	const UINT32 dstByte = FreeRDPGetBytesPerPixel(DstFormat);
	const UINT32 srcByte = FreeRDPGetBytesPerPixel(SrcFormat);

	if ((nHeight > INT32_MAX) || (nWidth > INT32_MAX))
		return FALSE;

	if (!pDstData || !pSrcData)
		return FALSE;

	if ((nWidth == 0) || (nHeight == 0))
		return TRUE;

	if (nDstStep == 0)
		nDstStep = nWidth * FreeRDPGetBytesPerPixel(DstFormat);

	if (nSrcStep == 0)
		nSrcStep = nWidth * FreeRDPGetBytesPerPixel(SrcFormat);

	const BOOL ovl = overlapping(pDstData, nXDst, nYDst, nDstStep, dstByte, pSrcData, nXSrc, nYSrc,
	                             nSrcStep, srcByte, nWidth, nHeight);
	if (ovl)
		return freerdp_image_copy_overlap(pDstData, DstFormat, nDstStep, nXDst, nYDst, nWidth,
		                                  nHeight, pSrcData, SrcFormat, nSrcStep, nXSrc, nYSrc,
		                                  palette, flags);
	return freerdp_image_copy_no_overlap(pDstData, DstFormat, nDstStep, nXDst, nYDst, nWidth,
	                                     nHeight, pSrcData, SrcFormat, nSrcStep, nXSrc, nYSrc,
	                                     palette, flags);
}