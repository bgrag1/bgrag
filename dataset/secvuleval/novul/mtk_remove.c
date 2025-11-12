static void mtk_remove(struct platform_device *pdev)
{
	struct mtk_eth *eth = platform_get_drvdata(pdev);
	struct mtk_mac *mac;
	int i;

	/* stop all devices to make sure that dma is properly shut down */
	for (i = 0; i < MTK_MAX_DEVS; i++) {
		if (!eth->netdev[i])
			continue;
		mtk_stop(eth->netdev[i]);
		mac = netdev_priv(eth->netdev[i]);
		phylink_disconnect_phy(mac->phylink);
	}

	mtk_wed_exit();
	mtk_hw_deinit(eth);

	netif_napi_del(&eth->tx_napi);
	netif_napi_del(&eth->rx_napi);
	mtk_cleanup(eth);
	free_netdev(eth->dummy_dev);
	mtk_mdio_cleanup(eth);
}
