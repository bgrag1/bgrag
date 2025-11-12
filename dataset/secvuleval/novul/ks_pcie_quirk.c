static void ks_pcie_quirk(struct pci_dev *dev)
{
	struct pci_bus *bus = dev->bus;
	struct keystone_pcie *ks_pcie;
	struct device *bridge_dev;
	struct pci_dev *bridge;
	u32 val;

	static const struct pci_device_id rc_pci_devids[] = {
		{ PCI_DEVICE(PCI_VENDOR_ID_TI, PCIE_RC_K2HK),
		 .class = PCI_CLASS_BRIDGE_PCI_NORMAL, .class_mask = ~0, },
		{ PCI_DEVICE(PCI_VENDOR_ID_TI, PCIE_RC_K2E),
		 .class = PCI_CLASS_BRIDGE_PCI_NORMAL, .class_mask = ~0, },
		{ PCI_DEVICE(PCI_VENDOR_ID_TI, PCIE_RC_K2L),
		 .class = PCI_CLASS_BRIDGE_PCI_NORMAL, .class_mask = ~0, },
		{ PCI_DEVICE(PCI_VENDOR_ID_TI, PCIE_RC_K2G),
		 .class = PCI_CLASS_BRIDGE_PCI_NORMAL, .class_mask = ~0, },
		{ 0, },
	};
	static const struct pci_device_id am6_pci_devids[] = {
		{ PCI_DEVICE(PCI_VENDOR_ID_TI, PCI_DEVICE_ID_TI_AM654X),
		 .class = PCI_CLASS_BRIDGE_PCI << 8, .class_mask = ~0, },
		{ 0, },
	};

	if (pci_is_root_bus(bus))
		bridge = dev;

	/* look for the host bridge */
	while (!pci_is_root_bus(bus)) {
		bridge = bus->self;
		bus = bus->parent;
	}

	if (!bridge)
		return;

	/*
	 * Keystone PCI controller has a h/w limitation of
	 * 256 bytes maximum read request size.  It can't handle
	 * anything higher than this.  So force this limit on
	 * all downstream devices.
	 */
	if (pci_match_id(rc_pci_devids, bridge)) {
		if (pcie_get_readrq(dev) > 256) {
			dev_info(&dev->dev, "limiting MRRS to 256 bytes\n");
			pcie_set_readrq(dev, 256);
		}
	}

	/*
	 * Memory transactions fail with PCI controller in AM654 PG1.0
	 * when MRRS is set to more than 128 bytes. Force the MRRS to
	 * 128 bytes in all downstream devices.
	 */
	if (pci_match_id(am6_pci_devids, bridge)) {
		bridge_dev = pci_get_host_bridge_device(dev);
		if (!bridge_dev || !bridge_dev->parent)
			return;

		ks_pcie = dev_get_drvdata(bridge_dev->parent);
		if (!ks_pcie)
			return;

		val = ks_pcie_app_readl(ks_pcie, PID);
		val &= RTL;
		val >>= RTL_SHIFT;
		if (val != AM6_PCI_PG1_RTL_VER)
			return;

		if (pcie_get_readrq(dev) > 128) {
			dev_info(&dev->dev, "limiting MRRS to 128 bytes\n");
			pcie_set_readrq(dev, 128);
		}
	}
}
