void dm_helpers_dp_mst_send_payload_allocation(
		struct dc_context *ctx,
		const struct dc_stream_state *stream)
{
	struct amdgpu_dm_connector *aconnector;
	struct drm_dp_mst_topology_state *mst_state;
	struct drm_dp_mst_topology_mgr *mst_mgr;
	struct drm_dp_mst_atomic_payload *new_payload;
	enum mst_progress_status set_flag = MST_ALLOCATE_NEW_PAYLOAD;
	enum mst_progress_status clr_flag = MST_CLEAR_ALLOCATED_PAYLOAD;
	int ret = 0;

	aconnector = (struct amdgpu_dm_connector *)stream->dm_stream_context;

	if (!aconnector || !aconnector->mst_root)
		return;

	mst_mgr = &aconnector->mst_root->mst_mgr;
	mst_state = to_drm_dp_mst_topology_state(mst_mgr->base.state);
	new_payload = drm_atomic_get_mst_payload_state(mst_state, aconnector->mst_output_port);

	ret = drm_dp_add_payload_part2(mst_mgr, mst_state->base.state, new_payload);

	if (ret) {
		amdgpu_dm_set_mst_status(&aconnector->mst_status,
			set_flag, false);
	} else {
		amdgpu_dm_set_mst_status(&aconnector->mst_status,
			set_flag, true);
		amdgpu_dm_set_mst_status(&aconnector->mst_status,
			clr_flag, false);
	}
}
