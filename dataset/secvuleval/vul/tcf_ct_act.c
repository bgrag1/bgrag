TC_INDIRECT_SCOPE int tcf_ct_act(struct sk_buff *skb, const struct tc_action *a,
				 struct tcf_result *res)
{
	struct net *net = dev_net(skb->dev);
	enum ip_conntrack_info ctinfo;
	struct tcf_ct *c = to_ct(a);
	struct nf_conn *tmpl = NULL;
	struct nf_hook_state state;
	bool cached, commit, clear;
	int nh_ofs, err, retval;
	struct tcf_ct_params *p;
	bool add_helper = false;
	bool skip_add = false;
	bool defrag = false;
	struct nf_conn *ct;
	u8 family;

	p = rcu_dereference_bh(c->params);

	retval = READ_ONCE(c->tcf_action);
	commit = p->ct_action & TCA_CT_ACT_COMMIT;
	clear = p->ct_action & TCA_CT_ACT_CLEAR;
	tmpl = p->tmpl;

	tcf_lastuse_update(&c->tcf_tm);
	tcf_action_update_bstats(&c->common, skb);

	if (clear) {
		tc_skb_cb(skb)->post_ct = false;
		ct = nf_ct_get(skb, &ctinfo);
		if (ct) {
			nf_ct_put(ct);
			nf_ct_set(skb, NULL, IP_CT_UNTRACKED);
		}

		goto out_clear;
	}

	family = tcf_ct_skb_nf_family(skb);
	if (family == NFPROTO_UNSPEC)
		goto drop;

	/* The conntrack module expects to be working at L3.
	 * We also try to pull the IPv4/6 header to linear area
	 */
	nh_ofs = skb_network_offset(skb);
	skb_pull_rcsum(skb, nh_ofs);
	err = tcf_ct_handle_fragments(net, skb, family, p->zone, &defrag);
	if (err)
		goto out_frag;

	err = nf_ct_skb_network_trim(skb, family);
	if (err)
		goto drop;

	/* If we are recirculating packets to match on ct fields and
	 * committing with a separate ct action, then we don't need to
	 * actually run the packet through conntrack twice unless it's for a
	 * different zone.
	 */
	cached = tcf_ct_skb_nfct_cached(net, skb, p);
	if (!cached) {
		if (tcf_ct_flow_table_lookup(p, skb, family)) {
			skip_add = true;
			goto do_nat;
		}

		/* Associate skb with specified zone. */
		if (tmpl) {
			nf_conntrack_put(skb_nfct(skb));
			nf_conntrack_get(&tmpl->ct_general);
			nf_ct_set(skb, tmpl, IP_CT_NEW);
		}

		state.hook = NF_INET_PRE_ROUTING;
		state.net = net;
		state.pf = family;
		err = nf_conntrack_in(skb, &state);
		if (err != NF_ACCEPT)
			goto out_push;
	}

do_nat:
	ct = nf_ct_get(skb, &ctinfo);
	if (!ct)
		goto out_push;
	nf_ct_deliver_cached_events(ct);
	nf_conn_act_ct_ext_fill(skb, ct, ctinfo);

	err = tcf_ct_act_nat(skb, ct, ctinfo, p->ct_action, &p->range, commit);
	if (err != NF_ACCEPT)
		goto drop;

	if (!nf_ct_is_confirmed(ct) && commit && p->helper && !nfct_help(ct)) {
		err = __nf_ct_try_assign_helper(ct, p->tmpl, GFP_ATOMIC);
		if (err)
			goto drop;
		add_helper = true;
		if (p->ct_action & TCA_CT_ACT_NAT && !nfct_seqadj(ct)) {
			if (!nfct_seqadj_ext_add(ct))
				goto drop;
		}
	}

	if (nf_ct_is_confirmed(ct) ? ((!cached && !skip_add) || add_helper) : commit) {
		if (nf_ct_helper(skb, ct, ctinfo, family) != NF_ACCEPT)
			goto drop;
	}

	if (commit) {
		tcf_ct_act_set_mark(ct, p->mark, p->mark_mask);
		tcf_ct_act_set_labels(ct, p->labels, p->labels_mask);

		if (!nf_ct_is_confirmed(ct))
			nf_conn_act_ct_ext_add(skb, ct, ctinfo);

		/* This will take care of sending queued events
		 * even if the connection is already confirmed.
		 */
		if (nf_conntrack_confirm(skb) != NF_ACCEPT)
			goto drop;
	}

	if (!skip_add)
		tcf_ct_flow_table_process_conn(p->ct_ft, ct, ctinfo);

out_push:
	skb_push_rcsum(skb, nh_ofs);

	tc_skb_cb(skb)->post_ct = true;
	tc_skb_cb(skb)->zone = p->zone;
out_clear:
	if (defrag)
		qdisc_skb_cb(skb)->pkt_len = skb->len;
	return retval;

out_frag:
	if (err != -EINPROGRESS)
		tcf_action_inc_drop_qstats(&c->common);
	return TC_ACT_CONSUMED;

drop:
	tcf_action_inc_drop_qstats(&c->common);
	return TC_ACT_SHOT;
}
