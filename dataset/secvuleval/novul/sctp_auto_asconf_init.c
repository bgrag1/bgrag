static void sctp_auto_asconf_init(struct sctp_sock *sp)
{
	struct net *net = sock_net(&sp->inet.sk);

	if (net->sctp.default_auto_asconf) {
		spin_lock_bh(&net->sctp.addr_wq_lock);
		list_add_tail(&sp->auto_asconf_list, &net->sctp.auto_asconf_splist);
		spin_unlock_bh(&net->sctp.addr_wq_lock);
		sp->do_auto_asconf = 1;
	}
}