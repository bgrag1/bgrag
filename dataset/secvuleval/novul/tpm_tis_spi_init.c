int tpm_tis_spi_init(struct spi_device *spi, struct tpm_tis_spi_phy *phy,
		     int irq, const struct tpm_tis_phy_ops *phy_ops)
{
	phy->iobuf = devm_kmalloc(&spi->dev, SPI_HDRSIZE + MAX_SPI_FRAMESIZE, GFP_KERNEL);
	if (!phy->iobuf)
		return -ENOMEM;

	phy->spi_device = spi;

	return tpm_tis_core_init(&spi->dev, &phy->priv, irq, phy_ops, NULL);
}
