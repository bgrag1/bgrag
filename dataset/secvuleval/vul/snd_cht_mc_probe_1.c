static int snd_cht_mc_probe(struct platform_device *pdev)
{
	int ret_val = 0;
	struct cht_mc_private *drv;
	struct snd_soc_acpi_mach *mach = pdev->dev.platform_data;
	const char *platform_name;
	struct acpi_device *adev;
	bool sof_parent;
	int dai_index = 0;
	int i;

	drv = devm_kzalloc(&pdev->dev, sizeof(*drv), GFP_KERNEL);
	if (!drv)
		return -ENOMEM;

	strcpy(drv->codec_name, RT5672_I2C_DEFAULT);

	/* find index of codec dai */
	for (i = 0; i < ARRAY_SIZE(cht_dailink); i++) {
		if (cht_dailink[i].codecs->name &&
		    !strcmp(cht_dailink[i].codecs->name, RT5672_I2C_DEFAULT)) {
			dai_index = i;
			break;
		}
	}

	/* fixup codec name based on HID */
	adev = acpi_dev_get_first_match_dev(mach->id, NULL, -1);
	if (adev) {
		snprintf(drv->codec_name, sizeof(drv->codec_name),
			 "i2c-%s", acpi_dev_name(adev));
		cht_dailink[dai_index].codecs->name = drv->codec_name;
	}
	acpi_dev_put(adev);

	/* Use SSP0 on Bay Trail CR devices */
	if (soc_intel_is_byt() && mach->mach_params.acpi_ipc_irq_index == 0) {
		cht_dailink[dai_index].cpus->dai_name = "ssp0-port";
		drv->use_ssp0 = true;
	}

	/* override platform name, if required */
	snd_soc_card_cht.dev = &pdev->dev;
	platform_name = mach->mach_params.platform;

	ret_val = snd_soc_fixup_dai_links_platform_name(&snd_soc_card_cht,
							platform_name);
	if (ret_val)
		return ret_val;

	snd_soc_card_cht.components = rt5670_components();

	drv->mclk = devm_clk_get(&pdev->dev, "pmc_plt_clk_3");
	if (IS_ERR(drv->mclk)) {
		dev_err(&pdev->dev,
			"Failed to get MCLK from pmc_plt_clk_3: %ld\n",
			PTR_ERR(drv->mclk));
		return PTR_ERR(drv->mclk);
	}
	snd_soc_card_set_drvdata(&snd_soc_card_cht, drv);

	sof_parent = snd_soc_acpi_sof_parent(&pdev->dev);

	/* set card and driver name */
	if (sof_parent) {
		snd_soc_card_cht.name = SOF_CARD_NAME;
		snd_soc_card_cht.driver_name = SOF_DRIVER_NAME;
	} else {
		snd_soc_card_cht.name = CARD_NAME;
		snd_soc_card_cht.driver_name = DRIVER_NAME;
	}

	/* set pm ops */
	if (sof_parent)
		pdev->dev.driver->pm = &snd_soc_pm_ops;

	/* register the soc card */
	ret_val = devm_snd_soc_register_card(&pdev->dev, &snd_soc_card_cht);
	if (ret_val) {
		dev_err(&pdev->dev,
			"snd_soc_register_card failed %d\n", ret_val);
		return ret_val;
	}
	platform_set_drvdata(pdev, &snd_soc_card_cht);
	return ret_val;
}
