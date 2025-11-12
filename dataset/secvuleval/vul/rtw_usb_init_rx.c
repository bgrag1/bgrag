static int rtw_usb_init_rx(struct rtw_dev *rtwdev)
{
	struct rtw_usb *rtwusb = rtw_get_usb_priv(rtwdev);
	int i;

	rtwusb->rxwq = create_singlethread_workqueue("rtw88_usb: rx wq");
	if (!rtwusb->rxwq) {
		rtw_err(rtwdev, "failed to create RX work queue\n");
		return -ENOMEM;
	}

	skb_queue_head_init(&rtwusb->rx_queue);

	INIT_WORK(&rtwusb->rx_work, rtw_usb_rx_handler);

	for (i = 0; i < RTW_USB_RXCB_NUM; i++) {
		struct rx_usb_ctrl_block *rxcb = &rtwusb->rx_cb[i];

		rtw_usb_rx_resubmit(rtwusb, rxcb);
	}

	return 0;
}
