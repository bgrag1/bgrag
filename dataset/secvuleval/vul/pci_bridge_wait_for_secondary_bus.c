int pci_bridge_wait_for_secondary_bus(struct pci_dev *dev, char *reset_type)
{
	struct pci_dev *child;
	int delay;

	if (pci_dev_is_disconnected(dev))
		return 0;

	if (!pci_is_bridge(dev))
		return 0;

	down_read(&pci_bus_sem);

	/*
	 * We only deal with devices that are present currently on the bus.
	 * For any hot-added devices the access delay is handled in pciehp
	 * board_added(). In case of ACPI hotplug the firmware is expected
	 * to configure the devices before OS is notified.
	 */
	if (!dev->subordinate || list_empty(&dev->subordinate->devices)) {
		up_read(&pci_bus_sem);
		return 0;
	}

	/* Take d3cold_delay requirements into account */
	delay = pci_bus_max_d3cold_delay(dev->subordinate);
	if (!delay) {
		up_read(&pci_bus_sem);
		return 0;
	}

	child = list_first_entry(&dev->subordinate->devices, struct pci_dev,
				 bus_list);
	up_read(&pci_bus_sem);

	/*
	 * Conventional PCI and PCI-X we need to wait Tpvrh + Trhfa before
	 * accessing the device after reset (that is 1000 ms + 100 ms).
	 */
	if (!pci_is_pcie(dev)) {
		pci_dbg(dev, "waiting %d ms for secondary bus\n", 1000 + delay);
		msleep(1000 + delay);
		return 0;
	}

	/*
	 * For PCIe downstream and root ports that do not support speeds
	 * greater than 5 GT/s need to wait minimum 100 ms. For higher
	 * speeds (gen3) we need to wait first for the data link layer to
	 * become active.
	 *
	 * However, 100 ms is the minimum and the PCIe spec says the
	 * software must allow at least 1s before it can determine that the
	 * device that did not respond is a broken device. Also device can
	 * take longer than that to respond if it indicates so through Request
	 * Retry Status completions.
	 *
	 * Therefore we wait for 100 ms and check for the device presence
	 * until the timeout expires.
	 */
	if (!pcie_downstream_port(dev))
		return 0;

	if (pcie_get_speed_cap(dev) <= PCIE_SPEED_5_0GT) {
		u16 status;

		pci_dbg(dev, "waiting %d ms for downstream link\n", delay);
		msleep(delay);

		if (!pci_dev_wait(child, reset_type, PCI_RESET_WAIT - delay))
			return 0;

		/*
		 * If the port supports active link reporting we now check
		 * whether the link is active and if not bail out early with
		 * the assumption that the device is not present anymore.
		 */
		if (!dev->link_active_reporting)
			return -ENOTTY;

		pcie_capability_read_word(dev, PCI_EXP_LNKSTA, &status);
		if (!(status & PCI_EXP_LNKSTA_DLLLA))
			return -ENOTTY;

		return pci_dev_wait(child, reset_type,
				    PCIE_RESET_READY_POLL_MS - PCI_RESET_WAIT);
	}

	pci_dbg(dev, "waiting %d ms for downstream link, after activation\n",
		delay);
	if (!pcie_wait_for_link_delay(dev, true, delay)) {
		/* Did not train, no need to wait any further */
		pci_info(dev, "Data Link Layer Link Active not set in 1000 msec\n");
		return -ENOTTY;
	}

	return pci_dev_wait(child, reset_type,
			    PCIE_RESET_READY_POLL_MS - delay);
}
