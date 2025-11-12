static void cpu_update_secondary_bootdata(unsigned int cpuid,
				   struct task_struct *tidle)
{
	unsigned long hartid = cpuid_to_hartid_map(cpuid);

	/*
	 * The hartid must be less than NR_CPUS to avoid out-of-bound access
	 * errors for __cpu_spinwait_stack/task_pointer. That is not always possible
	 * for platforms with discontiguous hartid numbering scheme. That's why
	 * spinwait booting is not the recommended approach for any platforms
	 * booting Linux in S-mode and can be disabled in the future.
	 */
	if (hartid == INVALID_HARTID || hartid >= (unsigned long) NR_CPUS)
		return;

	/* Make sure tidle is updated */
	smp_mb();
	WRITE_ONCE(__cpu_spinwait_stack_pointer[hartid],
		   task_stack_page(tidle) + THREAD_SIZE);
	WRITE_ONCE(__cpu_spinwait_task_pointer[hartid], tidle);
}
