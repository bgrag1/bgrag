__be32 nf_tproxy_laddr4(struct sk_buff *skb, __be32 user_laddr, __be32 daddr)
{
	const struct in_ifaddr *ifa;
	struct in_device *indev;
	__be32 laddr;

	if (user_laddr)
		return user_laddr;

	laddr = 0;
	indev = __in_dev_get_rcu(skb->dev);
	if (!indev)
		return daddr;

	in_dev_for_each_ifa_rcu(ifa, indev) {
		if (ifa->ifa_flags & IFA_F_SECONDARY)
			continue;

		laddr = ifa->ifa_local;
		break;
	}

	return laddr ? laddr : daddr;
}
