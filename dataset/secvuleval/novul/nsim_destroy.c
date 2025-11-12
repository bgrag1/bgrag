void nsim_destroy(struct netdevsim *ns)
{
	struct net_device *dev = ns->netdev;

	rtnl_lock();
	unregister_netdevice(dev);
	if (nsim_dev_port_is_pf(ns->nsim_dev_port)) {
		nsim_macsec_teardown(ns);
		nsim_ipsec_teardown(ns);
		nsim_bpf_uninit(ns);
	}
	rtnl_unlock();
	if (nsim_dev_port_is_pf(ns->nsim_dev_port))
		nsim_exit_netdevsim(ns);
	free_netdev(dev);
}
