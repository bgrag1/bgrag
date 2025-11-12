static int axg_card_add_tdm_loopback(struct snd_soc_card *card,
				     int *index)
{
	struct meson_card *priv = snd_soc_card_get_drvdata(card);
	struct snd_soc_dai_link *pad = &card->dai_link[*index];
	struct snd_soc_dai_link *lb;
	struct snd_soc_dai_link_component *dlc;
	int ret;

	/* extend links */
	ret = meson_card_reallocate_links(card, card->num_links + 1);
	if (ret)
		return ret;

	lb = &card->dai_link[*index + 1];

	lb->name = devm_kasprintf(card->dev, GFP_KERNEL, "%s-lb", pad->name);
	if (!lb->name)
		return -ENOMEM;

	dlc = devm_kzalloc(card->dev, sizeof(*dlc), GFP_KERNEL);
	if (!dlc)
		return -ENOMEM;

	lb->cpus = dlc;
	lb->codecs = &snd_soc_dummy_dlc;
	lb->num_cpus = 1;
	lb->num_codecs = 1;

	lb->stream_name = lb->name;
	lb->cpus->of_node = pad->cpus->of_node;
	lb->cpus->dai_name = "TDM Loopback";
	lb->dpcm_capture = 1;
	lb->no_pcm = 1;
	lb->ops = &axg_card_tdm_be_ops;
	lb->init = axg_card_tdm_dai_lb_init;

	/* Provide the same link data to the loopback */
	priv->link_data[*index + 1] = priv->link_data[*index];

	/*
	 * axg_card_clean_references() will iterate over this link,
	 * make sure the node count is balanced
	 */
	of_node_get(lb->cpus->of_node);

	/* Let add_links continue where it should */
	*index += 1;

	return 0;
}
