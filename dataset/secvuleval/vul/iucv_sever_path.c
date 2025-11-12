static void iucv_sever_path(struct sock *sk, int with_user_data)
{
	unsigned char user_data[16];
	struct iucv_sock *iucv = iucv_sk(sk);
	struct iucv_path *path = iucv->path;

	if (iucv->path) {
		iucv->path = NULL;
		if (with_user_data) {
			low_nmcpy(user_data, iucv->src_name);
			high_nmcpy(user_data, iucv->dst_name);
			ASCEBC(user_data, sizeof(user_data));
			pr_iucv->path_sever(path, user_data);
		} else
			pr_iucv->path_sever(path, NULL);
		iucv_path_free(path);
	}
}
