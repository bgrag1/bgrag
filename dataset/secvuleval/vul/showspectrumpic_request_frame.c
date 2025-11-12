static int showspectrumpic_request_frame(AVFilterLink *outlink)
{
    AVFilterContext *ctx = outlink->src;
    ShowSpectrumContext *s = ctx->priv;
    AVFilterLink *inlink = ctx->inputs[0];
    int ret, samples;

    ret = ff_request_frame(inlink);
    samples = av_audio_fifo_size(s->fifo);
    if (ret == AVERROR_EOF && s->outpicref && samples > 0) {
        int consumed = 0;
        int x = 0, sz = s->orientation == VERTICAL ? s->w : s->h;
        int ch, spf, spb;
        AVFrame *fin;

        spf = s->win_size * (samples / ((s->win_size * sz) * ceil(samples / (float)(s->win_size * sz))));
        spf = FFMAX(1, spf);

        spb = (samples / (spf * sz)) * spf;

        fin = ff_get_audio_buffer(inlink, s->win_size);
        if (!fin)
            return AVERROR(ENOMEM);

        while (x < sz) {
            ret = av_audio_fifo_peek(s->fifo, (void **)fin->extended_data, s->win_size);
            if (ret < 0) {
                av_frame_free(&fin);
                return ret;
            }

            av_audio_fifo_drain(s->fifo, spf);

            if (ret < s->win_size) {
                for (ch = 0; ch < s->nb_display_channels; ch++) {
                    memset(fin->extended_data[ch] + ret * sizeof(float), 0,
                           (s->win_size - ret) * sizeof(float));
                }
            }

            ff_filter_execute(ctx, run_channel_fft, fin, NULL, s->nb_display_channels);
            acalc_magnitudes(s);

            consumed += spf;
            if (consumed >= spb) {
                int h = s->orientation == VERTICAL ? s->h : s->w;

                scale_magnitudes(s, 1.f / (consumed / spf));
                plot_spectrum_column(inlink, fin);
                consumed = 0;
                x++;
                for (ch = 0; ch < s->nb_display_channels; ch++)
                    memset(s->magnitudes[ch], 0, h * sizeof(float));
            }
        }

        av_frame_free(&fin);
        s->outpicref->pts = 0;

        if (s->legend)
            draw_legend(ctx, samples);

        ret = ff_filter_frame(outlink, s->outpicref);
        s->outpicref = NULL;
    }

    return ret;
}