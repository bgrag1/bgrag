static int intel_spi_populate_chip(struct intel_spi *ispi)
{
	struct flash_platform_data *pdata;
	struct mtd_partition *parts;
	struct spi_board_info chip;
	int ret;

	ret = intel_spi_read_desc(ispi);
	if (ret)
		return ret;

	pdata = devm_kzalloc(ispi->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	pdata->nr_parts = 1;
	pdata->parts = devm_kcalloc(ispi->dev, pdata->nr_parts,
				    sizeof(*pdata->parts), GFP_KERNEL);
	if (!pdata->parts)
		return -ENOMEM;

	intel_spi_fill_partition(ispi, pdata->parts);

	memset(&chip, 0, sizeof(chip));
	snprintf(chip.modalias, 8, "spi-nor");
	chip.platform_data = pdata;

	if (!spi_new_device(ispi->host, &chip))
		return -ENODEV;

	/* Add the second chip if present */
	if (ispi->host->num_chipselect < 2)
		return 0;

	pdata = devm_kzalloc(ispi->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	pdata->name = devm_kasprintf(ispi->dev, GFP_KERNEL, "%s-chip1",
				     dev_name(ispi->dev));
	pdata->nr_parts = 1;
	parts = devm_kcalloc(ispi->dev, pdata->nr_parts, sizeof(*parts),
			     GFP_KERNEL);
	if (!parts)
		return -ENOMEM;

	parts[0].size = MTDPART_SIZ_FULL;
	parts[0].name = "BIOS1";
	pdata->parts = parts;

	chip.platform_data = pdata;
	chip.chip_select = 1;

	if (!spi_new_device(ispi->host, &chip))
		return -ENODEV;
	return 0;
}
