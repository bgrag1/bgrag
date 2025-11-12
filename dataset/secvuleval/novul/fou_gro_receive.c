static struct sk_buff *fou_gro_receive(struct sock *sk,
				       struct list_head *head,
				       struct sk_buff *skb)
{
	const struct net_offload __rcu **offloads;
	struct fou *fou = fou_from_sock(sk);
	const struct net_offload *ops;
	struct sk_buff *pp = NULL;
	u8 proto;

	if (!fou)
		goto out;

	proto = fou->protocol;

	/* We can clear the encap_mark for FOU as we are essentially doing
	 * one of two possible things.  We are either adding an L4 tunnel
	 * header to the outer L3 tunnel header, or we are simply
	 * treating the GRE tunnel header as though it is a UDP protocol
	 * specific header such as VXLAN or GENEVE.
	 */
	NAPI_GRO_CB(skb)->encap_mark = 0;

	/* Flag this frame as already having an outer encap header */
	NAPI_GRO_CB(skb)->is_fou = 1;

	offloads = NAPI_GRO_CB(skb)->is_ipv6 ? inet6_offloads : inet_offloads;
	ops = rcu_dereference(offloads[proto]);
	if (!ops || !ops->callbacks.gro_receive)
		goto out;

	pp = call_gro_receive(ops->callbacks.gro_receive, head, skb);

out:
	return pp;
}
