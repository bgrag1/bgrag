int rtw_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct rtw_dev *rtwdev;
	struct ieee80211_hw *hw;
	struct rtw_usb *rtwusb;
	int drv_data_size;
	int ret;

	drv_data_size = sizeof(struct rtw_dev) + sizeof(struct rtw_usb);
	hw = ieee80211_alloc_hw(drv_data_size, &rtw_ops);
	if (!hw)
		return -ENOMEM;

	rtwdev = hw->priv;
	rtwdev->hw = hw;
	rtwdev->dev = &intf->dev;
	rtwdev->chip = (struct rtw_chip_info *)id->driver_info;
	rtwdev->hci.ops = &rtw_usb_ops;
	rtwdev->hci.type = RTW_HCI_TYPE_USB;

	rtwusb = rtw_get_usb_priv(rtwdev);
	rtwusb->rtwdev = rtwdev;

	ret = rtw_usb_alloc_rx_bufs(rtwusb);
	if (ret)
		goto err_release_hw;

	ret = rtw_core_init(rtwdev);
	if (ret)
		goto err_free_rx_bufs;

	ret = rtw_usb_intf_init(rtwdev, intf);
	if (ret) {
		rtw_err(rtwdev, "failed to init USB interface\n");
		goto err_deinit_core;
	}

	ret = rtw_usb_init_tx(rtwdev);
	if (ret) {
		rtw_err(rtwdev, "failed to init USB TX\n");
		goto err_destroy_usb;
	}

	ret = rtw_usb_init_rx(rtwdev);
	if (ret) {
		rtw_err(rtwdev, "failed to init USB RX\n");
		goto err_destroy_txwq;
	}

	ret = rtw_chip_info_setup(rtwdev);
	if (ret) {
		rtw_err(rtwdev, "failed to setup chip information\n");
		goto err_destroy_rxwq;
	}

	ret = rtw_register_hw(rtwdev, rtwdev->hw);
	if (ret) {
		rtw_err(rtwdev, "failed to register hw\n");
		goto err_destroy_rxwq;
	}

	rtw_usb_setup_rx(rtwdev);

	return 0;

err_destroy_rxwq:
	rtw_usb_deinit_rx(rtwdev);

err_destroy_txwq:
	rtw_usb_deinit_tx(rtwdev);

err_destroy_usb:
	rtw_usb_intf_deinit(rtwdev, intf);

err_deinit_core:
	rtw_core_deinit(rtwdev);

err_free_rx_bufs:
	rtw_usb_free_rx_bufs(rtwusb);

err_release_hw:
	ieee80211_free_hw(hw);

	return ret;
}
