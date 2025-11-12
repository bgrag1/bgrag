static int draw_legend(AVFilterContext *ctx, int samples)
{
    ShowSpectrumContext *s = ctx->priv;
    AVFilterLink *inlink = ctx->inputs[0];
    AVFilterLink *outlink = ctx->outputs[0];
    int ch, y, x = 0, sz = s->orientation == VERTICAL ? s->w : s->h;
    int multi = (s->mode == SEPARATE && s->color_mode == CHANNEL);
    float spp = samples / (float)sz;
    char *text;
    uint8_t *dst;
    char chlayout_str[128];

    av_get_channel_layout_string(chlayout_str, sizeof(chlayout_str), inlink->channels,
                                 inlink->channel_layout);

    text = av_asprintf("%d Hz | %s", inlink->sample_rate, chlayout_str);
    if (!text)
        return AVERROR(ENOMEM);

    drawtext(s->outpicref, 2, outlink->h - 10, "CREATED BY LIBAVFILTER", 0);
    drawtext(s->outpicref, outlink->w - 2 - strlen(text) * 10, outlink->h - 10, text, 0);
    av_freep(&text);
    if (s->stop) {
        text = av_asprintf("Zoom: %d Hz - %d Hz", s->start, s->stop);
        if (!text)
            return AVERROR(ENOMEM);
        drawtext(s->outpicref, outlink->w - 2 - strlen(text) * 10, 3, text, 0);
        av_freep(&text);
    }

    dst = s->outpicref->data[0] + (s->start_y - 1) * s->outpicref->linesize[0] + s->start_x - 1;
    for (x = 0; x < s->w + 1; x++)
        dst[x] = 200;
    dst = s->outpicref->data[0] + (s->start_y + s->h) * s->outpicref->linesize[0] + s->start_x - 1;
    for (x = 0; x < s->w + 1; x++)
        dst[x] = 200;
    for (y = 0; y < s->h + 2; y++) {
        dst = s->outpicref->data[0] + (y + s->start_y - 1) * s->outpicref->linesize[0];
        dst[s->start_x - 1] = 200;
        dst[s->start_x + s->w] = 200;
    }
    if (s->orientation == VERTICAL) {
        int h = s->mode == SEPARATE ? s->h / s->nb_display_channels : s->h;
        int hh = s->mode == SEPARATE ? -(s->h % s->nb_display_channels) + 1 : 1;
        for (ch = 0; ch < (s->mode == SEPARATE ? s->nb_display_channels : 1); ch++) {
            for (y = 0; y < h; y += 20) {
                dst = s->outpicref->data[0] + (s->start_y + h * (ch + 1) - y - hh) * s->outpicref->linesize[0];
                dst[s->start_x - 2] = 200;
                dst[s->start_x + s->w + 1] = 200;
            }
            for (y = 0; y < h; y += 40) {
                dst = s->outpicref->data[0] + (s->start_y + h * (ch + 1) - y - hh) * s->outpicref->linesize[0];
                dst[s->start_x - 3] = 200;
                dst[s->start_x + s->w + 2] = 200;
            }
            dst = s->outpicref->data[0] + (s->start_y - 2) * s->outpicref->linesize[0] + s->start_x;
            for (x = 0; x < s->w; x+=40)
                dst[x] = 200;
            dst = s->outpicref->data[0] + (s->start_y - 3) * s->outpicref->linesize[0] + s->start_x;
            for (x = 0; x < s->w; x+=80)
                dst[x] = 200;
            dst = s->outpicref->data[0] + (s->h + s->start_y + 1) * s->outpicref->linesize[0] + s->start_x;
            for (x = 0; x < s->w; x+=40) {
                dst[x] = 200;
            }
            dst = s->outpicref->data[0] + (s->h + s->start_y + 2) * s->outpicref->linesize[0] + s->start_x;
            for (x = 0; x < s->w; x+=80) {
                dst[x] = 200;
            }
            for (y = 0; y < h; y += 40) {
                float range = s->stop ? s->stop - s->start : inlink->sample_rate / 2;
                float hertz = get_hz(y, h, s->start, s->start + range, s->fscale);
                char *units;

                if (hertz == 0)
                    units = av_asprintf("DC");
                else
                    units = av_asprintf("%.2f", hertz);
                if (!units)
                    return AVERROR(ENOMEM);

                drawtext(s->outpicref, s->start_x - 8 * strlen(units) - 4, h * (ch + 1) + s->start_y - y - 4 - hh, units, 0);
                av_free(units);
            }
        }

        for (x = 0; x < s->w && s->single_pic; x+=80) {
            float seconds = x * spp / inlink->sample_rate;
            char *units = get_time(ctx, seconds, x);
            if (!units)
                return AVERROR(ENOMEM);

            drawtext(s->outpicref, s->start_x + x - 4 * strlen(units), s->h + s->start_y + 6, units, 0);
            drawtext(s->outpicref, s->start_x + x - 4 * strlen(units), s->start_y - 12, units, 0);
            av_free(units);
        }

        drawtext(s->outpicref, outlink->w / 2 - 4 * 4, outlink->h - s->start_y / 2, "TIME", 0);
        drawtext(s->outpicref, s->start_x / 7, outlink->h / 2 - 14 * 4, "FREQUENCY (Hz)", 1);
    } else {
        int w = s->mode == SEPARATE ? s->w / s->nb_display_channels : s->w;
        for (y = 0; y < s->h; y += 20) {
            dst = s->outpicref->data[0] + (s->start_y + y) * s->outpicref->linesize[0];
            dst[s->start_x - 2] = 200;
            dst[s->start_x + s->w + 1] = 200;
        }
        for (y = 0; y < s->h; y += 40) {
            dst = s->outpicref->data[0] + (s->start_y + y) * s->outpicref->linesize[0];
            dst[s->start_x - 3] = 200;
            dst[s->start_x + s->w + 2] = 200;
        }
        for (ch = 0; ch < (s->mode == SEPARATE ? s->nb_display_channels : 1); ch++) {
            dst = s->outpicref->data[0] + (s->start_y - 2) * s->outpicref->linesize[0] + s->start_x + w * ch;
            for (x = 0; x < w; x+=40)
                dst[x] = 200;
            dst = s->outpicref->data[0] + (s->start_y - 3) * s->outpicref->linesize[0] + s->start_x + w * ch;
            for (x = 0; x < w; x+=80)
                dst[x] = 200;
            dst = s->outpicref->data[0] + (s->h + s->start_y + 1) * s->outpicref->linesize[0] + s->start_x + w * ch;
            for (x = 0; x < w; x+=40) {
                dst[x] = 200;
            }
            dst = s->outpicref->data[0] + (s->h + s->start_y + 2) * s->outpicref->linesize[0] + s->start_x + w * ch;
            for (x = 0; x < w; x+=80) {
                dst[x] = 200;
            }
            for (x = 0; x < w - 79; x += 80) {
                float range = s->stop ? s->stop - s->start : inlink->sample_rate / 2;
                float hertz = get_hz(x, w, s->start, s->start + range, s->fscale);
                char *units;

                if (hertz == 0)
                    units = av_asprintf("DC");
                else
                    units = av_asprintf("%.2f", hertz);
                if (!units)
                    return AVERROR(ENOMEM);

                drawtext(s->outpicref, s->start_x - 4 * strlen(units) + x + w * ch, s->start_y - 12, units, 0);
                drawtext(s->outpicref, s->start_x - 4 * strlen(units) + x + w * ch, s->h + s->start_y + 6, units, 0);
                av_free(units);
            }
        }
        for (y = 0; y < s->h && s->single_pic; y+=40) {
            float seconds = y * spp / inlink->sample_rate;
            char *units = get_time(ctx, seconds, x);
            if (!units)
                return AVERROR(ENOMEM);

            drawtext(s->outpicref, s->start_x - 8 * strlen(units) - 4, s->start_y + y - 4, units, 0);
            av_free(units);
        }
        drawtext(s->outpicref, s->start_x / 7, outlink->h / 2 - 4 * 4, "TIME", 1);
        drawtext(s->outpicref, outlink->w / 2 - 14 * 4, outlink->h - s->start_y / 2, "FREQUENCY (Hz)", 0);
    }

    for (ch = 0; ch < (multi ? s->nb_display_channels : 1); ch++) {
        int h = multi ? s->h / s->nb_display_channels : s->h;

        for (y = 0; y < h; y++) {
            float out[3] = { 0., 127.5, 127.5};
            int chn;

            for (chn = 0; chn < (s->mode == SEPARATE ? 1 : s->nb_display_channels); chn++) {
                float yf, uf, vf;
                int channel = (multi) ? s->nb_display_channels - ch - 1 : chn;
                float lout[3];

                color_range(s, channel, &yf, &uf, &vf);
                pick_color(s, yf, uf, vf, y / (float)h, lout);
                out[0] += lout[0];
                out[1] += lout[1];
                out[2] += lout[2];
            }
            memset(s->outpicref->data[0]+(s->start_y + h * (ch + 1) - y - 1) * s->outpicref->linesize[0] + s->w + s->start_x + 20, av_clip_uint8(out[0]), 10);
            memset(s->outpicref->data[1]+(s->start_y + h * (ch + 1) - y - 1) * s->outpicref->linesize[1] + s->w + s->start_x + 20, av_clip_uint8(out[1]), 10);
            memset(s->outpicref->data[2]+(s->start_y + h * (ch + 1) - y - 1) * s->outpicref->linesize[2] + s->w + s->start_x + 20, av_clip_uint8(out[2]), 10);
        }

        for (y = 0; ch == 0 && y < h + 5; y += 25) {
            static const char *log_fmt = "%.0f";
            static const char *lin_fmt = "%.3f";
            const float a = av_clipf(1.f - y / (float)(h - 1), 0.f, 1.f);
            const float value = s->scale == LOG ? log10f(get_iscale(ctx, s->scale, a)) * 20.f : get_iscale(ctx, s->scale, a);
            char *text;

            text = av_asprintf(s->scale == LOG ? log_fmt : lin_fmt, value);
            if (!text)
                continue;
            drawtext(s->outpicref, s->w + s->start_x + 35, s->start_y + y - 3, text, 0);
            av_free(text);
        }
    }

    if (s->scale == LOG)
        drawtext(s->outpicref, s->w + s->start_x + 22, s->start_y + s->h + 20, "dBFS", 0);

    return 0;
}