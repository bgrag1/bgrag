static void dpu_encoder_virt_atomic_enable(struct drm_encoder *drm_enc,
					struct drm_atomic_state *state)
{
	struct dpu_encoder_virt *dpu_enc = NULL;
	int ret = 0;
	struct drm_display_mode *cur_mode = NULL;

	dpu_enc = to_dpu_encoder_virt(drm_enc);
	dpu_enc->dsc = dpu_encoder_get_dsc_config(drm_enc);

	atomic_set(&dpu_enc->frame_done_timeout_cnt, 0);

	mutex_lock(&dpu_enc->enc_lock);

	dpu_enc->commit_done_timedout = false;

	dpu_enc->connector = drm_atomic_get_new_connector_for_encoder(state, drm_enc);

	cur_mode = &dpu_enc->base.crtc->state->adjusted_mode;

	dpu_enc->wide_bus_en = dpu_encoder_is_widebus_enabled(drm_enc);

	trace_dpu_enc_enable(DRMID(drm_enc), cur_mode->hdisplay,
			     cur_mode->vdisplay);

	/* always enable slave encoder before master */
	if (dpu_enc->cur_slave && dpu_enc->cur_slave->ops.enable)
		dpu_enc->cur_slave->ops.enable(dpu_enc->cur_slave);

	if (dpu_enc->cur_master && dpu_enc->cur_master->ops.enable)
		dpu_enc->cur_master->ops.enable(dpu_enc->cur_master);

	ret = dpu_encoder_resource_control(drm_enc, DPU_ENC_RC_EVENT_KICKOFF);
	if (ret) {
		DPU_ERROR_ENC(dpu_enc, "dpu resource control failed: %d\n",
				ret);
		goto out;
	}

	_dpu_encoder_virt_enable_helper(drm_enc);

	dpu_enc->enabled = true;

out:
	mutex_unlock(&dpu_enc->enc_lock);
}
