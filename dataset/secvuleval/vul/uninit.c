static av_cold void uninit(AVFilterContext *ctx)
{
    ShowSpectrumContext *s = ctx->priv;
    int i;

    av_freep(&s->combine_buffer);
    if (s->fft) {
        for (i = 0; i < s->nb_display_channels; i++)
            av_tx_uninit(&s->fft[i]);
    }
    av_freep(&s->fft);
    if (s->ifft) {
        for (i = 0; i < s->nb_display_channels; i++)
            av_tx_uninit(&s->ifft[i]);
    }
    av_freep(&s->ifft);
    if (s->fft_data) {
        for (i = 0; i < s->nb_display_channels; i++)
            av_freep(&s->fft_data[i]);
    }
    av_freep(&s->fft_data);
    if (s->fft_in) {
        for (i = 0; i < s->nb_display_channels; i++)
            av_freep(&s->fft_in[i]);
    }
    av_freep(&s->fft_in);
    if (s->fft_scratch) {
        for (i = 0; i < s->nb_display_channels; i++)
            av_freep(&s->fft_scratch[i]);
    }
    av_freep(&s->fft_scratch);
    if (s->color_buffer) {
        for (i = 0; i < s->nb_display_channels; i++)
            av_freep(&s->color_buffer[i]);
    }
    av_freep(&s->color_buffer);
    av_freep(&s->window_func_lut);
    if (s->magnitudes) {
        for (i = 0; i < s->nb_display_channels; i++)
            av_freep(&s->magnitudes[i]);
    }
    av_freep(&s->magnitudes);
    av_frame_free(&s->outpicref);
    av_audio_fifo_free(s->fifo);
    if (s->phases) {
        for (i = 0; i < s->nb_display_channels; i++)
            av_freep(&s->phases[i]);
    }
    av_freep(&s->phases);
}