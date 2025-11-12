static int plot_spectrum_column(AVFilterLink *inlink, AVFrame *insamples)
{
    AVFilterContext *ctx = inlink->dst;
    AVFilterLink *outlink = ctx->outputs[0];
    ShowSpectrumContext *s = ctx->priv;
    AVFrame *outpicref = s->outpicref;
    int ret, plane, x, y, z = s->orientation == VERTICAL ? s->h : s->w;

    /* fill a new spectrum column */
    /* initialize buffer for combining to black */
    clear_combine_buffer(s, z);

    ff_filter_execute(ctx, s->plot_channel, NULL, NULL, s->nb_display_channels);

    for (y = 0; y < z * 3; y++) {
        for (x = 0; x < s->nb_display_channels; x++) {
            s->combine_buffer[y] += s->color_buffer[x][y];
        }
    }

    av_frame_make_writable(s->outpicref);
    /* copy to output */
    if (s->orientation == VERTICAL) {
        if (s->sliding == SCROLL) {
            for (plane = 0; plane < 3; plane++) {
                for (y = 0; y < s->h; y++) {
                    uint8_t *p = outpicref->data[plane] + s->start_x +
                                 (y + s->start_y) * outpicref->linesize[plane];
                    memmove(p, p + 1, s->w - 1);
                }
            }
            s->xpos = s->w - 1;
        } else if (s->sliding == RSCROLL) {
            for (plane = 0; plane < 3; plane++) {
                for (y = 0; y < s->h; y++) {
                    uint8_t *p = outpicref->data[plane] + s->start_x +
                                 (y + s->start_y) * outpicref->linesize[plane];
                    memmove(p + 1, p, s->w - 1);
                }
            }
            s->xpos = 0;
        }
        for (plane = 0; plane < 3; plane++) {
            uint8_t *p = outpicref->data[plane] + s->start_x +
                         (outlink->h - 1 - s->start_y) * outpicref->linesize[plane] +
                         s->xpos;
            for (y = 0; y < s->h; y++) {
                *p = lrintf(av_clipf(s->combine_buffer[3 * y + plane], 0, 255));
                p -= outpicref->linesize[plane];
            }
        }
    } else {
        if (s->sliding == SCROLL) {
            for (plane = 0; plane < 3; plane++) {
                for (y = 1; y < s->h; y++) {
                    memmove(outpicref->data[plane] + (y-1 + s->start_y) * outpicref->linesize[plane] + s->start_x,
                            outpicref->data[plane] + (y   + s->start_y) * outpicref->linesize[plane] + s->start_x,
                            s->w);
                }
            }
            s->xpos = s->h - 1;
        } else if (s->sliding == RSCROLL) {
            for (plane = 0; plane < 3; plane++) {
                for (y = s->h - 1; y >= 1; y--) {
                    memmove(outpicref->data[plane] + (y   + s->start_y) * outpicref->linesize[plane] + s->start_x,
                            outpicref->data[plane] + (y-1 + s->start_y) * outpicref->linesize[plane] + s->start_x,
                            s->w);
                }
            }
            s->xpos = 0;
        }
        for (plane = 0; plane < 3; plane++) {
            uint8_t *p = outpicref->data[plane] + s->start_x +
                         (s->xpos + s->start_y) * outpicref->linesize[plane];
            for (x = 0; x < s->w; x++) {
                *p = lrintf(av_clipf(s->combine_buffer[3 * x + plane], 0, 255));
                p++;
            }
        }
    }

    if (s->sliding != FULLFRAME || s->xpos == 0)
        outpicref->pts = av_rescale_q(insamples->pts, inlink->time_base, outlink->time_base);

    if (s->sliding == LREPLACE) {
        s->xpos--;
        if (s->orientation == VERTICAL && s->xpos < 0)
            s->xpos = s->w - 1;
        if (s->orientation == HORIZONTAL && s->xpos < 0)
            s->xpos = s->h - 1;
    } else {
        s->xpos++;
        if (s->orientation == VERTICAL && s->xpos >= s->w)
            s->xpos = 0;
        if (s->orientation == HORIZONTAL && s->xpos >= s->h)
            s->xpos = 0;
    }

    if (!s->single_pic && (s->sliding != FULLFRAME || s->xpos == 0)) {
        if (s->old_pts < outpicref->pts) {
            AVFrame *clone;

            if (s->legend) {
                char *units = get_time(ctx, insamples->pts /(float)inlink->sample_rate, x);
                if (!units)
                    return AVERROR(ENOMEM);

                if (s->orientation == VERTICAL) {
                    for (y = 0; y < 10; y++) {
                        memset(s->outpicref->data[0] + outlink->w / 2 - 4 * s->old_len +
                               (outlink->h - s->start_y / 2 - 20 + y) * s->outpicref->linesize[0], 0, 10 * s->old_len);
                    }
                    drawtext(s->outpicref,
                             outlink->w / 2 - 4 * strlen(units),
                             outlink->h - s->start_y / 2 - 20,
                             units, 0);
                } else  {
                    for (y = 0; y < 10 * s->old_len; y++) {
                        memset(s->outpicref->data[0] + s->start_x / 7 + 20 +
                               (outlink->h / 2 - 4 * s->old_len + y) * s->outpicref->linesize[0], 0, 10);
                    }
                    drawtext(s->outpicref,
                             s->start_x / 7 + 20,
                             outlink->h / 2 - 4 * strlen(units),
                             units, 1);
                }
                s->old_len = strlen(units);
                av_free(units);
            }
            s->old_pts = outpicref->pts;
            clone = av_frame_clone(s->outpicref);
            if (!clone)
                return AVERROR(ENOMEM);
            ret = ff_filter_frame(outlink, clone);
            if (ret < 0)
                return ret;
            return 0;
        }
    }

    return 1;
}