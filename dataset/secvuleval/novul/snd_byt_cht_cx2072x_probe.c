static int snd_byt_cht_cx2072x_probe(struct platform_device *pdev)
{
	struct snd_soc_acpi_mach *mach;
	struct acpi_device *adev;
	int dai_index = 0;
	bool sof_parent;
	int i, ret;

	byt_cht_cx2072x_card.dev = &pdev->dev;
	mach = dev_get_platdata(&pdev->dev);

	/* fix index of codec dai */
	for (i = 0; i < ARRAY_SIZE(byt_cht_cx2072x_dais); i++) {
		if (byt_cht_cx2072x_dais[i].num_codecs &&
		    !strcmp(byt_cht_cx2072x_dais[i].codecs->name,
			    "i2c-14F10720:00")) {
			dai_index = i;
			break;
		}
	}

	/* fixup codec name based on HID */
	adev = acpi_dev_get_first_match_dev(mach->id, NULL, -1);
	if (adev) {
		snprintf(codec_name, sizeof(codec_name), "i2c-%s",
			 acpi_dev_name(adev));
		byt_cht_cx2072x_dais[dai_index].codecs->name = codec_name;
	}
	acpi_dev_put(adev);

	/* override platform name, if required */
	ret = snd_soc_fixup_dai_links_platform_name(&byt_cht_cx2072x_card,
						    mach->mach_params.platform);
	if (ret)
		return ret;

	sof_parent = snd_soc_acpi_sof_parent(&pdev->dev);

	/* set card and driver name */
	if (sof_parent) {
		byt_cht_cx2072x_card.name = SOF_CARD_NAME;
		byt_cht_cx2072x_card.driver_name = SOF_DRIVER_NAME;
	} else {
		byt_cht_cx2072x_card.name = CARD_NAME;
		byt_cht_cx2072x_card.driver_name = DRIVER_NAME;
	}

	/* set pm ops */
	if (sof_parent)
		pdev->dev.driver->pm = &snd_soc_pm_ops;

	return devm_snd_soc_register_card(&pdev->dev, &byt_cht_cx2072x_card);
}
