static enum es_result vc_handle_exitcode(struct es_em_ctxt *ctxt,
					 struct ghcb *ghcb,
					 unsigned long exit_code)
{
	enum es_result result;

	switch (exit_code) {
	case SVM_EXIT_READ_DR7:
		result = vc_handle_dr7_read(ghcb, ctxt);
		break;
	case SVM_EXIT_WRITE_DR7:
		result = vc_handle_dr7_write(ghcb, ctxt);
		break;
	case SVM_EXIT_EXCP_BASE + X86_TRAP_AC:
		result = vc_handle_trap_ac(ghcb, ctxt);
		break;
	case SVM_EXIT_RDTSC:
	case SVM_EXIT_RDTSCP:
		result = vc_handle_rdtsc(ghcb, ctxt, exit_code);
		break;
	case SVM_EXIT_RDPMC:
		result = vc_handle_rdpmc(ghcb, ctxt);
		break;
	case SVM_EXIT_INVD:
		pr_err_ratelimited("#VC exception for INVD??? Seriously???\n");
		result = ES_UNSUPPORTED;
		break;
	case SVM_EXIT_CPUID:
		result = vc_handle_cpuid(ghcb, ctxt);
		break;
	case SVM_EXIT_IOIO:
		result = vc_handle_ioio(ghcb, ctxt);
		break;
	case SVM_EXIT_MSR:
		result = vc_handle_msr(ghcb, ctxt);
		break;
	case SVM_EXIT_VMMCALL:
		result = vc_handle_vmmcall(ghcb, ctxt);
		break;
	case SVM_EXIT_WBINVD:
		result = vc_handle_wbinvd(ghcb, ctxt);
		break;
	case SVM_EXIT_MONITOR:
		result = vc_handle_monitor(ghcb, ctxt);
		break;
	case SVM_EXIT_MWAIT:
		result = vc_handle_mwait(ghcb, ctxt);
		break;
	case SVM_EXIT_NPF:
		result = vc_handle_mmio(ghcb, ctxt);
		break;
	default:
		/*
		 * Unexpected #VC exception
		 */
		result = ES_UNSUPPORTED;
	}

	return result;
}