static void sprd_iommu_cleanup(struct sprd_iommu_domain *dom)
{
	size_t pgt_size;

	/* Nothing need to do if the domain hasn't been attached */
	if (!dom->sdev)
		return;

	pgt_size = sprd_iommu_pgt_size(&dom->domain);
	dma_free_coherent(dom->sdev->dev, pgt_size, dom->pgt_va, dom->pgt_pa);
	dom->sdev = NULL;
	sprd_iommu_hw_en(dom->sdev, false);
}
