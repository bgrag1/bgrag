static void arena_vm_close(struct vm_area_struct *vma)
{
	struct bpf_map *map = vma->vm_file->private_data;
	struct bpf_arena *arena = container_of(map, struct bpf_arena, map);
	struct vma_list *vml = vma->vm_private_data;

	if (!atomic_dec_and_test(&vml->mmap_count))
		return;
	guard(mutex)(&arena->lock);
	/* update link list under lock */
	list_del(&vml->head);
	vma->vm_private_data = NULL;
	kfree(vml);
}
