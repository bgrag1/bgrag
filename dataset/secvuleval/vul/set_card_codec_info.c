static int set_card_codec_info(struct snd_soc_card *card,
			       struct device_node *sub_node,
			       struct snd_soc_dai_link *dai_link)
{
	struct device *dev = card->dev;
	struct device_node *codec_node;
	int ret;

	codec_node = of_get_child_by_name(sub_node, "codec");
	if (!codec_node) {
		dev_dbg(dev, "%s no specified codec\n", dai_link->name);
		return 0;
	}

	/* set card codec info */
	ret = snd_soc_of_get_dai_link_codecs(dev, codec_node, dai_link);

	of_node_put(codec_node);

	if (ret < 0)
		return dev_err_probe(dev, ret, "%s: codec dai not found\n",
				     dai_link->name);

	return 0;
}
