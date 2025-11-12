static int input_action_end_dx4(struct sk_buff *skb,
				struct seg6_local_lwt *slwt)
{
	if (!decap_and_validate(skb, IPPROTO_IPIP))
		goto drop;

	if (!pskb_may_pull(skb, sizeof(struct iphdr)))
		goto drop;

	skb->protocol = htons(ETH_P_IP);
	skb_set_transport_header(skb, sizeof(struct iphdr));
	nf_reset_ct(skb);

	if (static_branch_unlikely(&nf_hooks_lwtunnel_enabled))
		return NF_HOOK(NFPROTO_IPV4, NF_INET_PRE_ROUTING,
			       dev_net(skb->dev), NULL, skb, skb->dev,
			       NULL, input_action_end_dx4_finish);

	return input_action_end_dx4_finish(dev_net(skb->dev), NULL, skb);
drop:
	kfree_skb(skb);
	return -EINVAL;
}
