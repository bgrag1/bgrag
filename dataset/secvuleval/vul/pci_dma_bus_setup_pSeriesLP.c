static void pci_dma_bus_setup_pSeriesLP(struct pci_bus *bus)
{
	struct iommu_table *tbl;
	struct device_node *dn, *pdn;
	struct pci_dn *ppci;
	struct dynamic_dma_window_prop prop;

	dn = pci_bus_to_OF_node(bus);

	pr_debug("pci_dma_bus_setup_pSeriesLP: setting up bus %pOF\n",
		 dn);

	pdn = pci_dma_find(dn, &prop);

	/* In PPC architecture, there will always be DMA window on bus or one of the
	 * parent bus. During reboot, there will be ibm,dma-window property to
	 * define DMA window. For kdump, there will at least be default window or DDW
	 * or both.
	 */

	ppci = PCI_DN(pdn);

	pr_debug("  parent is %pOF, iommu_table: 0x%p\n",
		 pdn, ppci->table_group);

	if (!ppci->table_group) {
		ppci->table_group = iommu_pseries_alloc_group(ppci->phb->node);
		tbl = ppci->table_group->tables[0];

		iommu_table_setparms_common(tbl, ppci->phb->bus->number,
				be32_to_cpu(prop.liobn),
				be64_to_cpu(prop.dma_base),
				1ULL << be32_to_cpu(prop.window_shift),
				be32_to_cpu(prop.tce_shift), NULL,
				&iommu_table_lpar_multi_ops);

		/* Only for normal boot with default window. Doesn't matter even
		 * if we set these with DDW which is 64bit during kdump, since
		 * these will not be used during kdump.
		 */
		ppci->table_group->tce32_start = be64_to_cpu(prop.dma_base);
		ppci->table_group->tce32_size = 1 << be32_to_cpu(prop.window_shift);

		if (!iommu_init_table(tbl, ppci->phb->node, 0, 0))
			panic("Failed to initialize iommu table");

		iommu_register_group(ppci->table_group,
				pci_domain_nr(bus), 0);
		pr_debug("  created table: %p\n", ppci->table_group);
	}
}
