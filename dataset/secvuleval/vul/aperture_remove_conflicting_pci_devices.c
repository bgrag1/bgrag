int aperture_remove_conflicting_pci_devices(struct pci_dev *pdev, const char *name)
{
	bool primary = false;
	resource_size_t base, size;
	int bar, ret = 0;

	if (pdev == vga_default_device())
		primary = true;

	if (primary)
		sysfb_disable();

	for (bar = 0; bar < PCI_STD_NUM_BARS; ++bar) {
		if (!(pci_resource_flags(pdev, bar) & IORESOURCE_MEM))
			continue;

		base = pci_resource_start(pdev, bar);
		size = pci_resource_len(pdev, bar);
		aperture_detach_devices(base, size);
	}

	/*
	 * If this is the primary adapter, there could be a VGA device
	 * that consumes the VGA framebuffer I/O range. Remove this
	 * device as well.
	 */
	if (primary)
		ret = __aperture_remove_legacy_vga_devices(pdev);

	return ret;

}
