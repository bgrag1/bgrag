static void dpu_encoder_virt_atomic_mode_set(struct drm_encoder *drm_enc,
					     struct drm_crtc_state *crtc_state,
					     struct drm_connector_state *conn_state)
{
	struct dpu_encoder_virt *dpu_enc;
	struct msm_drm_private *priv;
	struct dpu_kms *dpu_kms;
	struct dpu_crtc_state *cstate;
	struct dpu_global_state *global_state;
	struct dpu_hw_blk *hw_pp[MAX_CHANNELS_PER_ENC];
	struct dpu_hw_blk *hw_ctl[MAX_CHANNELS_PER_ENC];
	struct dpu_hw_blk *hw_lm[MAX_CHANNELS_PER_ENC];
	struct dpu_hw_blk *hw_dspp[MAX_CHANNELS_PER_ENC] = { NULL };
	struct dpu_hw_blk *hw_dsc[MAX_CHANNELS_PER_ENC];
	int num_lm, num_ctl, num_pp, num_dsc;
	unsigned int dsc_mask = 0;
	int i;

	if (!drm_enc) {
		DPU_ERROR("invalid encoder\n");
		return;
	}

	dpu_enc = to_dpu_encoder_virt(drm_enc);
	DPU_DEBUG_ENC(dpu_enc, "\n");

	priv = drm_enc->dev->dev_private;
	dpu_kms = to_dpu_kms(priv->kms);

	global_state = dpu_kms_get_existing_global_state(dpu_kms);
	if (IS_ERR_OR_NULL(global_state)) {
		DPU_ERROR("Failed to get global state");
		return;
	}

	trace_dpu_enc_mode_set(DRMID(drm_enc));

	/* Query resource that have been reserved in atomic check step. */
	num_pp = dpu_rm_get_assigned_resources(&dpu_kms->rm, global_state,
		drm_enc->base.id, DPU_HW_BLK_PINGPONG, hw_pp,
		ARRAY_SIZE(hw_pp));
	num_ctl = dpu_rm_get_assigned_resources(&dpu_kms->rm, global_state,
		drm_enc->base.id, DPU_HW_BLK_CTL, hw_ctl, ARRAY_SIZE(hw_ctl));
	num_lm = dpu_rm_get_assigned_resources(&dpu_kms->rm, global_state,
		drm_enc->base.id, DPU_HW_BLK_LM, hw_lm, ARRAY_SIZE(hw_lm));
	dpu_rm_get_assigned_resources(&dpu_kms->rm, global_state,
		drm_enc->base.id, DPU_HW_BLK_DSPP, hw_dspp,
		ARRAY_SIZE(hw_dspp));

	for (i = 0; i < MAX_CHANNELS_PER_ENC; i++)
		dpu_enc->hw_pp[i] = i < num_pp ? to_dpu_hw_pingpong(hw_pp[i])
						: NULL;

	num_dsc = dpu_rm_get_assigned_resources(&dpu_kms->rm, global_state,
						drm_enc->base.id, DPU_HW_BLK_DSC,
						hw_dsc, ARRAY_SIZE(hw_dsc));
	for (i = 0; i < num_dsc; i++) {
		dpu_enc->hw_dsc[i] = to_dpu_hw_dsc(hw_dsc[i]);
		dsc_mask |= BIT(dpu_enc->hw_dsc[i]->idx - DSC_0);
	}

	dpu_enc->dsc_mask = dsc_mask;

	if ((dpu_enc->disp_info.intf_type == INTF_WB && conn_state->writeback_job) ||
	    dpu_enc->disp_info.intf_type == INTF_DP) {
		struct dpu_hw_blk *hw_cdm = NULL;

		dpu_rm_get_assigned_resources(&dpu_kms->rm, global_state,
					      drm_enc->base.id, DPU_HW_BLK_CDM,
					      &hw_cdm, 1);
		dpu_enc->cur_master->hw_cdm = hw_cdm ? to_dpu_hw_cdm(hw_cdm) : NULL;
	}

	cstate = to_dpu_crtc_state(crtc_state);

	for (i = 0; i < num_lm; i++) {
		int ctl_idx = (i < num_ctl) ? i : (num_ctl-1);

		cstate->mixers[i].hw_lm = to_dpu_hw_mixer(hw_lm[i]);
		cstate->mixers[i].lm_ctl = to_dpu_hw_ctl(hw_ctl[ctl_idx]);
		cstate->mixers[i].hw_dspp = to_dpu_hw_dspp(hw_dspp[i]);
	}

	cstate->num_mixers = num_lm;

	dpu_enc->connector = conn_state->connector;

	for (i = 0; i < dpu_enc->num_phys_encs; i++) {
		struct dpu_encoder_phys *phys = dpu_enc->phys_encs[i];

		if (!dpu_enc->hw_pp[i]) {
			DPU_ERROR_ENC(dpu_enc,
				"no pp block assigned at idx: %d\n", i);
			return;
		}

		if (!hw_ctl[i]) {
			DPU_ERROR_ENC(dpu_enc,
				"no ctl block assigned at idx: %d\n", i);
			return;
		}

		phys->hw_pp = dpu_enc->hw_pp[i];
		phys->hw_ctl = to_dpu_hw_ctl(hw_ctl[i]);

		phys->cached_mode = crtc_state->adjusted_mode;
		if (phys->ops.atomic_mode_set)
			phys->ops.atomic_mode_set(phys, crtc_state, conn_state);
	}
}
