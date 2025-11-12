static int bytcht_da7213_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card;
	struct snd_soc_acpi_mach *mach;
	const char *platform_name;
	struct acpi_device *adev;
	bool sof_parent;
	int dai_index = 0;
	int ret_val = 0;
	int i;

	mach = pdev->dev.platform_data;
	card = &bytcht_da7213_card;
	card->dev = &pdev->dev;

	/* fix index of codec dai */
	for (i = 0; i < ARRAY_SIZE(dailink); i++) {
		if (dailink[i].codecs->name &&
		    !strcmp(dailink[i].codecs->name, "i2c-DLGS7213:00")) {
			dai_index = i;
			break;
		}
	}

	/* fixup codec name based on HID */
	adev = acpi_dev_get_first_match_dev(mach->id, NULL, -1);
	if (adev) {
		snprintf(codec_name, sizeof(codec_name),
			 "i2c-%s", acpi_dev_name(adev));
		dailink[dai_index].codecs->name = codec_name;
	}
	acpi_dev_put(adev);

	/* override platform name, if required */
	platform_name = mach->mach_params.platform;

	ret_val = snd_soc_fixup_dai_links_platform_name(card, platform_name);
	if (ret_val)
		return ret_val;

	sof_parent = snd_soc_acpi_sof_parent(&pdev->dev);

	/* set card and driver name */
	if (sof_parent) {
		bytcht_da7213_card.name = SOF_CARD_NAME;
		bytcht_da7213_card.driver_name = SOF_DRIVER_NAME;
	} else {
		bytcht_da7213_card.name = CARD_NAME;
		bytcht_da7213_card.driver_name = DRIVER_NAME;
	}

	/* set pm ops */
	if (sof_parent)
		pdev->dev.driver->pm = &snd_soc_pm_ops;

	ret_val = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret_val) {
		dev_err(&pdev->dev,
			"snd_soc_register_card failed %d\n", ret_val);
		return ret_val;
	}
	platform_set_drvdata(pdev, card);
	return ret_val;
}
