static void
ice_prepare_for_reset(struct ice_pf *pf, enum ice_reset_req reset_type)
{
	struct ice_hw *hw = &pf->hw;
	struct ice_vsi *vsi;
	struct ice_vf *vf;
	unsigned int bkt;

	dev_dbg(ice_pf_to_dev(pf), "reset_type=%d\n", reset_type);

	/* already prepared for reset */
	if (test_bit(ICE_PREPARED_FOR_RESET, pf->state))
		return;

	synchronize_irq(pf->oicr_irq.virq);

	ice_unplug_aux_dev(pf);

	/* Notify VFs of impending reset */
	if (ice_check_sq_alive(hw, &hw->mailboxq))
		ice_vc_notify_reset(pf);

	/* Disable VFs until reset is completed */
	mutex_lock(&pf->vfs.table_lock);
	ice_for_each_vf(pf, bkt, vf)
		ice_set_vf_state_dis(vf);
	mutex_unlock(&pf->vfs.table_lock);

	if (ice_is_eswitch_mode_switchdev(pf)) {
		if (reset_type != ICE_RESET_PFR)
			ice_clear_sw_switch_recipes(pf);
	}

	/* release ADQ specific HW and SW resources */
	vsi = ice_get_main_vsi(pf);
	if (!vsi)
		goto skip;

	/* to be on safe side, reset orig_rss_size so that normal flow
	 * of deciding rss_size can take precedence
	 */
	vsi->orig_rss_size = 0;

	if (test_bit(ICE_FLAG_TC_MQPRIO, pf->flags)) {
		if (reset_type == ICE_RESET_PFR) {
			vsi->old_ena_tc = vsi->all_enatc;
			vsi->old_numtc = vsi->all_numtc;
		} else {
			ice_remove_q_channels(vsi, true);

			/* for other reset type, do not support channel rebuild
			 * hence reset needed info
			 */
			vsi->old_ena_tc = 0;
			vsi->all_enatc = 0;
			vsi->old_numtc = 0;
			vsi->all_numtc = 0;
			vsi->req_txq = 0;
			vsi->req_rxq = 0;
			clear_bit(ICE_FLAG_TC_MQPRIO, pf->flags);
			memset(&vsi->mqprio_qopt, 0, sizeof(vsi->mqprio_qopt));
		}
	}

	if (vsi->netdev)
		netif_device_detach(vsi->netdev);
skip:

	/* clear SW filtering DB */
	ice_clear_hw_tbls(hw);
	/* disable the VSIs and their queues that are not already DOWN */
	set_bit(ICE_VSI_REBUILD_PENDING, ice_get_main_vsi(pf)->state);
	ice_pf_dis_all_vsi(pf, false);

	if (test_bit(ICE_FLAG_PTP_SUPPORTED, pf->flags))
		ice_ptp_prepare_for_reset(pf, reset_type);

	if (ice_is_feature_supported(pf, ICE_F_GNSS))
		ice_gnss_exit(pf);

	if (hw->port_info)
		ice_sched_clear_port(hw->port_info);

	ice_shutdown_all_ctrlq(hw, false);

	set_bit(ICE_PREPARED_FOR_RESET, pf->state);
}
