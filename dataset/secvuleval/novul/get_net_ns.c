struct ns_common *get_net_ns(struct ns_common *ns)
{
	struct net *net;

	net = maybe_get_net(container_of(ns, struct net, ns));
	if (net)
		return &net->ns;
	return ERR_PTR(-EINVAL);
}
