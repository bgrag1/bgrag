static int activate(AVFilterContext *ctx)
{
    AVFilterLink *inlink = ctx->inputs[0];
    AVFilterLink *outlink = ctx->outputs[0];
    ShowSpectrumContext *s = ctx->priv;
    int ret, status;
    int64_t pts;

    FF_FILTER_FORWARD_STATUS_BACK(outlink, inlink);

    if (s->outpicref) {
        AVFrame *fin;

        ret = ff_inlink_consume_samples(inlink, s->hop_size, s->hop_size, &fin);
        if (ret < 0)
            return ret;
        if (ret > 0) {
            s->consumed += fin->nb_samples;
            ff_filter_execute(ctx, run_channel_fft, fin, NULL, s->nb_display_channels);

            if (s->data == D_MAGNITUDE)
                ff_filter_execute(ctx, calc_channel_magnitudes, NULL, NULL, s->nb_display_channels);

            if (s->data == D_PHASE)
                ff_filter_execute(ctx, calc_channel_phases, NULL, NULL, s->nb_display_channels);

            if (s->data == D_UPHASE)
                ff_filter_execute(ctx, calc_channel_uphases, NULL, NULL, s->nb_display_channels);

            ret = plot_spectrum_column(inlink, fin);
            av_frame_free(&fin);
            if (ret <= 0)
                return ret;
        }
    }

    if (ff_outlink_get_status(inlink) == AVERROR_EOF &&
        s->sliding == FULLFRAME &&
        s->xpos > 0 && s->outpicref) {

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

    if (ff_inlink_acknowledge_status(inlink, &status, &pts)) {
        if (status == AVERROR_EOF) {
            ff_outlink_set_status(outlink, status, s->pts);
            return 0;
        }
    }

    if (ff_inlink_queued_samples(inlink) >= s->hop_size) {
        ff_filter_set_ready(ctx, 10);
        return 0;
    }

    if (ff_outlink_frame_wanted(outlink)) {
        ff_inlink_request_frame(inlink);
        return 0;
    }

    return FFERROR_NOT_READY;
}