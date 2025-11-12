static int showspectrumpic_request_frame(AVFilterLink *outlink)
{
    AVFilterContext *ctx = outlink->src;
    ShowSpectrumContext *s = ctx->priv;
    AVFilterLink *inlink = ctx->inputs[0];
    int ret;

    ret = ff_request_frame(inlink);
    if (ret == AVERROR_EOF && s->outpicref && s->samples > 0) {
        int consumed = 0;
        int x = 0, sz = s->orientation == VERTICAL ? s->w : s->h;
        unsigned int nb_frame = 0;
        int ch, spf, spb;
        int src_offset = 0;
        AVFrame *fin;

        spf = s->win_size * (s->samples / ((s->win_size * sz) * ceil(s->samples / (float)(s->win_size * sz))));
        spf = FFMAX(1, spf);
        s->hop_size = spf;

        spb = (s->samples / (spf * sz)) * spf;

        fin = ff_get_audio_buffer(inlink, spf);
        if (!fin)
            return AVERROR(ENOMEM);

        while (x < sz) {
            int acc_samples = 0;
            int dst_offset = 0;

            while (nb_frame <= s->nb_frames) {
                AVFrame *cur_frame = s->frames[nb_frame];
                int cur_frame_samples = cur_frame->nb_samples;
                int nb_samples = 0;

                if (acc_samples < spf) {
                    nb_samples = FFMIN(spf - acc_samples, cur_frame_samples - src_offset);
                    acc_samples += nb_samples;
                    av_samples_copy(fin->extended_data, cur_frame->extended_data,
                                    dst_offset, src_offset, nb_samples,
                                    cur_frame->ch_layout.nb_channels, AV_SAMPLE_FMT_FLTP);
                }

                src_offset += nb_samples;
                dst_offset += nb_samples;
                if (cur_frame_samples <= src_offset) {
                    av_frame_free(&s->frames[nb_frame]);
                    nb_frame++;
                    src_offset = 0;
                }

                if (acc_samples == spf)
                    break;
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
            draw_legend(ctx, s->samples);

        ret = ff_filter_frame(outlink, s->outpicref);
        s->outpicref = NULL;
    }

    return ret;
}