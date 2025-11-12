static int showspectrumpic_filter_frame(AVFilterLink *inlink, AVFrame *insamples)
{
    AVFilterContext *ctx = inlink->dst;
    ShowSpectrumContext *s = ctx->priv;
    void *ptr;

    if (s->nb_frames + 1ULL > s->frames_size / sizeof(*(s->frames))) {
        ptr = av_fast_realloc(s->frames, &s->frames_size, s->frames_size * 2);
        if (!ptr)
            return AVERROR(ENOMEM);
        s->frames = ptr;
    }

    s->frames[s->nb_frames] = insamples;
    s->samples += insamples->nb_samples;
    s->nb_frames++;

    return 0;
}