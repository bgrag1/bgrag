static int __fib6_rule_action(struct fib_rule *rule, struct flowi *flp,
			      int flags, struct fib_lookup_arg *arg)
{
	struct fib6_result *res = arg->result;
	struct flowi6 *flp6 = &flp->u.ip6;
	struct rt6_info *rt = NULL;
	struct fib6_table *table;
	struct net *net = rule->fr_net;
	pol_lookup_t lookup = arg->lookup_ptr;
	int err = 0;
	u32 tb_id;

	switch (rule->action) {
	case FR_ACT_TO_TBL:
		break;
	case FR_ACT_UNREACHABLE:
		err = -ENETUNREACH;
		rt = net->ipv6.ip6_null_entry;
		goto discard_pkt;
	default:
	case FR_ACT_BLACKHOLE:
		err = -EINVAL;
		rt = net->ipv6.ip6_blk_hole_entry;
		goto discard_pkt;
	case FR_ACT_PROHIBIT:
		err = -EACCES;
		rt = net->ipv6.ip6_prohibit_entry;
		goto discard_pkt;
	}

	tb_id = fib_rule_get_table(rule, arg);
	table = fib6_get_table(net, tb_id);
	if (!table) {
		err = -EAGAIN;
		goto out;
	}

	rt = pol_lookup_func(lookup,
			     net, table, flp6, arg->lookup_data, flags);
	if (rt != net->ipv6.ip6_null_entry) {
		err = fib6_rule_saddr(net, rule, flags, flp6,
				      ip6_dst_idev(&rt->dst)->dev);

		if (err == -EAGAIN)
			goto again;

		err = rt->dst.error;
		if (err != -EAGAIN)
			goto out;
	}
again:
	ip6_rt_put_flags(rt, flags);
	err = -EAGAIN;
	rt = NULL;
	goto out;

discard_pkt:
	if (!(flags & RT6_LOOKUP_F_DST_NOREF))
		dst_hold(&rt->dst);
out:
	res->rt6 = rt;
	return err;
}
