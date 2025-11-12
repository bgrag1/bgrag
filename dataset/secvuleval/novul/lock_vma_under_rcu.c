struct vm_area_struct *lock_vma_under_rcu(struct mm_struct *mm,
					  unsigned long address)
{
	MA_STATE(mas, &mm->mm_mt, address, address);
	struct vm_area_struct *vma;

	rcu_read_lock();
retry:
	vma = mas_walk(&mas);
	if (!vma)
		goto inval;

	/* Only anonymous and tcp vmas are supported for now */
	if (!vma_is_anonymous(vma) && !vma_is_tcp(vma))
		goto inval;

	if (!vma_start_read(vma))
		goto inval;

	/*
	 * find_mergeable_anon_vma uses adjacent vmas which are not locked.
	 * This check must happen after vma_start_read(); otherwise, a
	 * concurrent mremap() with MREMAP_DONTUNMAP could dissociate the VMA
	 * from its anon_vma.
	 */
	if (unlikely(!vma->anon_vma && !vma_is_tcp(vma)))
		goto inval_end_read;

	/*
	 * Due to the possibility of userfault handler dropping mmap_lock, avoid
	 * it for now and fall back to page fault handling under mmap_lock.
	 */
	if (userfaultfd_armed(vma))
		goto inval_end_read;

	/* Check since vm_start/vm_end might change before we lock the VMA */
	if (unlikely(address < vma->vm_start || address >= vma->vm_end))
		goto inval_end_read;

	/* Check if the VMA got isolated after we found it */
	if (vma->detached) {
		vma_end_read(vma);
		count_vm_vma_lock_event(VMA_LOCK_MISS);
		/* The area was replaced with another one */
		goto retry;
	}

	rcu_read_unlock();
	return vma;

inval_end_read:
	vma_end_read(vma);
inval:
	rcu_read_unlock();
	count_vm_vma_lock_event(VMA_LOCK_ABORT);
	return NULL;
}
