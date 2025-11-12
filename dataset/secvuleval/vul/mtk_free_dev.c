static int mtk_free_dev(struct mtk_eth *eth)
{
	int i;

	for (i = 0; i < MTK_MAX_DEVS; i++) {
		if (!eth->netdev[i])
			continue;
		free_netdev(eth->netdev[i]);
	}

	for (i = 0; i < ARRAY_SIZE(eth->dsa_meta); i++) {
		if (!eth->dsa_meta[i])
			break;
		metadata_dst_free(eth->dsa_meta[i]);
	}

	free_netdev(eth->dummy_dev);

	return 0;
}
