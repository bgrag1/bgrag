int ip_route_use_hint(struct sk_buff *skb, __be32 daddr, __be32 saddr,
		      u8 tos, struct net_device *dev,
		      const struct sk_buff *hint)
{
	struct in_device *in_dev = __in_dev_get_rcu(dev);
	struct rtable *rt = skb_rtable(hint);
	struct net *net = dev_net(dev);
	int err = -EINVAL;
	u32 tag = 0;

	if (!in_dev)
		return -EINVAL;

	if (ipv4_is_multicast(saddr) || ipv4_is_lbcast(saddr))
		goto martian_source;

	if (ipv4_is_zeronet(saddr))
		goto martian_source;

	if (ipv4_is_loopback(saddr) && !IN_DEV_NET_ROUTE_LOCALNET(in_dev, net))
		goto martian_source;

	if (rt->rt_type != RTN_LOCAL)
		goto skip_validate_source;

	tos &= IPTOS_RT_MASK;
	err = fib_validate_source(skb, saddr, daddr, tos, 0, dev, in_dev, &tag);
	if (err < 0)
		goto martian_source;

skip_validate_source:
	skb_dst_copy(skb, hint);
	return 0;

martian_source:
	ip_handle_martian_source(dev, in_dev, skb, daddr, saddr);
	return err;
}
