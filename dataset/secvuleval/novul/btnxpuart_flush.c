static int btnxpuart_flush(struct hci_dev *hdev)
{
	struct btnxpuart_dev *nxpdev = hci_get_drvdata(hdev);

	/* Flush any pending characters */
	serdev_device_write_flush(nxpdev->serdev);
	skb_queue_purge(&nxpdev->txq);

	cancel_work_sync(&nxpdev->tx_work);

	if (!IS_ERR_OR_NULL(nxpdev->rx_skb)) {
		kfree_skb(nxpdev->rx_skb);
		nxpdev->rx_skb = NULL;
	}

	return 0;
}
