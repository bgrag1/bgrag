static struct sock *gtp_encap_enable_socket(int fd, int type,
					    struct gtp_dev *gtp)
{
	struct udp_tunnel_sock_cfg tuncfg = {NULL};
	struct socket *sock;
	struct sock *sk;
	int err;

	pr_debug("enable gtp on %d, %d\n", fd, type);

	sock = sockfd_lookup(fd, &err);
	if (!sock) {
		pr_debug("gtp socket fd=%d not found\n", fd);
		return ERR_PTR(err);
	}

	sk = sock->sk;
	if (sk->sk_protocol != IPPROTO_UDP ||
	    sk->sk_type != SOCK_DGRAM ||
	    (sk->sk_family != AF_INET && sk->sk_family != AF_INET6)) {
		pr_debug("socket fd=%d not UDP\n", fd);
		sk = ERR_PTR(-EINVAL);
		goto out_sock;
	}

	if (sk->sk_family == AF_INET6 &&
	    !sk->sk_ipv6only) {
		sk = ERR_PTR(-EADDRNOTAVAIL);
		goto out_sock;
	}

	lock_sock(sk);
	if (sk->sk_user_data) {
		sk = ERR_PTR(-EBUSY);
		goto out_rel_sock;
	}

	sock_hold(sk);

	tuncfg.sk_user_data = gtp;
	tuncfg.encap_type = type;
	tuncfg.encap_rcv = gtp_encap_recv;
	tuncfg.encap_destroy = gtp_encap_destroy;

	setup_udp_tunnel_sock(sock_net(sock->sk), sock, &tuncfg);

out_rel_sock:
	release_sock(sock->sk);
out_sock:
	sockfd_put(sock);
	return sk;
}
