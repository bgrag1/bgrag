vm_fault_t netfs_page_mkwrite(struct vm_fault *vmf, struct netfs_group *netfs_group)
{
	struct netfs_group *group;
	struct folio *folio = page_folio(vmf->page);
	struct file *file = vmf->vma->vm_file;
	struct address_space *mapping = file->f_mapping;
	struct inode *inode = file_inode(file);
	struct netfs_inode *ictx = netfs_inode(inode);
	vm_fault_t ret = VM_FAULT_RETRY;
	int err;

	_enter("%lx", folio->index);

	sb_start_pagefault(inode->i_sb);

	if (folio_lock_killable(folio) < 0)
		goto out;
	if (folio->mapping != mapping) {
		folio_unlock(folio);
		ret = VM_FAULT_NOPAGE;
		goto out;
	}

	if (folio_wait_writeback_killable(folio)) {
		ret = VM_FAULT_LOCKED;
		goto out;
	}

	/* Can we see a streaming write here? */
	if (WARN_ON(!folio_test_uptodate(folio))) {
		ret = VM_FAULT_SIGBUS | VM_FAULT_LOCKED;
		goto out;
	}

	group = netfs_folio_group(folio);
	if (group != netfs_group && group != NETFS_FOLIO_COPY_TO_CACHE) {
		folio_unlock(folio);
		err = filemap_fdatawait_range(mapping,
					      folio_pos(folio),
					      folio_pos(folio) + folio_size(folio));
		switch (err) {
		case 0:
			ret = VM_FAULT_RETRY;
			goto out;
		case -ENOMEM:
			ret = VM_FAULT_OOM;
			goto out;
		default:
			ret = VM_FAULT_SIGBUS;
			goto out;
		}
	}

	if (folio_test_dirty(folio))
		trace_netfs_folio(folio, netfs_folio_trace_mkwrite_plus);
	else
		trace_netfs_folio(folio, netfs_folio_trace_mkwrite);
	netfs_set_group(folio, netfs_group);
	file_update_time(file);
	if (ictx->ops->post_modify)
		ictx->ops->post_modify(inode);
	ret = VM_FAULT_LOCKED;
out:
	sb_end_pagefault(inode->i_sb);
	return ret;
}
