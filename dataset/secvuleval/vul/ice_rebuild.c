static void ice_rebuild(struct ice_pf *pf, enum ice_reset_req reset_type)
{
	struct device *dev = ice_pf_to_dev(pf);
	struct ice_hw *hw = &pf->hw;
	bool dvm;
	int err;

	if (test_bit(ICE_DOWN, pf->state))
		goto clear_recovery;

	dev_dbg(dev, "rebuilding PF after reset_type=%d\n", reset_type);

#define ICE_EMP_RESET_SLEEP_MS 5000
	if (reset_type == ICE_RESET_EMPR) {
		/* If an EMP reset has occurred, any previously pending flash
		 * update will have completed. We no longer know whether or
		 * not the NVM update EMP reset is restricted.
		 */
		pf->fw_emp_reset_disabled = false;

		msleep(ICE_EMP_RESET_SLEEP_MS);
	}

	err = ice_init_all_ctrlq(hw);
	if (err) {
		dev_err(dev, "control queues init failed %d\n", err);
		goto err_init_ctrlq;
	}

	/* if DDP was previously loaded successfully */
	if (!ice_is_safe_mode(pf)) {
		/* reload the SW DB of filter tables */
		if (reset_type == ICE_RESET_PFR)
			ice_fill_blk_tbls(hw);
		else
			/* Reload DDP Package after CORER/GLOBR reset */
			ice_load_pkg(NULL, pf);
	}

	err = ice_clear_pf_cfg(hw);
	if (err) {
		dev_err(dev, "clear PF configuration failed %d\n", err);
		goto err_init_ctrlq;
	}

	ice_clear_pxe_mode(hw);

	err = ice_init_nvm(hw);
	if (err) {
		dev_err(dev, "ice_init_nvm failed %d\n", err);
		goto err_init_ctrlq;
	}

	err = ice_get_caps(hw);
	if (err) {
		dev_err(dev, "ice_get_caps failed %d\n", err);
		goto err_init_ctrlq;
	}

	err = ice_aq_set_mac_cfg(hw, ICE_AQ_SET_MAC_FRAME_SIZE_MAX, NULL);
	if (err) {
		dev_err(dev, "set_mac_cfg failed %d\n", err);
		goto err_init_ctrlq;
	}

	dvm = ice_is_dvm_ena(hw);

	err = ice_aq_set_port_params(pf->hw.port_info, dvm, NULL);
	if (err)
		goto err_init_ctrlq;

	err = ice_sched_init_port(hw->port_info);
	if (err)
		goto err_sched_init_port;

	/* start misc vector */
	err = ice_req_irq_msix_misc(pf);
	if (err) {
		dev_err(dev, "misc vector setup failed: %d\n", err);
		goto err_sched_init_port;
	}

	if (test_bit(ICE_FLAG_FD_ENA, pf->flags)) {
		wr32(hw, PFQF_FD_ENA, PFQF_FD_ENA_FD_ENA_M);
		if (!rd32(hw, PFQF_FD_SIZE)) {
			u16 unused, guar, b_effort;

			guar = hw->func_caps.fd_fltr_guar;
			b_effort = hw->func_caps.fd_fltr_best_effort;

			/* force guaranteed filter pool for PF */
			ice_alloc_fd_guar_item(hw, &unused, guar);
			/* force shared filter pool for PF */
			ice_alloc_fd_shrd_item(hw, &unused, b_effort);
		}
	}

	if (test_bit(ICE_FLAG_DCB_ENA, pf->flags))
		ice_dcb_rebuild(pf);

	/* If the PF previously had enabled PTP, PTP init needs to happen before
	 * the VSI rebuild. If not, this causes the PTP link status events to
	 * fail.
	 */
	if (test_bit(ICE_FLAG_PTP_SUPPORTED, pf->flags))
		ice_ptp_rebuild(pf, reset_type);

	if (ice_is_feature_supported(pf, ICE_F_GNSS))
		ice_gnss_init(pf);

	/* rebuild PF VSI */
	err = ice_vsi_rebuild_by_type(pf, ICE_VSI_PF);
	if (err) {
		dev_err(dev, "PF VSI rebuild failed: %d\n", err);
		goto err_vsi_rebuild;
	}

	if (reset_type == ICE_RESET_PFR) {
		err = ice_rebuild_channels(pf);
		if (err) {
			dev_err(dev, "failed to rebuild and replay ADQ VSIs, err %d\n",
				err);
			goto err_vsi_rebuild;
		}
	}

	/* If Flow Director is active */
	if (test_bit(ICE_FLAG_FD_ENA, pf->flags)) {
		err = ice_vsi_rebuild_by_type(pf, ICE_VSI_CTRL);
		if (err) {
			dev_err(dev, "control VSI rebuild failed: %d\n", err);
			goto err_vsi_rebuild;
		}

		/* replay HW Flow Director recipes */
		if (hw->fdir_prof)
			ice_fdir_replay_flows(hw);

		/* replay Flow Director filters */
		ice_fdir_replay_fltrs(pf);

		ice_rebuild_arfs(pf);
	}

	ice_update_pf_netdev_link(pf);

	/* tell the firmware we are up */
	err = ice_send_version(pf);
	if (err) {
		dev_err(dev, "Rebuild failed due to error sending driver version: %d\n",
			err);
		goto err_vsi_rebuild;
	}

	ice_replay_post(hw);

	/* if we get here, reset flow is successful */
	clear_bit(ICE_RESET_FAILED, pf->state);

	ice_plug_aux_dev(pf);
	if (ice_is_feature_supported(pf, ICE_F_SRIOV_LAG))
		ice_lag_rebuild(pf);

	/* Restore timestamp mode settings after VSI rebuild */
	ice_ptp_restore_timestamp_mode(pf);
	return;

err_vsi_rebuild:
err_sched_init_port:
	ice_sched_cleanup_all(hw);
err_init_ctrlq:
	ice_shutdown_all_ctrlq(hw, false);
	set_bit(ICE_RESET_FAILED, pf->state);
clear_recovery:
	/* set this bit in PF state to control service task scheduling */
	set_bit(ICE_NEEDS_RESTART, pf->state);
	dev_err(dev, "Rebuild failed, unload and reload driver\n");
}
