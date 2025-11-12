static int tda10048_set_if(struct dvb_frontend *fe, u32 bw)
{
	struct tda10048_state *state = fe->demodulator_priv;
	struct tda10048_config *config = &state->config;
	int i;
	u32 if_freq_khz;
	u64 sample_freq;

	dprintk(1, "%s(bw = %d)\n", __func__, bw);

	/* based on target bandwidth and clk we calculate pll factors */
	switch (bw) {
	case 6000000:
		if_freq_khz = config->dtv6_if_freq_khz;
		break;
	case 7000000:
		if_freq_khz = config->dtv7_if_freq_khz;
		break;
	case 8000000:
		if_freq_khz = config->dtv8_if_freq_khz;
		break;
	default:
		printk(KERN_ERR "%s() no default\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(pll_tab); i++) {
		if ((pll_tab[i].clk_freq_khz == config->clk_freq_khz) &&
			(pll_tab[i].if_freq_khz == if_freq_khz)) {

			state->freq_if_hz = pll_tab[i].if_freq_khz * 1000;
			state->xtal_hz = pll_tab[i].clk_freq_khz * 1000;
			break;
		}
	}
	if (i == ARRAY_SIZE(pll_tab)) {
		printk(KERN_ERR "%s() Incorrect attach settings\n",
			__func__);
		return -EINVAL;
	}

	dprintk(1, "- freq_if_hz = %d\n", state->freq_if_hz);
	dprintk(1, "- xtal_hz = %d\n", state->xtal_hz);
	dprintk(1, "- pll_mfactor = %d\n", state->pll_mfactor);
	dprintk(1, "- pll_nfactor = %d\n", state->pll_nfactor);
	dprintk(1, "- pll_pfactor = %d\n", state->pll_pfactor);

	/* Calculate the sample frequency */
	sample_freq = state->xtal_hz;
	sample_freq *= state->pll_mfactor + 45;
	do_div(sample_freq, state->pll_nfactor + 1);
	do_div(sample_freq, state->pll_pfactor + 4);
	state->sample_freq = sample_freq;
	dprintk(1, "- sample_freq = %d\n", state->sample_freq);

	/* Update the I/F */
	tda10048_set_phy2(fe, state->sample_freq, state->freq_if_hz);

	return 0;
}
