static void nxp_fspi_fill_txfifo(struct nxp_fspi *f,
				 const struct spi_mem_op *op)
{
	void __iomem *base = f->iobase;
	int i, ret;
	u8 *buf = (u8 *) op->data.buf.out;

	/* clear the TX FIFO. */
	fspi_writel(f, FSPI_IPTXFCR_CLR, base + FSPI_IPTXFCR);

	/*
	 * Default value of water mark level is 8 bytes, hence in single
	 * write request controller can write max 8 bytes of data.
	 */

	for (i = 0; i < ALIGN_DOWN(op->data.nbytes, 8); i += 8) {
		/* Wait for TXFIFO empty */
		ret = fspi_readl_poll_tout(f, f->iobase + FSPI_INTR,
					   FSPI_INTR_IPTXWE, 0,
					   POLL_TOUT, true);
		WARN_ON(ret);

		fspi_writel(f, *(u32 *) (buf + i), base + FSPI_TFDR);
		fspi_writel(f, *(u32 *) (buf + i + 4), base + FSPI_TFDR + 4);
		fspi_writel(f, FSPI_INTR_IPTXWE, base + FSPI_INTR);
	}

	if (i < op->data.nbytes) {
		u32 data = 0;
		int j;
		/* Wait for TXFIFO empty */
		ret = fspi_readl_poll_tout(f, f->iobase + FSPI_INTR,
					   FSPI_INTR_IPTXWE, 0,
					   POLL_TOUT, true);
		WARN_ON(ret);

		for (j = 0; j < ALIGN(op->data.nbytes - i, 4); j += 4) {
			memcpy(&data, buf + i + j, 4);
			fspi_writel(f, data, base + FSPI_TFDR + j);
		}
		fspi_writel(f, FSPI_INTR_IPTXWE, base + FSPI_INTR);
	}
}
