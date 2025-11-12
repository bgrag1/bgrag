static void pnv_php_disable_irq(struct pnv_php_slot *php_slot,
				bool disable_device)
{
	struct pci_dev *pdev = php_slot->pdev;
	int irq = php_slot->irq;
	u16 ctrl;

	if (php_slot->irq > 0) {
		pcie_capability_read_word(pdev, PCI_EXP_SLTCTL, &ctrl);
		ctrl &= ~(PCI_EXP_SLTCTL_HPIE |
			  PCI_EXP_SLTCTL_PDCE |
			  PCI_EXP_SLTCTL_DLLSCE);
		pcie_capability_write_word(pdev, PCI_EXP_SLTCTL, ctrl);

		free_irq(php_slot->irq, php_slot);
		php_slot->irq = 0;
	}

	if (php_slot->wq) {
		destroy_workqueue(php_slot->wq);
		php_slot->wq = NULL;
	}

	if (disable_device || irq > 0) {
		if (pdev->msix_enabled)
			pci_disable_msix(pdev);
		else if (pdev->msi_enabled)
			pci_disable_msi(pdev);

		pci_disable_device(pdev);
	}
}
