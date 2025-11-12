static int xfrm6_get_saddr(struct net *net, int oif,
			   xfrm_address_t *saddr, xfrm_address_t *daddr,
			   u32 mark)
{
	struct dst_entry *dst;
	struct net_device *dev;
	struct inet6_dev *idev;

	dst = xfrm6_dst_lookup(net, 0, oif, NULL, daddr, mark);
	if (IS_ERR(dst))
		return -EHOSTUNREACH;

	idev = ip6_dst_idev(dst);
	if (!idev) {
		dst_release(dst);
		return -EHOSTUNREACH;
	}
	dev = idev->dev;
	ipv6_dev_get_saddr(dev_net(dev), dev, &daddr->in6, 0, &saddr->in6);
	dst_release(dst);
	return 0;
}
