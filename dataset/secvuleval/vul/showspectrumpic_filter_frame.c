static int showspectrumpic_filter_frame(AVFilterLink *inlink, AVFrame *insamples)
{
    AVFilterContext *ctx = inlink->dst;
    ShowSpectrumContext *s = ctx->priv;
    int ret;

    ret = av_audio_fifo_write(s->fifo, (void **)insamples->extended_data, insamples->nb_samples);
    av_frame_free(&insamples);
    return ret;
}