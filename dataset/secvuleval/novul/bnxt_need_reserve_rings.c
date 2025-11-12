static bool bnxt_need_reserve_rings(struct bnxt *bp)
{
	struct bnxt_hw_resc *hw_resc = &bp->hw_resc;
	int cp = bnxt_cp_rings_in_use(bp);
	int nq = bnxt_nq_rings_in_use(bp);
	int rx = bp->rx_nr_rings, stat;
	int vnic, grp = rx;

	/* Old firmware does not need RX ring reservations but we still
	 * need to setup a default RSS map when needed.  With new firmware
	 * we go through RX ring reservations first and then set up the
	 * RSS map for the successfully reserved RX rings when needed.
	 */
	if (!BNXT_NEW_RM(bp))
		bnxt_check_rss_tbl_no_rmgr(bp);

	if (hw_resc->resv_tx_rings != bp->tx_nr_rings &&
	    bp->hwrm_spec_code >= 0x10601)
		return true;

	if (!BNXT_NEW_RM(bp))
		return false;

	vnic = bnxt_get_total_vnics(bp, rx);

	if (bp->flags & BNXT_FLAG_AGG_RINGS)
		rx <<= 1;
	stat = bnxt_get_func_stat_ctxs(bp);
	if (hw_resc->resv_rx_rings != rx || hw_resc->resv_cp_rings != cp ||
	    hw_resc->resv_vnics != vnic || hw_resc->resv_stat_ctxs != stat ||
	    (hw_resc->resv_hw_ring_grps != grp &&
	     !(bp->flags & BNXT_FLAG_CHIP_P5_PLUS)))
		return true;
	if ((bp->flags & BNXT_FLAG_CHIP_P5_PLUS) && BNXT_PF(bp) &&
	    hw_resc->resv_irqs != nq)
		return true;
	return false;
}
