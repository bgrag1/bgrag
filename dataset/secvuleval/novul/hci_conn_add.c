struct hci_conn *hci_conn_add(struct hci_dev *hdev, int type, bdaddr_t *dst,
			      u8 role, u16 handle)
{
	struct hci_conn *conn;

	switch (type) {
	case ACL_LINK:
		if (!hdev->acl_mtu)
			return ERR_PTR(-ECONNREFUSED);
		break;
	case ISO_LINK:
		if (hdev->iso_mtu)
			/* Dedicated ISO Buffer exists */
			break;
		fallthrough;
	case LE_LINK:
		if (hdev->le_mtu && hdev->le_mtu < HCI_MIN_LE_MTU)
			return ERR_PTR(-ECONNREFUSED);
		if (!hdev->le_mtu && hdev->acl_mtu < HCI_MIN_LE_MTU)
			return ERR_PTR(-ECONNREFUSED);
		break;
	case SCO_LINK:
	case ESCO_LINK:
		if (!hdev->sco_pkts)
			/* Controller does not support SCO or eSCO over HCI */
			return ERR_PTR(-ECONNREFUSED);
		break;
	default:
		return ERR_PTR(-ECONNREFUSED);
	}

	bt_dev_dbg(hdev, "dst %pMR handle 0x%4.4x", dst, handle);

	conn = kzalloc(sizeof(*conn), GFP_KERNEL);
	if (!conn)
		return ERR_PTR(-ENOMEM);

	bacpy(&conn->dst, dst);
	bacpy(&conn->src, &hdev->bdaddr);
	conn->handle = handle;
	conn->hdev  = hdev;
	conn->type  = type;
	conn->role  = role;
	conn->mode  = HCI_CM_ACTIVE;
	conn->state = BT_OPEN;
	conn->auth_type = HCI_AT_GENERAL_BONDING;
	conn->io_capability = hdev->io_capability;
	conn->remote_auth = 0xff;
	conn->key_type = 0xff;
	conn->rssi = HCI_RSSI_INVALID;
	conn->tx_power = HCI_TX_POWER_INVALID;
	conn->max_tx_power = HCI_TX_POWER_INVALID;
	conn->sync_handle = HCI_SYNC_HANDLE_INVALID;

	set_bit(HCI_CONN_POWER_SAVE, &conn->flags);
	conn->disc_timeout = HCI_DISCONN_TIMEOUT;

	/* Set Default Authenticated payload timeout to 30s */
	conn->auth_payload_timeout = DEFAULT_AUTH_PAYLOAD_TIMEOUT;

	if (conn->role == HCI_ROLE_MASTER)
		conn->out = true;

	switch (type) {
	case ACL_LINK:
		conn->pkt_type = hdev->pkt_type & ACL_PTYPE_MASK;
		conn->mtu = hdev->acl_mtu;
		break;
	case LE_LINK:
		/* conn->src should reflect the local identity address */
		hci_copy_identity_address(hdev, &conn->src, &conn->src_type);
		conn->mtu = hdev->le_mtu ? hdev->le_mtu : hdev->acl_mtu;
		break;
	case ISO_LINK:
		/* conn->src should reflect the local identity address */
		hci_copy_identity_address(hdev, &conn->src, &conn->src_type);

		/* set proper cleanup function */
		if (!bacmp(dst, BDADDR_ANY))
			conn->cleanup = bis_cleanup;
		else if (conn->role == HCI_ROLE_MASTER)
			conn->cleanup = cis_cleanup;

		conn->mtu = hdev->iso_mtu ? hdev->iso_mtu :
			    hdev->le_mtu ? hdev->le_mtu : hdev->acl_mtu;
		break;
	case SCO_LINK:
		if (lmp_esco_capable(hdev))
			conn->pkt_type = (hdev->esco_type & SCO_ESCO_MASK) |
					(hdev->esco_type & EDR_ESCO_MASK);
		else
			conn->pkt_type = hdev->pkt_type & SCO_PTYPE_MASK;

		conn->mtu = hdev->sco_mtu;
		break;
	case ESCO_LINK:
		conn->pkt_type = hdev->esco_type & ~EDR_ESCO_MASK;
		conn->mtu = hdev->sco_mtu;
		break;
	}

	skb_queue_head_init(&conn->data_q);

	INIT_LIST_HEAD(&conn->chan_list);
	INIT_LIST_HEAD(&conn->link_list);

	INIT_DELAYED_WORK(&conn->disc_work, hci_conn_timeout);
	INIT_DELAYED_WORK(&conn->auto_accept_work, hci_conn_auto_accept);
	INIT_DELAYED_WORK(&conn->idle_work, hci_conn_idle);
	INIT_DELAYED_WORK(&conn->le_conn_timeout, le_conn_timeout);

	atomic_set(&conn->refcnt, 0);

	hci_dev_hold(hdev);

	hci_conn_hash_add(hdev, conn);

	/* The SCO and eSCO connections will only be notified when their
	 * setup has been completed. This is different to ACL links which
	 * can be notified right away.
	 */
	if (conn->type != SCO_LINK && conn->type != ESCO_LINK) {
		if (hdev->notify)
			hdev->notify(hdev, HCI_NOTIFY_CONN_ADD);
	}

	hci_conn_init_sysfs(conn);

	return conn;
}
