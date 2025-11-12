struct ns_common *get_net_ns(struct ns_common *ns)
{
	return &get_net(container_of(ns, struct net, ns))->ns;
}
