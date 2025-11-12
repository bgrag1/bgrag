static int input_action_end_dx6(struct sk_buff *skb,
				struct seg6_local_lwt *slwt)
{
	/* this function accepts IPv6 encapsulated packets, with either
	 * an SRH with SL=0, or no SRH.
	 */

	if (!decap_and_validate(skb, IPPROTO_IPV6))
		goto drop;

	if (!pskb_may_pull(skb, sizeof(struct ipv6hdr)))
		goto drop;

	skb_set_transport_header(skb, sizeof(struct ipv6hdr));
	nf_reset_ct(skb);

	if (static_branch_unlikely(&nf_hooks_lwtunnel_enabled))
		return NF_HOOK(NFPROTO_IPV6, NF_INET_PRE_ROUTING,
			       dev_net(skb->dev), NULL, skb, skb->dev,
			       NULL, input_action_end_dx6_finish);

	return input_action_end_dx6_finish(dev_net(skb->dev), NULL, skb);
drop:
	kfree_skb(skb);
	return -EINVAL;
}
