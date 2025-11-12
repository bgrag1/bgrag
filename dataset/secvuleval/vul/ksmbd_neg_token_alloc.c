static int ksmbd_neg_token_alloc(void *context, size_t hdrlen,
				 unsigned char tag, const void *value,
				 size_t vlen)
{
	struct ksmbd_conn *conn = context;

	conn->mechToken = kmemdup_nul(value, vlen, GFP_KERNEL);
	if (!conn->mechToken)
		return -ENOMEM;

	return 0;
}
