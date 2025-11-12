int
smb2_set_path_size(const unsigned int xid, struct cifs_tcon *tcon,
		   const char *full_path, __u64 size,
		   struct cifs_sb_info *cifs_sb, bool set_alloc,
		   struct dentry *dentry)
{
	struct cifs_open_parms oparms;
	struct cifsFileInfo *cfile;
	struct kvec in_iov;
	__le64 eof = cpu_to_le64(size);
	int rc;

	in_iov.iov_base = &eof;
	in_iov.iov_len = sizeof(eof);
	cifs_get_writable_path(tcon, full_path, FIND_WR_ANY, &cfile);

	oparms = CIFS_OPARMS(cifs_sb, tcon, full_path, FILE_WRITE_DATA,
			     FILE_OPEN, 0, ACL_NO_MODE);
	rc = smb2_compound_op(xid, tcon, cifs_sb,
			      full_path, &oparms, &in_iov,
			      &(int){SMB2_OP_SET_EOF}, 1,
			      cfile, NULL, NULL, dentry);
	if (rc == -EINVAL) {
		cifs_dbg(FYI, "invalid lease key, resending request without lease");
		rc = smb2_compound_op(xid, tcon, cifs_sb,
				      full_path, &oparms, &in_iov,
				      &(int){SMB2_OP_SET_EOF}, 1,
				      cfile, NULL, NULL, NULL);
	}
	return rc;
}
