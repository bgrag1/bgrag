void __init do_vc_no_ghcb(struct pt_regs *regs, unsigned long exit_code)
{
	unsigned int subfn = lower_bits(regs->cx, 32);
	unsigned int fn = lower_bits(regs->ax, 32);
	u16 opcode = *(unsigned short *)regs->ip;
	struct cpuid_leaf leaf;
	int ret;

	/* Only CPUID is supported via MSR protocol */
	if (exit_code != SVM_EXIT_CPUID)
		goto fail;

	/* Is it really a CPUID insn? */
	if (opcode != 0xa20f)
		goto fail;

	leaf.fn = fn;
	leaf.subfn = subfn;

	ret = snp_cpuid(NULL, NULL, &leaf);
	if (!ret)
		goto cpuid_done;

	if (ret != -EOPNOTSUPP)
		goto fail;

	if (__sev_cpuid_hv_msr(&leaf))
		goto fail;

cpuid_done:
	regs->ax = leaf.eax;
	regs->bx = leaf.ebx;
	regs->cx = leaf.ecx;
	regs->dx = leaf.edx;

	/*
	 * This is a VC handler and the #VC is only raised when SEV-ES is
	 * active, which means SEV must be active too. Do sanity checks on the
	 * CPUID results to make sure the hypervisor does not trick the kernel
	 * into the no-sev path. This could map sensitive data unencrypted and
	 * make it accessible to the hypervisor.
	 *
	 * In particular, check for:
	 *	- Availability of CPUID leaf 0x8000001f
	 *	- SEV CPUID bit.
	 *
	 * The hypervisor might still report the wrong C-bit position, but this
	 * can't be checked here.
	 */

	if (fn == 0x80000000 && (regs->ax < 0x8000001f))
		/* SEV leaf check */
		goto fail;
	else if ((fn == 0x8000001f && !(regs->ax & BIT(1))))
		/* SEV bit */
		goto fail;

	/* Skip over the CPUID two-byte opcode */
	regs->ip += 2;

	return;

fail:
	/* Terminate the guest */
	sev_es_terminate(SEV_TERM_SET_GEN, GHCB_SEV_ES_GEN_REQ);
}