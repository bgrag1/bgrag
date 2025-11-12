static void clsact_destroy(struct Qdisc *sch)
{
	struct clsact_sched_data *q = qdisc_priv(sch);
	struct net_device *dev = qdisc_dev(sch);
	struct bpf_mprog_entry *ingress_entry = rtnl_dereference(dev->tcx_ingress);
	struct bpf_mprog_entry *egress_entry = rtnl_dereference(dev->tcx_egress);

	if (sch->parent != TC_H_CLSACT)
		return;

	tcf_block_put_ext(q->ingress_block, sch, &q->ingress_block_info);
	tcf_block_put_ext(q->egress_block, sch, &q->egress_block_info);

	if (ingress_entry) {
		tcx_miniq_dec(ingress_entry);
		if (!tcx_entry_is_active(ingress_entry)) {
			tcx_entry_update(dev, NULL, true);
			tcx_entry_free(ingress_entry);
		}
	}

	if (egress_entry) {
		tcx_miniq_dec(egress_entry);
		if (!tcx_entry_is_active(egress_entry)) {
			tcx_entry_update(dev, NULL, false);
			tcx_entry_free(egress_entry);
		}
	}

	net_dec_ingress_queue();
	net_dec_egress_queue();
}
