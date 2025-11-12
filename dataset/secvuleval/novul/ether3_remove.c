static void ether3_remove(struct expansion_card *ec)
{
	struct net_device *dev = ecard_get_drvdata(ec);

	ether3_outw(priv(dev)->regs.config2 |= CFG2_CTRLO, REG_CONFIG2);
	ecard_set_drvdata(ec, NULL);

	unregister_netdev(dev);
	del_timer_sync(&priv(dev)->timer);
	free_netdev(dev);
	ecard_release_resources(ec);
}
