int av_hwframe_ctx_init(AVBufferRef *ref)
{
    AVHWFramesContext *ctx = (AVHWFramesContext*)ref->data;
    const enum AVPixelFormat *pix_fmt;
    int ret;

    if (ctx->internal->source_frames) {
        /* A derived frame context is already initialised. */
        return 0;
    }

    /* validate the pixel format */
    for (pix_fmt = ctx->internal->hw_type->pix_fmts; *pix_fmt != AV_PIX_FMT_NONE; pix_fmt++) {
        if (*pix_fmt == ctx->format)
            break;
    }
    if (*pix_fmt == AV_PIX_FMT_NONE) {
        av_log(ctx, AV_LOG_ERROR,
               "The hardware pixel format '%s' is not supported by the device type '%s'\n",
               av_get_pix_fmt_name(ctx->format), ctx->internal->hw_type->name);
        return AVERROR(ENOSYS);
    }

    /* validate the dimensions */
    ret = av_image_check_size(ctx->width, ctx->height, 0, ctx);
    if (ret < 0)
        return ret;

    /* format-specific init */
    if (ctx->internal->hw_type->frames_init) {
        ret = ctx->internal->hw_type->frames_init(ctx);
        if (ret < 0)
            return ret;
    }

    if (ctx->internal->pool_internal && !ctx->pool)
        ctx->pool = ctx->internal->pool_internal;

    /* preallocate the frames in the pool, if requested */
    if (ctx->initial_pool_size > 0) {
        ret = hwframe_pool_prealloc(ref);
        if (ret < 0)
            return ret;
    }

    return 0;
}