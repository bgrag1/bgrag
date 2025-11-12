static void arena_vm_close(struct vm_area_struct *vma)
{
	struct bpf_map *map = vma->vm_file->private_data;
	struct bpf_arena *arena = container_of(map, struct bpf_arena, map);
	struct vma_list *vml;

	guard(mutex)(&arena->lock);
	vml = vma->vm_private_data;
	list_del(&vml->head);
	vma->vm_private_data = NULL;
	kfree(vml);
}
