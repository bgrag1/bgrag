static int clsact_init(struct Qdisc *sch, struct nlattr *opt,
		       struct netlink_ext_ack *extack)
{
	struct clsact_sched_data *q = qdisc_priv(sch);
	struct net_device *dev = qdisc_dev(sch);
	struct bpf_mprog_entry *entry;
	bool created;
	int err;

	if (sch->parent != TC_H_CLSACT)
		return -EOPNOTSUPP;

	net_inc_ingress_queue();
	net_inc_egress_queue();

	entry = tcx_entry_fetch_or_create(dev, true, &created);
	if (!entry)
		return -ENOMEM;
	tcx_miniq_inc(entry);
	mini_qdisc_pair_init(&q->miniqp_ingress, sch, &tcx_entry(entry)->miniq);
	if (created)
		tcx_entry_update(dev, entry, true);

	q->ingress_block_info.binder_type = FLOW_BLOCK_BINDER_TYPE_CLSACT_INGRESS;
	q->ingress_block_info.chain_head_change = clsact_chain_head_change;
	q->ingress_block_info.chain_head_change_priv = &q->miniqp_ingress;

	err = tcf_block_get_ext(&q->ingress_block, sch, &q->ingress_block_info,
				extack);
	if (err)
		return err;

	mini_qdisc_pair_block_init(&q->miniqp_ingress, q->ingress_block);

	entry = tcx_entry_fetch_or_create(dev, false, &created);
	if (!entry)
		return -ENOMEM;
	tcx_miniq_inc(entry);
	mini_qdisc_pair_init(&q->miniqp_egress, sch, &tcx_entry(entry)->miniq);
	if (created)
		tcx_entry_update(dev, entry, false);

	q->egress_block_info.binder_type = FLOW_BLOCK_BINDER_TYPE_CLSACT_EGRESS;
	q->egress_block_info.chain_head_change = clsact_chain_head_change;
	q->egress_block_info.chain_head_change_priv = &q->miniqp_egress;

	return tcf_block_get_ext(&q->egress_block, sch, &q->egress_block_info, extack);
}
