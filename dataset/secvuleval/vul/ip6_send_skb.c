int ip6_send_skb(struct sk_buff *skb)
{
	struct net *net = sock_net(skb->sk);
	struct rt6_info *rt = dst_rt6_info(skb_dst(skb));
	int err;

	err = ip6_local_out(net, skb->sk, skb);
	if (err) {
		if (err > 0)
			err = net_xmit_errno(err);
		if (err)
			IP6_INC_STATS(net, rt->rt6i_idev,
				      IPSTATS_MIB_OUTDISCARDS);
	}

	return err;
}
