static int ice_xdp(struct net_device *dev, struct netdev_bpf *xdp)
{
	struct ice_netdev_priv *np = netdev_priv(dev);
	struct ice_vsi *vsi = np->vsi;
	int ret;

	if (vsi->type != ICE_VSI_PF) {
		NL_SET_ERR_MSG_MOD(xdp->extack, "XDP can be loaded only on PF VSI");
		return -EINVAL;
	}

	mutex_lock(&vsi->xdp_state_lock);

	switch (xdp->command) {
	case XDP_SETUP_PROG:
		ret = ice_xdp_setup_prog(vsi, xdp->prog, xdp->extack);
		break;
	case XDP_SETUP_XSK_POOL:
		ret = ice_xsk_pool_setup(vsi, xdp->xsk.pool, xdp->xsk.queue_id);
		break;
	default:
		ret = -EINVAL;
	}

	mutex_unlock(&vsi->xdp_state_lock);
	return ret;
}
