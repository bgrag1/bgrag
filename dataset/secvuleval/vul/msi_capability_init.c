static int msi_capability_init(struct pci_dev *dev, int nvec,
			       struct irq_affinity *affd)
{
	struct irq_affinity_desc *masks = NULL;
	struct msi_desc *entry;
	int ret;

	/* Reject multi-MSI early on irq domain enabled architectures */
	if (nvec > 1 && !pci_msi_domain_supports(dev, MSI_FLAG_MULTI_PCI_MSI, ALLOW_LEGACY))
		return 1;

	/*
	 * Disable MSI during setup in the hardware, but mark it enabled
	 * so that setup code can evaluate it.
	 */
	pci_msi_set_enable(dev, 0);
	dev->msi_enabled = 1;

	if (affd)
		masks = irq_create_affinity_masks(nvec, affd);

	msi_lock_descs(&dev->dev);
	ret = msi_setup_msi_desc(dev, nvec, masks);
	if (ret)
		goto fail;

	/* All MSIs are unmasked by default; mask them all */
	entry = msi_first_desc(&dev->dev, MSI_DESC_ALL);
	pci_msi_mask(entry, msi_multi_mask(entry));

	/* Configure MSI capability structure */
	ret = pci_msi_setup_msi_irqs(dev, nvec, PCI_CAP_ID_MSI);
	if (ret)
		goto err;

	ret = msi_verify_entries(dev);
	if (ret)
		goto err;

	/* Set MSI enabled bits	*/
	pci_intx_for_msi(dev, 0);
	pci_msi_set_enable(dev, 1);

	pcibios_free_irq(dev);
	dev->irq = entry->irq;
	goto unlock;

err:
	pci_msi_unmask(entry, msi_multi_mask(entry));
	pci_free_msi_irqs(dev);
fail:
	dev->msi_enabled = 0;
unlock:
	msi_unlock_descs(&dev->dev);
	kfree(masks);
	return ret;
}
