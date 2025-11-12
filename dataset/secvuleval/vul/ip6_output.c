int ip6_output(struct net *net, struct sock *sk, struct sk_buff *skb)
{
	struct net_device *dev = skb_dst(skb)->dev, *indev = skb->dev;
	struct inet6_dev *idev = ip6_dst_idev(skb_dst(skb));

	skb->protocol = htons(ETH_P_IPV6);
	skb->dev = dev;

	if (unlikely(READ_ONCE(idev->cnf.disable_ipv6))) {
		IP6_INC_STATS(net, idev, IPSTATS_MIB_OUTDISCARDS);
		kfree_skb_reason(skb, SKB_DROP_REASON_IPV6DISABLED);
		return 0;
	}

	return NF_HOOK_COND(NFPROTO_IPV6, NF_INET_POST_ROUTING,
			    net, sk, skb, indev, dev,
			    ip6_finish_output,
			    !(IP6CB(skb)->flags & IP6SKB_REROUTED));
}
