int amdgpu_ras_interrupt_dispatch(struct amdgpu_device *adev,
		struct ras_dispatch_if *info)
{
	struct ras_manager *obj;
	struct ras_ih_data *data;

	obj = amdgpu_ras_find_obj(adev, &info->head);
	if (!obj)
		return -EINVAL;

	data = &obj->ih_data;

	if (data->inuse == 0)
		return 0;

	/* Might be overflow... */
	memcpy(&data->ring[data->wptr], info->entry,
			data->element_size);

	wmb();
	data->wptr = (data->aligned_element_size +
			data->wptr) % data->ring_size;

	schedule_work(&data->ih_work);

	return 0;
}
