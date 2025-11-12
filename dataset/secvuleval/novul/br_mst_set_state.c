int br_mst_set_state(struct net_bridge_port *p, u16 msti, u8 state,
		     struct netlink_ext_ack *extack)
{
	struct switchdev_attr attr = {
		.id = SWITCHDEV_ATTR_ID_PORT_MST_STATE,
		.orig_dev = p->dev,
		.u.mst_state = {
			.msti = msti,
			.state = state,
		},
	};
	struct net_bridge_vlan_group *vg;
	struct net_bridge_vlan *v;
	int err = 0;

	rcu_read_lock();
	vg = nbp_vlan_group(p);
	if (!vg)
		goto out;

	/* MSTI 0 (CST) state changes are notified via the regular
	 * SWITCHDEV_ATTR_ID_PORT_STP_STATE.
	 */
	if (msti) {
		err = switchdev_port_attr_set(p->dev, &attr, extack);
		if (err && err != -EOPNOTSUPP)
			goto out;
	}

	err = 0;
	list_for_each_entry_rcu(v, &vg->vlan_list, vlist) {
		if (v->brvlan->msti != msti)
			continue;

		br_mst_vlan_set_state(p, v, state);
	}

out:
	rcu_read_unlock();
	return err;
}
