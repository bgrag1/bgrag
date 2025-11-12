static void intel_mst_enable_dp(struct intel_atomic_state *state,
				struct intel_encoder *encoder,
				const struct intel_crtc_state *pipe_config,
				const struct drm_connector_state *conn_state)
{
	struct intel_dp_mst_encoder *intel_mst = enc_to_mst(encoder);
	struct intel_digital_port *dig_port = intel_mst->primary;
	struct intel_dp *intel_dp = &dig_port->dp;
	struct intel_connector *connector = to_intel_connector(conn_state->connector);
	struct drm_i915_private *dev_priv = to_i915(encoder->base.dev);
	struct drm_dp_mst_topology_state *mst_state =
		drm_atomic_get_new_mst_topology_state(&state->base, &intel_dp->mst_mgr);
	enum transcoder trans = pipe_config->cpu_transcoder;
	bool first_mst_stream = intel_dp->active_mst_links == 1;
	struct intel_crtc *pipe_crtc;

	drm_WARN_ON(&dev_priv->drm, pipe_config->has_pch_encoder);

	if (intel_dp_is_uhbr(pipe_config)) {
		const struct drm_display_mode *adjusted_mode =
			&pipe_config->hw.adjusted_mode;
		u64 crtc_clock_hz = KHz(adjusted_mode->crtc_clock);

		intel_de_write(dev_priv, TRANS_DP2_VFREQHIGH(pipe_config->cpu_transcoder),
			       TRANS_DP2_VFREQ_PIXEL_CLOCK(crtc_clock_hz >> 24));
		intel_de_write(dev_priv, TRANS_DP2_VFREQLOW(pipe_config->cpu_transcoder),
			       TRANS_DP2_VFREQ_PIXEL_CLOCK(crtc_clock_hz & 0xffffff));
	}

	enable_bs_jitter_was(pipe_config);

	intel_ddi_enable_transcoder_func(encoder, pipe_config);

	clear_act_sent(encoder, pipe_config);

	intel_de_rmw(dev_priv, TRANS_DDI_FUNC_CTL(trans), 0,
		     TRANS_DDI_DP_VC_PAYLOAD_ALLOC);

	drm_dbg_kms(&dev_priv->drm, "active links %d\n",
		    intel_dp->active_mst_links);

	wait_for_act_sent(encoder, pipe_config);

	if (first_mst_stream)
		intel_ddi_wait_for_fec_status(encoder, pipe_config, true);

	drm_dp_add_payload_part2(&intel_dp->mst_mgr,
				 drm_atomic_get_mst_payload_state(mst_state, connector->port));

	if (DISPLAY_VER(dev_priv) >= 12)
		intel_de_rmw(dev_priv, hsw_chicken_trans_reg(dev_priv, trans),
			     FECSTALL_DIS_DPTSTREAM_DPTTG,
			     pipe_config->fec_enable ? FECSTALL_DIS_DPTSTREAM_DPTTG : 0);

	intel_audio_sdp_split_update(pipe_config);

	intel_enable_transcoder(pipe_config);

	for_each_intel_crtc_in_pipe_mask_reverse(&dev_priv->drm, pipe_crtc,
						 intel_crtc_joined_pipe_mask(pipe_config)) {
		const struct intel_crtc_state *pipe_crtc_state =
			intel_atomic_get_new_crtc_state(state, pipe_crtc);

		intel_crtc_vblank_on(pipe_crtc_state);
	}

	intel_hdcp_enable(state, encoder, pipe_config, conn_state);
}
