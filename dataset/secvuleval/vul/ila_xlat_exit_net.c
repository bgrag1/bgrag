void ila_xlat_exit_net(struct net *net)
{
	struct ila_net *ilan = net_generic(net, ila_net_id);

	rhashtable_free_and_destroy(&ilan->xlat.rhash_table, ila_free_cb, NULL);

	free_bucket_spinlocks(ilan->xlat.locks);

	if (ilan->xlat.hooks_registered)
		nf_unregister_net_hooks(net, ila_nf_hook_ops,
					ARRAY_SIZE(ila_nf_hook_ops));
}
