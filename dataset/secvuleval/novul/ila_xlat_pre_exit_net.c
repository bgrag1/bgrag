void ila_xlat_pre_exit_net(struct net *net)
{
	struct ila_net *ilan = net_generic(net, ila_net_id);

	if (ilan->xlat.hooks_registered)
		nf_unregister_net_hooks(net, ila_nf_hook_ops,
					ARRAY_SIZE(ila_nf_hook_ops));
}
