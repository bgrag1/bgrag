static void br_mst_vlan_set_state(struct net_bridge_port *p, struct net_bridge_vlan *v,
				  u8 state)
{
	struct net_bridge_vlan_group *vg = nbp_vlan_group(p);

	if (v->state == state)
		return;

	br_vlan_set_state(v, state);

	if (v->vid == vg->pvid)
		br_vlan_set_pvid_state(vg, state);
}
