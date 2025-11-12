static int snd_byt_wm5102_mc_probe(struct platform_device *pdev)
{
	static const char * const out_map_name[] = { "spk", "hpout2" };
	static const char * const intmic_map_name[] = { "in3l", "in1l" };
	static const char * const hsmic_map_name[] = { "in1l", "in2l" };
	char codec_name[SND_ACPI_I2C_ID_LEN];
	struct device *dev = &pdev->dev;
	struct byt_wm5102_private *priv;
	struct snd_soc_acpi_mach *mach;
	const char *platform_name;
	struct acpi_device *adev;
	struct device *codec_dev;
	int dai_index = 0;
	bool sof_parent;
	int i, ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	/* Get MCLK */
	priv->mclk = devm_clk_get(dev, "pmc_plt_clk_3");
	if (IS_ERR(priv->mclk))
		return dev_err_probe(dev, PTR_ERR(priv->mclk), "getting pmc_plt_clk_3\n");

	/*
	 * Get speaker VDD enable GPIO:
	 * 1. Get codec-device-name
	 * 2. Get codec-device
	 * 3. Get GPIO from codec-device
	 */
	mach = dev->platform_data;
	adev = acpi_dev_get_first_match_dev(mach->id, NULL, -1);
	if (adev) {
		snprintf(codec_name, sizeof(codec_name), "spi-%s", acpi_dev_name(adev));
		acpi_dev_put(adev);
	} else {
		/* Special case for when the codec is missing from the DSTD */
		strscpy(codec_name, "spi1.0", sizeof(codec_name));
	}

	codec_dev = bus_find_device_by_name(&spi_bus_type, NULL, codec_name);
	if (!codec_dev)
		return -EPROBE_DEFER;

	/* Note no devm_ here since we call gpiod_get on codec_dev rather then dev */
	priv->spkvdd_en_gpio = gpiod_get(codec_dev, "wlf,spkvdd-ena", GPIOD_OUT_LOW);
	put_device(codec_dev);

	if (IS_ERR(priv->spkvdd_en_gpio)) {
		ret = PTR_ERR(priv->spkvdd_en_gpio);
		/*
		 * The spkvdd gpio-lookup is registered by: drivers/mfd/arizona-spi.c,
		 * so -ENOENT means that arizona-spi hasn't probed yet.
		 */
		if (ret == -ENOENT)
			ret = -EPROBE_DEFER;

		return dev_err_probe(dev, ret, "getting spkvdd-GPIO\n");
	}

	if (soc_intel_is_cht()) {
		/*
		 * CHT always uses SSP2 and 19.2 MHz; and
		 * the one currently supported CHT design uses HPOUT2 as
		 * speaker output and has the intmic on IN1L + hsmic on IN2L.
		 */
		quirk = BYT_WM5102_SSP2 | BYT_WM5102_MCLK_19_2MHZ |
			BYT_WM5102_INTMIC_IN1L_HSMIC_IN2L |
			BYT_WM5102_SPK_HPOUT2_MAP;
	}
	if (quirk_override != -1) {
		dev_info_once(dev, "Overriding quirk 0x%lx => 0x%x\n",
			      quirk, quirk_override);
		quirk = quirk_override;
	}
	log_quirks(dev);

	snprintf(byt_wm5102_components, sizeof(byt_wm5102_components),
		 "cfg-spk:%s cfg-intmic:%s cfg-hsmic:%s",
		 out_map_name[FIELD_GET(BYT_WM5102_OUT_MAP, quirk)],
		 intmic_map_name[FIELD_GET(BYT_WM5102_IN_MAP, quirk)],
		 hsmic_map_name[FIELD_GET(BYT_WM5102_IN_MAP, quirk)]);
	byt_wm5102_card.components = byt_wm5102_components;

	/* find index of codec dai */
	for (i = 0; i < ARRAY_SIZE(byt_wm5102_dais); i++) {
		if (byt_wm5102_dais[i].num_codecs &&
		    !strcmp(byt_wm5102_dais[i].codecs->name,
			    "wm5102-codec")) {
			dai_index = i;
			break;
		}
	}

	/* override platform name, if required */
	byt_wm5102_card.dev = dev;
	platform_name = mach->mach_params.platform;
	ret = snd_soc_fixup_dai_links_platform_name(&byt_wm5102_card, platform_name);
	if (ret)
		goto out_put_gpio;

	/* override SSP port, if required */
	if (quirk & BYT_WM5102_SSP2)
		byt_wm5102_dais[dai_index].cpus->dai_name = "ssp2-port";

	/* set card and driver name and pm-ops */
	sof_parent = snd_soc_acpi_sof_parent(dev);
	if (sof_parent) {
		byt_wm5102_card.name = SOF_CARD_NAME;
		byt_wm5102_card.driver_name = SOF_DRIVER_NAME;
		dev->driver->pm = &snd_soc_pm_ops;
	} else {
		byt_wm5102_card.name = CARD_NAME;
		byt_wm5102_card.driver_name = DRIVER_NAME;
	}

	snd_soc_card_set_drvdata(&byt_wm5102_card, priv);
	ret = devm_snd_soc_register_card(dev, &byt_wm5102_card);
	if (ret) {
		dev_err_probe(dev, ret, "registering card\n");
		goto out_put_gpio;
	}

	platform_set_drvdata(pdev, &byt_wm5102_card);
	return 0;

out_put_gpio:
	gpiod_put(priv->spkvdd_en_gpio);
	return ret;
}
