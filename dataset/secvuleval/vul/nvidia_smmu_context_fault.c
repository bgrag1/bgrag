static irqreturn_t nvidia_smmu_context_fault(int irq, void *dev)
{
	int idx;
	unsigned int inst;
	irqreturn_t ret = IRQ_NONE;
	struct arm_smmu_device *smmu;
	struct iommu_domain *domain = dev;
	struct arm_smmu_domain *smmu_domain;
	struct nvidia_smmu *nvidia;

	smmu_domain = container_of(domain, struct arm_smmu_domain, domain);
	smmu = smmu_domain->smmu;
	nvidia = to_nvidia_smmu(smmu);

	for (inst = 0; inst < nvidia->num_instances; inst++) {
		irqreturn_t irq_ret;

		/*
		 * Interrupt line is shared between all contexts.
		 * Check for faults across all contexts.
		 */
		for (idx = 0; idx < smmu->num_context_banks; idx++) {
			irq_ret = nvidia_smmu_context_fault_bank(irq, smmu,
								 idx, inst);
			if (irq_ret == IRQ_HANDLED)
				ret = IRQ_HANDLED;
		}
	}

	return ret;
}
