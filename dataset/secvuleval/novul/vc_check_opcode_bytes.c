static enum es_result vc_check_opcode_bytes(struct es_em_ctxt *ctxt,
					    unsigned long exit_code)
{
	unsigned int opcode = (unsigned int)ctxt->insn.opcode.value;
	u8 modrm = ctxt->insn.modrm.value;

	switch (exit_code) {

	case SVM_EXIT_IOIO:
	case SVM_EXIT_NPF:
		/* handled separately */
		return ES_OK;

	case SVM_EXIT_CPUID:
		if (opcode == 0xa20f)
			return ES_OK;
		break;

	case SVM_EXIT_INVD:
		if (opcode == 0x080f)
			return ES_OK;
		break;

	case SVM_EXIT_MONITOR:
		if (opcode == 0x010f && modrm == 0xc8)
			return ES_OK;
		break;

	case SVM_EXIT_MWAIT:
		if (opcode == 0x010f && modrm == 0xc9)
			return ES_OK;
		break;

	case SVM_EXIT_MSR:
		/* RDMSR */
		if (opcode == 0x320f ||
		/* WRMSR */
		    opcode == 0x300f)
			return ES_OK;
		break;

	case SVM_EXIT_RDPMC:
		if (opcode == 0x330f)
			return ES_OK;
		break;

	case SVM_EXIT_RDTSC:
		if (opcode == 0x310f)
			return ES_OK;
		break;

	case SVM_EXIT_RDTSCP:
		if (opcode == 0x010f && modrm == 0xf9)
			return ES_OK;
		break;

	case SVM_EXIT_READ_DR7:
		if (opcode == 0x210f &&
		    X86_MODRM_REG(ctxt->insn.modrm.value) == 7)
			return ES_OK;
		break;

	case SVM_EXIT_VMMCALL:
		if (opcode == 0x010f && modrm == 0xd9)
			return ES_OK;

		break;

	case SVM_EXIT_WRITE_DR7:
		if (opcode == 0x230f &&
		    X86_MODRM_REG(ctxt->insn.modrm.value) == 7)
			return ES_OK;
		break;

	case SVM_EXIT_WBINVD:
		if (opcode == 0x90f)
			return ES_OK;
		break;

	default:
		break;
	}

	sev_printk(KERN_ERR "Wrong/unhandled opcode bytes: 0x%x, exit_code: 0x%lx, rIP: 0x%lx\n",
		   opcode, exit_code, ctxt->regs->ip);

	return ES_UNSUPPORTED;
}