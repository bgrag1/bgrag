static int activate(AVFilterContext *ctx)
{
    AVFilterLink *inlink = ctx->inputs[0];
    AVFilterLink *outlink = ctx->outputs[0];
    ShowSpectrumContext *s = ctx->priv;
    int ret;

    FF_FILTER_FORWARD_STATUS_BACK(outlink, inlink);

    if (av_audio_fifo_size(s->fifo) < s->win_size) {
        AVFrame *frame = NULL;

        ret = ff_inlink_consume_frame(inlink, &frame);
        if (ret < 0)
            return ret;
        if (ret > 0) {
            s->pts = frame->pts;
            s->consumed = 0;

            av_audio_fifo_write(s->fifo, (void **)frame->extended_data, frame->nb_samples);
            av_frame_free(&frame);
        }
    }

    if (s->outpicref && (av_audio_fifo_size(s->fifo) >= s->win_size ||
        ff_outlink_get_status(inlink))) {
        AVFrame *fin = ff_get_audio_buffer(inlink, s->win_size);
        if (!fin)
            return AVERROR(ENOMEM);

        fin->pts = s->pts + s->consumed;
        s->consumed += s->hop_size;
        ret = av_audio_fifo_peek(s->fifo, (void **)fin->extended_data,
                                 FFMIN(s->win_size, av_audio_fifo_size(s->fifo)));
        if (ret < 0) {
            av_frame_free(&fin);
            return ret;
        }

        av_assert0(fin->nb_samples == s->win_size);

        ff_filter_execute(ctx, run_channel_fft, fin, NULL, s->nb_display_channels);

        if (s->data == D_MAGNITUDE)
            ff_filter_execute(ctx, calc_channel_magnitudes, NULL, NULL, s->nb_display_channels);

        if (s->data == D_PHASE)
            ff_filter_execute(ctx, calc_channel_phases, NULL, NULL, s->nb_display_channels);

        if (s->data == D_UPHASE)
            ff_filter_execute(ctx, calc_channel_uphases, NULL, NULL, s->nb_display_channels);

        ret = plot_spectrum_column(inlink, fin);

        av_frame_free(&fin);
        av_audio_fifo_drain(s->fifo, s->hop_size);
        if (ret <= 0 && !ff_outlink_get_status(inlink))
            return ret;
    }

    if (ff_outlink_get_status(inlink) == AVERROR_EOF &&
        s->sliding == FULLFRAME &&
        s->xpos > 0 && s->outpicref) {
        int64_t pts;

        if (s->orientation == VERTICAL) {
            for (int i = 0; i < outlink->h; i++) {
                memset(s->outpicref->data[0] + i * s->outpicref->linesize[0] + s->xpos,   0, outlink->w - s->xpos);
                memset(s->outpicref->data[1] + i * s->outpicref->linesize[1] + s->xpos, 128, outlink->w - s->xpos);
                memset(s->outpicref->data[2] + i * s->outpicref->linesize[2] + s->xpos, 128, outlink->w - s->xpos);
            }
        } else {
            for (int i = s->xpos; i < outlink->h; i++) {
                memset(s->outpicref->data[0] + i * s->outpicref->linesize[0],   0, outlink->w);
                memset(s->outpicref->data[1] + i * s->outpicref->linesize[1], 128, outlink->w);
                memset(s->outpicref->data[2] + i * s->outpicref->linesize[2], 128, outlink->w);
            }
        }
        s->outpicref->pts += av_rescale_q(s->consumed, inlink->time_base, outlink->time_base);
        pts = s->outpicref->pts;
        ret = ff_filter_frame(outlink, s->outpicref);
        s->outpicref = NULL;
        ff_outlink_set_status(outlink, AVERROR_EOF, pts);
        return 0;
    }

    FF_FILTER_FORWARD_STATUS(inlink, outlink);
    if (av_audio_fifo_size(s->fifo) >= s->win_size ||
        ff_inlink_queued_frames(inlink) > 0 ||
        ff_outlink_get_status(inlink) == AVERROR_EOF) {
        ff_filter_set_ready(ctx, 10);
        return 0;
    }

    if (ff_outlink_frame_wanted(outlink) && av_audio_fifo_size(s->fifo) < s->win_size &&
        ff_inlink_queued_frames(inlink) == 0 &&
        ff_outlink_get_status(inlink) != AVERROR_EOF) {
        ff_inlink_request_frame(inlink);
        return 0;
    }

    return FFERROR_NOT_READY;
}