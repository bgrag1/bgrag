static inline struct fou *fou_from_sock(struct sock *sk)
{
	return sk->sk_user_data;
}
