static void bpf_link_free(struct bpf_link *link)
{
	const struct bpf_link_ops *ops = link->ops;
	bool sleepable = false;

	bpf_link_free_id(link->id);
	if (link->prog) {
		sleepable = link->prog->sleepable;
		/* detach BPF program, clean up used resources */
		ops->release(link);
		bpf_prog_put(link->prog);
	}
	if (ops->dealloc_deferred) {
		/* schedule BPF link deallocation; if underlying BPF program
		 * is sleepable, we need to first wait for RCU tasks trace
		 * sync, then go through "classic" RCU grace period
		 */
		if (sleepable)
			call_rcu_tasks_trace(&link->rcu, bpf_link_defer_dealloc_mult_rcu_gp);
		else
			call_rcu(&link->rcu, bpf_link_defer_dealloc_rcu_gp);
	} else if (ops->dealloc)
		ops->dealloc(link);
}
