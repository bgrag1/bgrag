static inline struct fou *fou_from_sock(struct sock *sk)
{
	return rcu_dereference_sk_user_data(sk);
}
