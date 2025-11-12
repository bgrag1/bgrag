static bool is_dsc_need_re_compute(
	struct drm_atomic_state *state,
	struct dc_state *dc_state,
	struct dc_link *dc_link)
{
	int i, j;
	bool is_dsc_need_re_compute = false;
	struct amdgpu_dm_connector *stream_on_link[MAX_PIPES];
	int new_stream_on_link_num = 0;
	struct amdgpu_dm_connector *aconnector;
	struct dc_stream_state *stream;
	const struct dc *dc = dc_link->dc;

	/* only check phy used by dsc mst branch */
	if (dc_link->type != dc_connection_mst_branch)
		return false;

	/* add a check for older MST DSC with no virtual DPCDs */
	if (needs_dsc_aux_workaround(dc_link)  &&
		(!(dc_link->dpcd_caps.dsc_caps.dsc_basic_caps.fields.dsc_support.DSC_SUPPORT ||
		dc_link->dpcd_caps.dsc_caps.dsc_basic_caps.fields.dsc_support.DSC_PASSTHROUGH_SUPPORT)))
		return false;

	for (i = 0; i < MAX_PIPES; i++)
		stream_on_link[i] = NULL;

	/* check if there is mode change in new request */
	for (i = 0; i < dc_state->stream_count; i++) {
		struct drm_crtc_state *new_crtc_state;
		struct drm_connector_state *new_conn_state;

		stream = dc_state->streams[i];
		if (!stream)
			continue;

		/* check if stream using the same link for mst */
		if (stream->link != dc_link)
			continue;

		aconnector = (struct amdgpu_dm_connector *) stream->dm_stream_context;
		if (!aconnector || !aconnector->dsc_aux)
			continue;

		stream_on_link[new_stream_on_link_num] = aconnector;
		new_stream_on_link_num++;

		new_conn_state = drm_atomic_get_new_connector_state(state, &aconnector->base);
		if (!new_conn_state)
			continue;

		if (IS_ERR(new_conn_state))
			continue;

		if (!new_conn_state->crtc)
			continue;

		new_crtc_state = drm_atomic_get_new_crtc_state(state, new_conn_state->crtc);
		if (!new_crtc_state)
			continue;

		if (IS_ERR(new_crtc_state))
			continue;

		if (new_crtc_state->enable && new_crtc_state->active) {
			if (new_crtc_state->mode_changed || new_crtc_state->active_changed ||
				new_crtc_state->connectors_changed)
				return true;
		}
	}

	/* check current_state if there stream on link but it is not in
	 * new request state
	 */
	for (i = 0; i < dc->current_state->stream_count; i++) {
		stream = dc->current_state->streams[i];
		/* only check stream on the mst hub */
		if (stream->link != dc_link)
			continue;

		aconnector = (struct amdgpu_dm_connector *)stream->dm_stream_context;
		if (!aconnector)
			continue;

		for (j = 0; j < new_stream_on_link_num; j++) {
			if (stream_on_link[j]) {
				if (aconnector == stream_on_link[j])
					break;
			}
		}

		if (j == new_stream_on_link_num) {
			/* not in new state */
			is_dsc_need_re_compute = true;
			break;
		}
	}

	return is_dsc_need_re_compute;
}
