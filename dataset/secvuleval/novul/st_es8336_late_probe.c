static int st_es8336_late_probe(struct snd_soc_card *card)
{
	struct acpi_device *adev;
	int ret;

	adev = acpi_dev_get_first_match_dev("ESSX8336", NULL, -1);
	if (!adev)
		return -ENODEV;

	codec_dev = acpi_get_first_physical_node(adev);
	acpi_dev_put(adev);
	if (!codec_dev) {
		dev_err(card->dev, "can not find codec dev\n");
		return -ENODEV;
	}

	ret = devm_acpi_dev_add_driver_gpios(codec_dev, acpi_es8336_gpios);
	if (ret)
		dev_warn(card->dev, "Failed to add driver gpios\n");

	gpio_pa = gpiod_get_optional(codec_dev, "pa-enable", GPIOD_OUT_LOW);
	if (IS_ERR(gpio_pa)) {
		ret = dev_err_probe(card->dev, PTR_ERR(gpio_pa),
				    "could not get pa-enable GPIO\n");
		put_device(codec_dev);
		return ret;
	}
	return 0;
}
