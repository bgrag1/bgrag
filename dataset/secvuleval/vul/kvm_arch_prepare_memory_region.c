int kvm_arch_prepare_memory_region(struct kvm *kvm,
				   const struct kvm_memory_slot *old,
				   struct kvm_memory_slot *new,
				   enum kvm_mr_change change)
{
	gpa_t size;

	/* When we are protected, we should not change the memory slots */
	if (kvm_s390_pv_get_handle(kvm))
		return -EINVAL;

	if (change != KVM_MR_DELETE && change != KVM_MR_FLAGS_ONLY) {
		/*
		 * A few sanity checks. We can have memory slots which have to be
		 * located/ended at a segment boundary (1MB). The memory in userland is
		 * ok to be fragmented into various different vmas. It is okay to mmap()
		 * and munmap() stuff in this slot after doing this call at any time
		 */

		if (new->userspace_addr & 0xffffful)
			return -EINVAL;

		size = new->npages * PAGE_SIZE;
		if (size & 0xffffful)
			return -EINVAL;

		if ((new->base_gfn * PAGE_SIZE) + size > kvm->arch.mem_limit)
			return -EINVAL;
	}

	if (!kvm->arch.migration_mode)
		return 0;

	/*
	 * Turn off migration mode when:
	 * - userspace creates a new memslot with dirty logging off,
	 * - userspace modifies an existing memslot (MOVE or FLAGS_ONLY) and
	 *   dirty logging is turned off.
	 * Migration mode expects dirty page logging being enabled to store
	 * its dirty bitmap.
	 */
	if (change != KVM_MR_DELETE &&
	    !(new->flags & KVM_MEM_LOG_DIRTY_PAGES))
		WARN(kvm_s390_vm_stop_migration(kvm),
		     "Failed to stop migration mode");

	return 0;
}
