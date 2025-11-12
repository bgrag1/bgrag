static int sbi_cpu_start(unsigned int cpuid, struct task_struct *tidle)
{
	unsigned long boot_addr = __pa_symbol(secondary_start_sbi);
	unsigned long hartid = cpuid_to_hartid_map(cpuid);
	unsigned long hsm_data;
	struct sbi_hart_boot_data *bdata = &per_cpu(boot_data, cpuid);

	/* Make sure tidle is updated */
	smp_mb();
	bdata->task_ptr = tidle;
	bdata->stack_ptr = task_pt_regs(tidle);
	/* Make sure boot data is updated */
	smp_mb();
	hsm_data = __pa(bdata);
	return sbi_hsm_hart_start(hartid, boot_addr, hsm_data);
}
