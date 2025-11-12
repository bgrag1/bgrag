static int act_establish(struct c4iw_dev *dev, struct sk_buff *skb)
{
	struct c4iw_ep *ep;
	struct cpl_act_establish *req = cplhdr(skb);
	unsigned short tcp_opt = ntohs(req->tcp_opt);
	unsigned int tid = GET_TID(req);
	unsigned int atid = TID_TID_G(ntohl(req->tos_atid));
	struct tid_info *t = dev->rdev.lldi.tids;
	int ret;

	ep = lookup_atid(t, atid);

	pr_debug("ep %p tid %u snd_isn %u rcv_isn %u\n", ep, tid,
		 be32_to_cpu(req->snd_isn), be32_to_cpu(req->rcv_isn));

	mutex_lock(&ep->com.mutex);
	dst_confirm(ep->dst);

	/* setup the hwtid for this connection */
	ep->hwtid = tid;
	cxgb4_insert_tid(t, ep, tid, ep->com.local_addr.ss_family);
	insert_ep_tid(ep);

	ep->snd_seq = be32_to_cpu(req->snd_isn);
	ep->rcv_seq = be32_to_cpu(req->rcv_isn);
	ep->snd_wscale = TCPOPT_SND_WSCALE_G(tcp_opt);

	set_emss(ep, tcp_opt);

	/* dealloc the atid */
	xa_erase_irq(&ep->com.dev->atids, atid);
	cxgb4_free_atid(t, atid);
	set_bit(ACT_ESTAB, &ep->com.history);

	/* start MPA negotiation */
	ret = send_flowc(ep);
	if (ret)
		goto err;
	if (ep->retry_with_mpa_v1)
		ret = send_mpa_req(ep, skb, 1);
	else
		ret = send_mpa_req(ep, skb, mpa_rev);
	if (ret)
		goto err;
	mutex_unlock(&ep->com.mutex);
	return 0;
err:
	mutex_unlock(&ep->com.mutex);
	connect_reply_upcall(ep, -ENOMEM);
	c4iw_ep_disconnect(ep, 0, GFP_KERNEL);
	return 0;
}
