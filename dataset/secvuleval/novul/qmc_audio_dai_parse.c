static int qmc_audio_dai_parse(struct qmc_audio *qmc_audio, struct device_node *np,
	struct qmc_dai *qmc_dai, struct snd_soc_dai_driver *qmc_soc_dai_driver)
{
	struct qmc_chan_info info;
	u32 val;
	int ret;

	qmc_dai->dev = qmc_audio->dev;

	ret = of_property_read_u32(np, "reg", &val);
	if (ret) {
		dev_err(qmc_audio->dev, "%pOF: failed to read reg\n", np);
		return ret;
	}
	qmc_dai->id = val;

	qmc_dai->name = devm_kasprintf(qmc_audio->dev, GFP_KERNEL, "%s.%d",
				       np->parent->name, qmc_dai->id);
	if (!qmc_dai->name)
		return -ENOMEM;

	qmc_dai->qmc_chan = devm_qmc_chan_get_byphandle(qmc_audio->dev, np,
							"fsl,qmc-chan");
	if (IS_ERR(qmc_dai->qmc_chan)) {
		ret = PTR_ERR(qmc_dai->qmc_chan);
		return dev_err_probe(qmc_audio->dev, ret,
				     "dai %d get QMC channel failed\n", qmc_dai->id);
	}

	qmc_soc_dai_driver->id = qmc_dai->id;
	qmc_soc_dai_driver->name = qmc_dai->name;

	ret = qmc_chan_get_info(qmc_dai->qmc_chan, &info);
	if (ret) {
		dev_err(qmc_audio->dev, "dai %d get QMC channel info failed %d\n",
			qmc_dai->id, ret);
		return ret;
	}
	dev_info(qmc_audio->dev, "dai %d QMC channel mode %d, nb_tx_ts %u, nb_rx_ts %u\n",
		 qmc_dai->id, info.mode, info.nb_tx_ts, info.nb_rx_ts);

	if (info.mode != QMC_TRANSPARENT) {
		dev_err(qmc_audio->dev, "dai %d QMC chan mode %d is not QMC_TRANSPARENT\n",
			qmc_dai->id, info.mode);
		return -EINVAL;
	}
	qmc_dai->nb_tx_ts = info.nb_tx_ts;
	qmc_dai->nb_rx_ts = info.nb_rx_ts;

	qmc_soc_dai_driver->playback.channels_min = 0;
	qmc_soc_dai_driver->playback.channels_max = 0;
	if (qmc_dai->nb_tx_ts) {
		qmc_soc_dai_driver->playback.channels_min = 1;
		qmc_soc_dai_driver->playback.channels_max = qmc_dai->nb_tx_ts;
	}
	qmc_soc_dai_driver->playback.formats = qmc_audio_formats(qmc_dai->nb_tx_ts);

	qmc_soc_dai_driver->capture.channels_min = 0;
	qmc_soc_dai_driver->capture.channels_max = 0;
	if (qmc_dai->nb_rx_ts) {
		qmc_soc_dai_driver->capture.channels_min = 1;
		qmc_soc_dai_driver->capture.channels_max = qmc_dai->nb_rx_ts;
	}
	qmc_soc_dai_driver->capture.formats = qmc_audio_formats(qmc_dai->nb_rx_ts);

	qmc_soc_dai_driver->playback.rates = snd_pcm_rate_to_rate_bit(info.tx_fs_rate);
	qmc_soc_dai_driver->playback.rate_min = info.tx_fs_rate;
	qmc_soc_dai_driver->playback.rate_max = info.tx_fs_rate;
	qmc_soc_dai_driver->capture.rates = snd_pcm_rate_to_rate_bit(info.rx_fs_rate);
	qmc_soc_dai_driver->capture.rate_min = info.rx_fs_rate;
	qmc_soc_dai_driver->capture.rate_max = info.rx_fs_rate;

	qmc_soc_dai_driver->ops = &qmc_dai_ops;

	return 0;
}
