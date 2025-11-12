void hclge_ptp_get_rx_hwts(struct hnae3_handle *handle, struct sk_buff *skb,
			   u32 nsec, u32 sec)
{
	struct hclge_vport *vport = hclge_get_vport(handle);
	struct hclge_dev *hdev = vport->back;
	unsigned long flags;
	u64 ns = nsec;
	u32 sec_h;

	if (!hdev->ptp || !test_bit(HCLGE_PTP_FLAG_RX_EN, &hdev->ptp->flags))
		return;

	/* Since the BD does not have enough space for the higher 16 bits of
	 * second, and this part will not change frequently, so read it
	 * from register.
	 */
	spin_lock_irqsave(&hdev->ptp->lock, flags);
	sec_h = readl(hdev->ptp->io_base + HCLGE_PTP_CUR_TIME_SEC_H_REG);
	spin_unlock_irqrestore(&hdev->ptp->lock, flags);

	ns += (((u64)sec_h) << HCLGE_PTP_SEC_H_OFFSET | sec) * NSEC_PER_SEC;
	skb_hwtstamps(skb)->hwtstamp = ns_to_ktime(ns);
	hdev->ptp->last_rx = jiffies;
	hdev->ptp->rx_cnt++;
}
