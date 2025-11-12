int ice_vsi_rebuild(struct ice_vsi *vsi, u32 vsi_flags)
{
	struct ice_coalesce_stored *coalesce;
	int prev_num_q_vectors;
	struct ice_pf *pf;
	int ret;

	if (!vsi)
		return -EINVAL;

	vsi->flags = vsi_flags;
	pf = vsi->back;
	if (WARN_ON(vsi->type == ICE_VSI_VF && !vsi->vf))
		return -EINVAL;

	mutex_lock(&vsi->xdp_state_lock);

	ret = ice_vsi_realloc_stat_arrays(vsi);
	if (ret)
		goto unlock;

	ice_vsi_decfg(vsi);
	ret = ice_vsi_cfg_def(vsi);
	if (ret)
		goto unlock;

	coalesce = kcalloc(vsi->num_q_vectors,
			   sizeof(struct ice_coalesce_stored), GFP_KERNEL);
	if (!coalesce) {
		ret = -ENOMEM;
		goto decfg;
	}

	prev_num_q_vectors = ice_vsi_rebuild_get_coalesce(vsi, coalesce);

	ret = ice_vsi_cfg_tc_lan(pf, vsi);
	if (ret) {
		if (vsi_flags & ICE_VSI_FLAG_INIT) {
			ret = -EIO;
			goto free_coalesce;
		}

		ret = ice_schedule_reset(pf, ICE_RESET_PFR);
		goto free_coalesce;
	}

	ice_vsi_rebuild_set_coalesce(vsi, coalesce, prev_num_q_vectors);
	clear_bit(ICE_VSI_REBUILD_PENDING, vsi->state);

free_coalesce:
	kfree(coalesce);
decfg:
	if (ret)
		ice_vsi_decfg(vsi);
unlock:
	mutex_unlock(&vsi->xdp_state_lock);
	return ret;
}
