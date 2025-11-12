static void ingress_destroy(struct Qdisc *sch)
{
	struct ingress_sched_data *q = qdisc_priv(sch);
	struct net_device *dev = qdisc_dev(sch);
	struct bpf_mprog_entry *entry = rtnl_dereference(dev->tcx_ingress);

	if (sch->parent != TC_H_INGRESS)
		return;

	tcf_block_put_ext(q->block, sch, &q->block_info);

	if (entry) {
		tcx_miniq_set_active(entry, false);
		if (!tcx_entry_is_active(entry)) {
			tcx_entry_update(dev, NULL, true);
			tcx_entry_free(entry);
		}
	}

	net_dec_ingress_queue();
}
