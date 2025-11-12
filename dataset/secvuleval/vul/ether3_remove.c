static void ether3_remove(struct expansion_card *ec)
{
	struct net_device *dev = ecard_get_drvdata(ec);

	ecard_set_drvdata(ec, NULL);

	unregister_netdev(dev);
	free_netdev(dev);
	ecard_release_resources(ec);
}
