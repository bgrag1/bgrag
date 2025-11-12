static irqreturn_t pmu_sbi_ovf_handler(int irq, void *dev)
{
	struct perf_sample_data data;
	struct pt_regs *regs;
	struct hw_perf_event *hw_evt;
	union sbi_pmu_ctr_info *info;
	int lidx, hidx, fidx;
	struct riscv_pmu *pmu;
	struct perf_event *event;
	unsigned long overflow;
	unsigned long overflowed_ctrs = 0;
	struct cpu_hw_events *cpu_hw_evt = dev;
	u64 start_clock = sched_clock();

	if (WARN_ON_ONCE(!cpu_hw_evt))
		return IRQ_NONE;

	/* Firmware counter don't support overflow yet */
	fidx = find_first_bit(cpu_hw_evt->used_hw_ctrs, RISCV_MAX_COUNTERS);
	if (fidx == RISCV_MAX_COUNTERS) {
		csr_clear(CSR_SIP, BIT(riscv_pmu_irq_num));
		return IRQ_NONE;
	}

	event = cpu_hw_evt->events[fidx];
	if (!event) {
		csr_clear(CSR_SIP, BIT(riscv_pmu_irq_num));
		return IRQ_NONE;
	}

	pmu = to_riscv_pmu(event->pmu);
	pmu_sbi_stop_hw_ctrs(pmu);

	/* Overflow status register should only be read after counter are stopped */
	ALT_SBI_PMU_OVERFLOW(overflow);

	/*
	 * Overflow interrupt pending bit should only be cleared after stopping
	 * all the counters to avoid any race condition.
	 */
	csr_clear(CSR_SIP, BIT(riscv_pmu_irq_num));

	/* No overflow bit is set */
	if (!overflow)
		return IRQ_NONE;

	regs = get_irq_regs();

	for_each_set_bit(lidx, cpu_hw_evt->used_hw_ctrs, RISCV_MAX_COUNTERS) {
		struct perf_event *event = cpu_hw_evt->events[lidx];

		/* Skip if invalid event or user did not request a sampling */
		if (!event || !is_sampling_event(event))
			continue;

		info = &pmu_ctr_list[lidx];
		/* Do a sanity check */
		if (!info || info->type != SBI_PMU_CTR_TYPE_HW)
			continue;

		/* compute hardware counter index */
		hidx = info->csr - CSR_CYCLE;
		/* check if the corresponding bit is set in sscountovf */
		if (!(overflow & (1 << hidx)))
			continue;

		/*
		 * Keep a track of overflowed counters so that they can be started
		 * with updated initial value.
		 */
		overflowed_ctrs |= 1 << lidx;
		hw_evt = &event->hw;
		riscv_pmu_event_update(event);
		perf_sample_data_init(&data, 0, hw_evt->last_period);
		if (riscv_pmu_event_set_period(event)) {
			/*
			 * Unlike other ISAs, RISC-V don't have to disable interrupts
			 * to avoid throttling here. As per the specification, the
			 * interrupt remains disabled until the OF bit is set.
			 * Interrupts are enabled again only during the start.
			 * TODO: We will need to stop the guest counters once
			 * virtualization support is added.
			 */
			perf_event_overflow(event, &data, regs);
		}
	}

	pmu_sbi_start_overflow_mask(pmu, overflowed_ctrs);
	perf_sample_event_took(sched_clock() - start_clock);

	return IRQ_HANDLED;
}
