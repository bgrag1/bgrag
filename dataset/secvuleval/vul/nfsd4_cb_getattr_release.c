static void
nfsd4_cb_getattr_release(struct nfsd4_callback *cb)
{
	struct nfs4_cb_fattr *ncf =
			container_of(cb, struct nfs4_cb_fattr, ncf_getattr);
	struct nfs4_delegation *dp =
			container_of(ncf, struct nfs4_delegation, dl_cb_fattr);

	nfs4_put_stid(&dp->dl_stid);
	clear_bit(CB_GETATTR_BUSY, &ncf->ncf_cb_flags);
	wake_up_bit(&ncf->ncf_cb_flags, CB_GETATTR_BUSY);
}
