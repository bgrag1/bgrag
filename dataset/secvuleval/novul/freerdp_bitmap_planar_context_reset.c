BOOL freerdp_bitmap_planar_context_reset(BITMAP_PLANAR_CONTEXT* context, UINT32 width,
                                         UINT32 height)
{
	if (!context)
		return FALSE;

	context->bgr = FALSE;
	context->maxWidth = PLANAR_ALIGN(width, 4);
	context->maxHeight = PLANAR_ALIGN(height, 4);
	const UINT64 tmp = (UINT64)context->maxWidth * context->maxHeight;
	if (tmp > UINT32_MAX)
		return FALSE;
	context->maxPlaneSize = tmp;

	if (context->maxWidth > UINT32_MAX / 4)
		return FALSE;
	context->nTempStep = context->maxWidth * 4;

	memset(context->planes, 0, sizeof(context->planes));
	memset(context->rlePlanes, 0, sizeof(context->rlePlanes));
	memset(context->deltaPlanes, 0, sizeof(context->deltaPlanes));

	if (context->maxPlaneSize > 0)
	{
		void* tmp = winpr_aligned_recalloc(context->planesBuffer, context->maxPlaneSize, 4, 32);
		if (!tmp)
			return FALSE;
		context->planesBuffer = tmp;

		tmp = winpr_aligned_recalloc(context->pTempData, context->maxPlaneSize, 6, 32);
		if (!tmp)
			return FALSE;
		context->pTempData = tmp;

		tmp = winpr_aligned_recalloc(context->deltaPlanesBuffer, context->maxPlaneSize, 4, 32);
		if (!tmp)
			return FALSE;
		context->deltaPlanesBuffer = tmp;

		tmp = winpr_aligned_recalloc(context->rlePlanesBuffer, context->maxPlaneSize, 4, 32);
		if (!tmp)
			return FALSE;
		context->rlePlanesBuffer = tmp;

		context->planes[0] = &context->planesBuffer[context->maxPlaneSize * 0];
		context->planes[1] = &context->planesBuffer[context->maxPlaneSize * 1];
		context->planes[2] = &context->planesBuffer[context->maxPlaneSize * 2];
		context->planes[3] = &context->planesBuffer[context->maxPlaneSize * 3];
		context->deltaPlanes[0] = &context->deltaPlanesBuffer[context->maxPlaneSize * 0];
		context->deltaPlanes[1] = &context->deltaPlanesBuffer[context->maxPlaneSize * 1];
		context->deltaPlanes[2] = &context->deltaPlanesBuffer[context->maxPlaneSize * 2];
		context->deltaPlanes[3] = &context->deltaPlanesBuffer[context->maxPlaneSize * 3];
	}
	return TRUE;
}