static void
nv50_msto_cleanup(struct drm_atomic_state *state,
		  struct drm_dp_mst_topology_state *new_mst_state,
		  struct drm_dp_mst_topology_mgr *mgr,
		  struct nv50_msto *msto)
{
	struct nouveau_drm *drm = nouveau_drm(msto->encoder.dev);
	struct drm_dp_mst_atomic_payload *new_payload =
		drm_atomic_get_mst_payload_state(new_mst_state, msto->mstc->port);
	struct drm_dp_mst_topology_state *old_mst_state =
		drm_atomic_get_old_mst_topology_state(state, mgr);
	const struct drm_dp_mst_atomic_payload *old_payload =
		drm_atomic_get_mst_payload_state(old_mst_state, msto->mstc->port);
	struct nv50_mstc *mstc = msto->mstc;
	struct nv50_mstm *mstm = mstc->mstm;

	NV_ATOMIC(drm, "%s: msto cleanup\n", msto->encoder.name);

	if (msto->disabled) {
		if (msto->head->func->display_id) {
			nvif_outp_dp_mst_id_put(&mstm->outp->outp, msto->display_id);
			msto->display_id = 0;
		}

		msto->mstc = NULL;
		msto->disabled = false;
		drm_dp_remove_payload_part2(mgr, new_mst_state, old_payload, new_payload);
	} else if (msto->enabled) {
		drm_dp_add_payload_part2(mgr, new_payload);
		msto->enabled = false;
	}
}
