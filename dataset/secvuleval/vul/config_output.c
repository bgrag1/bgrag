static int config_output(AVFilterLink *outlink)
{
    AVFilterContext *ctx = outlink->src;
    AVFilterLink *inlink = ctx->inputs[0];
    ShowSpectrumContext *s = ctx->priv;
    int i, fft_size, h, w, ret;
    float overlap;

    s->dmax = expf(s->limit * M_LN10 / 20.f);
    s->dmin = expf((s->limit - s->drange) * M_LN10 / 20.f);

    switch (s->fscale) {
    case F_LINEAR: s->plot_channel = plot_channel_lin; break;
    case F_LOG:    s->plot_channel = plot_channel_log; break;
    default: return AVERROR_BUG;
    }

    s->stop = FFMIN(s->stop, inlink->sample_rate / 2);
    if ((s->stop || s->start) && s->stop <= s->start) {
        av_log(ctx, AV_LOG_ERROR, "Stop frequency should be greater than start.\n");
        return AVERROR(EINVAL);
    }

    if (!strcmp(ctx->filter->name, "showspectrumpic"))
        s->single_pic = 1;

    outlink->w = s->w;
    outlink->h = s->h;
    outlink->sample_aspect_ratio = (AVRational){1,1};

    if (s->legend) {
        s->start_x = (log10(inlink->sample_rate) + 1) * 25;
        s->start_y = 64;
        outlink->w += s->start_x * 2;
        outlink->h += s->start_y * 2;
    }

    h = (s->mode == COMBINED || s->orientation == HORIZONTAL) ? s->h : s->h / inlink->channels;
    w = (s->mode == COMBINED || s->orientation == VERTICAL)   ? s->w : s->w / inlink->channels;
    s->channel_height = h;
    s->channel_width  = w;

    if (s->orientation == VERTICAL) {
        /* FFT window size (precision) according to the requested output frame height */
        fft_size = h * 2;
    } else {
        /* FFT window size (precision) according to the requested output frame width */
        fft_size = w * 2;
    }

    s->win_size = fft_size;
    s->buf_size = FFALIGN(s->win_size << (!!s->stop), av_cpu_max_align());

    if (!s->fft) {
        s->fft = av_calloc(inlink->channels, sizeof(*s->fft));
        if (!s->fft)
            return AVERROR(ENOMEM);
    }

    if (s->stop) {
        if (!s->ifft) {
            s->ifft = av_calloc(inlink->channels, sizeof(*s->ifft));
            if (!s->ifft)
                return AVERROR(ENOMEM);
        }
    }

    /* (re-)configuration if the video output changed (or first init) */
    if (fft_size != s->fft_size) {
        AVFrame *outpicref;

        s->fft_size = fft_size;

        /* FFT buffers: x2 for each (display) channel buffer.
         * Note: we use free and malloc instead of a realloc-like function to
         * make sure the buffer is aligned in memory for the FFT functions. */
        for (i = 0; i < s->nb_display_channels; i++) {
            if (s->stop) {
                av_tx_uninit(&s->ifft[i]);
                av_freep(&s->fft_scratch[i]);
            }
            av_tx_uninit(&s->fft[i]);
            av_freep(&s->fft_in[i]);
            av_freep(&s->fft_data[i]);
        }
        av_freep(&s->fft_data);

        s->nb_display_channels = inlink->channels;
        for (i = 0; i < s->nb_display_channels; i++) {
            float scale;

            ret = av_tx_init(&s->fft[i], &s->tx_fn, AV_TX_FLOAT_FFT, 0, fft_size << (!!s->stop), &scale, 0);
            if (s->stop) {
                ret = av_tx_init(&s->ifft[i], &s->itx_fn, AV_TX_FLOAT_FFT, 1, fft_size << (!!s->stop), &scale, 0);
                if (ret < 0) {
                    av_log(ctx, AV_LOG_ERROR, "Unable to create Inverse FFT context. "
                           "The window size might be too high.\n");
                    return ret;
                }
            }
            if (ret < 0) {
                av_log(ctx, AV_LOG_ERROR, "Unable to create FFT context. "
                       "The window size might be too high.\n");
                return ret;
            }
        }

        s->magnitudes = av_calloc(s->nb_display_channels, sizeof(*s->magnitudes));
        if (!s->magnitudes)
            return AVERROR(ENOMEM);
        for (i = 0; i < s->nb_display_channels; i++) {
            s->magnitudes[i] = av_calloc(s->orientation == VERTICAL ? s->h : s->w, sizeof(**s->magnitudes));
            if (!s->magnitudes[i])
                return AVERROR(ENOMEM);
        }

        s->phases = av_calloc(s->nb_display_channels, sizeof(*s->phases));
        if (!s->phases)
            return AVERROR(ENOMEM);
        for (i = 0; i < s->nb_display_channels; i++) {
            s->phases[i] = av_calloc(s->orientation == VERTICAL ? s->h : s->w, sizeof(**s->phases));
            if (!s->phases[i])
                return AVERROR(ENOMEM);
        }

        av_freep(&s->color_buffer);
        s->color_buffer = av_calloc(s->nb_display_channels, sizeof(*s->color_buffer));
        if (!s->color_buffer)
            return AVERROR(ENOMEM);
        for (i = 0; i < s->nb_display_channels; i++) {
            s->color_buffer[i] = av_calloc(s->orientation == VERTICAL ? s->h * 3 : s->w * 3, sizeof(**s->color_buffer));
            if (!s->color_buffer[i])
                return AVERROR(ENOMEM);
        }

        s->fft_in = av_calloc(s->nb_display_channels, sizeof(*s->fft_in));
        if (!s->fft_in)
            return AVERROR(ENOMEM);
        s->fft_data = av_calloc(s->nb_display_channels, sizeof(*s->fft_data));
        if (!s->fft_data)
            return AVERROR(ENOMEM);
        s->fft_scratch = av_calloc(s->nb_display_channels, sizeof(*s->fft_scratch));
        if (!s->fft_scratch)
            return AVERROR(ENOMEM);
        for (i = 0; i < s->nb_display_channels; i++) {
            s->fft_in[i] = av_calloc(s->buf_size, sizeof(**s->fft_in));
            if (!s->fft_in[i])
                return AVERROR(ENOMEM);

            s->fft_data[i] = av_calloc(s->buf_size, sizeof(**s->fft_data));
            if (!s->fft_data[i])
                return AVERROR(ENOMEM);

            s->fft_scratch[i] = av_calloc(s->buf_size, sizeof(**s->fft_scratch));
            if (!s->fft_scratch[i])
                return AVERROR(ENOMEM);
        }

        /* pre-calc windowing function */
        s->window_func_lut =
            av_realloc_f(s->window_func_lut, s->win_size,
                         sizeof(*s->window_func_lut));
        if (!s->window_func_lut)
            return AVERROR(ENOMEM);
        generate_window_func(s->window_func_lut, s->win_size, s->win_func, &overlap);
        if (s->overlap == 1)
            s->overlap = overlap;
        s->hop_size = (1.f - s->overlap) * s->win_size;
        if (s->hop_size < 1) {
            av_log(ctx, AV_LOG_ERROR, "overlap %f too big\n", s->overlap);
            return AVERROR(EINVAL);
        }

        for (s->win_scale = 0, i = 0; i < s->win_size; i++) {
            s->win_scale += s->window_func_lut[i] * s->window_func_lut[i];
        }
        s->win_scale = 1.f / sqrtf(s->win_scale);

        /* prepare the initial picref buffer (black frame) */
        av_frame_free(&s->outpicref);
        s->outpicref = outpicref =
            ff_get_video_buffer(outlink, outlink->w, outlink->h);
        if (!outpicref)
            return AVERROR(ENOMEM);
        outpicref->sample_aspect_ratio = (AVRational){1,1};
        for (i = 0; i < outlink->h; i++) {
            memset(outpicref->data[0] + i * outpicref->linesize[0],   0, outlink->w);
            memset(outpicref->data[1] + i * outpicref->linesize[1], 128, outlink->w);
            memset(outpicref->data[2] + i * outpicref->linesize[2], 128, outlink->w);
        }
        outpicref->color_range = AVCOL_RANGE_JPEG;

        if (!s->single_pic && s->legend)
            draw_legend(ctx, 0);
    }

    if ((s->orientation == VERTICAL   && s->xpos >= s->w) ||
        (s->orientation == HORIZONTAL && s->xpos >= s->h))
        s->xpos = 0;

    if (s->sliding == LREPLACE) {
        if (s->orientation == VERTICAL)
            s->xpos = s->w - 1;
        if (s->orientation == HORIZONTAL)
            s->xpos = s->h - 1;
    }

    s->auto_frame_rate = av_make_q(inlink->sample_rate, s->hop_size);
    if (s->orientation == VERTICAL && s->sliding == FULLFRAME)
        s->auto_frame_rate = av_mul_q(s->auto_frame_rate, av_make_q(1, s->w));
    if (s->orientation == HORIZONTAL && s->sliding == FULLFRAME)
        s->auto_frame_rate = av_mul_q(s->auto_frame_rate, av_make_q(1, s->h));
    if (!s->single_pic && strcmp(s->rate_str, "auto")) {
        int ret = av_parse_video_rate(&s->frame_rate, s->rate_str);
        if (ret < 0)
            return ret;
    } else {
        s->frame_rate = s->auto_frame_rate;
    }
    outlink->frame_rate = s->frame_rate;
    outlink->time_base = av_inv_q(outlink->frame_rate);

    if (s->orientation == VERTICAL) {
        s->combine_buffer =
            av_realloc_f(s->combine_buffer, s->h * 3,
                         sizeof(*s->combine_buffer));
    } else {
        s->combine_buffer =
            av_realloc_f(s->combine_buffer, s->w * 3,
                         sizeof(*s->combine_buffer));
    }

    av_log(ctx, AV_LOG_VERBOSE, "s:%dx%d FFT window size:%d\n",
           s->w, s->h, s->win_size);

    av_audio_fifo_free(s->fifo);
    s->fifo = av_audio_fifo_alloc(inlink->format, inlink->channels, s->win_size);
    if (!s->fifo)
        return AVERROR(ENOMEM);
    return 0;
}