static int run_channel_fft(AVFilterContext *ctx, void *arg, int jobnr, int nb_jobs)
{
    ShowSpectrumContext *s = ctx->priv;
    AVFilterLink *inlink = ctx->inputs[0];
    const float *window_func_lut = s->window_func_lut;
    AVFrame *fin = arg;
    const int ch = jobnr;
    int n;

    /* fill FFT input with the number of samples available */
    const float *p = (float *)fin->extended_data[ch];
    float *in_frame = (float *)s->in_frame->extended_data[ch];

    memmove(in_frame, in_frame + s->hop_size, (s->fft_size - s->hop_size) * sizeof(float));
    memcpy(in_frame + s->fft_size - s->hop_size, p, fin->nb_samples * sizeof(float));

    for (int i = fin->nb_samples; i < s->hop_size; i++)
        in_frame[i + s->fft_size - s->hop_size] = 0.f;

    if (s->stop) {
        float theta, phi, psi, a, b, S, c;
        AVComplexFloat *f = s->fft_in[ch];
        AVComplexFloat *g = s->fft_data[ch];
        AVComplexFloat *h = s->fft_scratch[ch];
        int L = s->buf_size;
        int N = s->win_size;
        int M = s->win_size / 2;

        for (n = 0; n < s->win_size; n++) {
            s->fft_data[ch][n].re = in_frame[n] * window_func_lut[n];
            s->fft_data[ch][n].im = 0;
        }

        phi = 2.f * M_PI * (s->stop - s->start) / (float)inlink->sample_rate / (M - 1);
        theta = 2.f * M_PI * s->start / (float)inlink->sample_rate;

        for (int n = 0; n < M; n++) {
            h[n].re = cosf(n * n / 2.f * phi);
            h[n].im = sinf(n * n / 2.f * phi);
        }

        for (int n = M; n < L; n++) {
            h[n].re = 0.f;
            h[n].im = 0.f;
        }

        for (int n = L - N; n < L; n++) {
            h[n].re = cosf((L - n) * (L - n) / 2.f * phi);
            h[n].im = sinf((L - n) * (L - n) / 2.f * phi);
        }

        for (int n = N; n < L; n++) {
            g[n].re = 0.f;
            g[n].im = 0.f;
        }

        for (int n = 0; n < N; n++) {
            psi = n * theta + n * n / 2.f * phi;
            c =  cosf(psi);
            S = -sinf(psi);
            a = c * g[n].re - S * g[n].im;
            b = S * g[n].re + c * g[n].im;
            g[n].re = a;
            g[n].im = b;
        }

        memcpy(f, h, s->buf_size * sizeof(*f));
        s->tx_fn(s->fft[ch], h, f, sizeof(float));

        memcpy(f, g, s->buf_size * sizeof(*f));
        s->tx_fn(s->fft[ch], g, f, sizeof(float));

        for (int n = 0; n < L; n++) {
            c = g[n].re;
            S = g[n].im;
            a = c * h[n].re - S * h[n].im;
            b = S * h[n].re + c * h[n].im;

            g[n].re = a / L;
            g[n].im = b / L;
        }

        memcpy(f, g, s->buf_size * sizeof(*f));
        s->itx_fn(s->ifft[ch], g, f, sizeof(float));

        for (int k = 0; k < M; k++) {
            psi = k * k / 2.f * phi;
            c =  cosf(psi);
            S = -sinf(psi);
            a = c * g[k].re - S * g[k].im;
            b = S * g[k].re + c * g[k].im;
            s->fft_data[ch][k].re = a;
            s->fft_data[ch][k].im = b;
        }
    } else {
        for (n = 0; n < s->win_size; n++) {
            s->fft_in[ch][n].re = in_frame[n] * window_func_lut[n];
            s->fft_in[ch][n].im = 0;
        }

        /* run FFT on each samples set */
        s->tx_fn(s->fft[ch], s->fft_data[ch], s->fft_in[ch], sizeof(float));
    }

    return 0;
}