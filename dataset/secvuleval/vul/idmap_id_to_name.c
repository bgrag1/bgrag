static __be32 idmap_id_to_name(struct xdr_stream *xdr,
			       struct svc_rqst *rqstp, int type, u32 id)
{
	struct ent *item, key = {
		.id = id,
		.type = type,
	};
	__be32 *p;
	int ret;
	struct nfsd_net *nn = net_generic(SVC_NET(rqstp), nfsd_net_id);

	strscpy(key.authname, rqst_authname(rqstp), sizeof(key.authname));
	ret = idmap_lookup(rqstp, idtoname_lookup, &key, nn->idtoname_cache, &item);
	if (ret == -ENOENT)
		return encode_ascii_id(xdr, id);
	if (ret)
		return nfserrno(ret);
	ret = strlen(item->name);
	WARN_ON_ONCE(ret > IDMAP_NAMESZ);
	p = xdr_reserve_space(xdr, ret + 4);
	if (!p)
		return nfserr_resource;
	p = xdr_encode_opaque(p, item->name, ret);
	cache_put(&item->h, nn->idtoname_cache);
	return 0;
}
