static bool access_gic_sgi(struct kvm_vcpu *vcpu,
			   struct sys_reg_params *p,
			   const struct sys_reg_desc *r)
{
	bool g1;

	if (!p->is_write)
		return read_from_write_only(vcpu, p, r);

	/*
	 * In a system where GICD_CTLR.DS=1, a ICC_SGI0R_EL1 access generates
	 * Group0 SGIs only, while ICC_SGI1R_EL1 can generate either group,
	 * depending on the SGI configuration. ICC_ASGI1R_EL1 is effectively
	 * equivalent to ICC_SGI0R_EL1, as there is no "alternative" secure
	 * group.
	 */
	if (p->Op0 == 0) {		/* AArch32 */
		switch (p->Op1) {
		default:		/* Keep GCC quiet */
		case 0:			/* ICC_SGI1R */
			g1 = true;
			break;
		case 1:			/* ICC_ASGI1R */
		case 2:			/* ICC_SGI0R */
			g1 = false;
			break;
		}
	} else {			/* AArch64 */
		switch (p->Op2) {
		default:		/* Keep GCC quiet */
		case 5:			/* ICC_SGI1R_EL1 */
			g1 = true;
			break;
		case 6:			/* ICC_ASGI1R_EL1 */
		case 7:			/* ICC_SGI0R_EL1 */
			g1 = false;
			break;
		}
	}

	vgic_v3_dispatch_sgi(vcpu, p->regval, g1);

	return true;
}
