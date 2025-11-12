int drm_dp_add_payload_part2(struct drm_dp_mst_topology_mgr *mgr,
			     struct drm_dp_mst_atomic_payload *payload)
{
	int ret = 0;

	/* Skip failed payloads */
	if (payload->payload_allocation_status != DRM_DP_MST_PAYLOAD_ALLOCATION_DFP) {
		drm_dbg_kms(mgr->dev, "Part 1 of payload creation for %s failed, skipping part 2\n",
			    payload->port->connector->name);
		return -EIO;
	}

	/* Allocate payload to remote end */
	ret = drm_dp_create_payload_to_remote(mgr, payload);
	if (ret < 0)
		drm_err(mgr->dev, "Step 2 of creating MST payload for %p failed: %d\n",
			payload->port, ret);
	else
		payload->payload_allocation_status = DRM_DP_MST_PAYLOAD_ALLOCATION_REMOTE;

	return ret;
}
