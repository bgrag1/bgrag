static void
aoecmd_cfg_pkts(ushort aoemajor, unsigned char aoeminor, struct sk_buff_head *queue)
{
	struct aoe_hdr *h;
	struct aoe_cfghdr *ch;
	struct sk_buff *skb;
	struct net_device *ifp;

	rcu_read_lock();
	for_each_netdev_rcu(&init_net, ifp) {
		dev_hold(ifp);
		if (!is_aoe_netif(ifp))
			goto cont;

		skb = new_skb(sizeof *h + sizeof *ch);
		if (skb == NULL) {
			printk(KERN_INFO "aoe: skb alloc failure\n");
			goto cont;
		}
		skb_put(skb, sizeof *h + sizeof *ch);
		skb->dev = ifp;
		__skb_queue_tail(queue, skb);
		h = (struct aoe_hdr *) skb_mac_header(skb);
		memset(h, 0, sizeof *h + sizeof *ch);

		memset(h->dst, 0xff, sizeof h->dst);
		memcpy(h->src, ifp->dev_addr, sizeof h->src);
		h->type = __constant_cpu_to_be16(ETH_P_AOE);
		h->verfl = AOE_HVER;
		h->major = cpu_to_be16(aoemajor);
		h->minor = aoeminor;
		h->cmd = AOECMD_CFG;

cont:
		dev_put(ifp);
	}
	rcu_read_unlock();
}
