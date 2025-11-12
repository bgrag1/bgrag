static int broxton_audio_probe(struct platform_device *pdev)
{
	struct bxt_rt286_private *ctx;
	struct snd_soc_card *card =
			(struct snd_soc_card *)pdev->id_entry->driver_data;
	struct snd_soc_acpi_mach *mach;
	const char *platform_name;
	int ret;
	int i;

	for (i = 0; i < ARRAY_SIZE(broxton_rt298_dais); i++) {
		if (card->dai_link[i].num_codecs &&
		    !strncmp(card->dai_link[i].codecs->name, "i2c-INT343A:00",
			     I2C_NAME_SIZE)) {
			if (!strncmp(card->name, "broxton-rt298",
				     PLATFORM_NAME_SIZE)) {
				card->dai_link[i].name = "SSP5-Codec";
				card->dai_link[i].cpus->dai_name = "SSP5 Pin";
			} else if (!strncmp(card->name, "geminilake-rt298",
					    PLATFORM_NAME_SIZE)) {
				card->dai_link[i].name = "SSP2-Codec";
				card->dai_link[i].cpus->dai_name = "SSP2 Pin";
			}
		}
	}

	ctx = devm_kzalloc(&pdev->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	INIT_LIST_HEAD(&ctx->hdmi_pcm_list);

	card->dev = &pdev->dev;
	snd_soc_card_set_drvdata(card, ctx);

	/* override platform name, if required */
	mach = pdev->dev.platform_data;
	platform_name = mach->mach_params.platform;

	ret = snd_soc_fixup_dai_links_platform_name(card,
						    platform_name);
	if (ret)
		return ret;

	ctx->common_hdmi_codec_drv = mach->mach_params.common_hdmi_codec_drv;

	return devm_snd_soc_register_card(&pdev->dev, card);
}
