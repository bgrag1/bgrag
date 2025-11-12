int mas_empty_area_rev(struct ma_state *mas, unsigned long min,
		unsigned long max, unsigned long size)
{
	struct maple_enode *last = mas->node;

	if (min > max)
		return -EINVAL;

	if (size == 0 || max - min < size - 1)
		return -EINVAL;

	if (mas_is_start(mas))
		mas_start(mas);
	else if ((mas->offset < 2) && (!mas_rewind_node(mas)))
		return -EBUSY;

	if (unlikely(mas_is_none(mas) || mas_is_ptr(mas)))
		return mas_sparse_area(mas, min, max, size, false);
	else if (mas->offset >= 2)
		mas->offset -= 2;
	else
		mas->offset = mas_data_end(mas);


	/* The start of the window can only be within these values. */
	mas->index = min;
	mas->last = max;

	while (!mas_rev_awalk(mas, size, &min, &max)) {
		if (last == mas->node) {
			if (!mas_rewind_node(mas))
				return -EBUSY;
		} else {
			last = mas->node;
		}
	}

	if (mas_is_err(mas))
		return xa_err(mas->node);

	if (unlikely(mas->offset == MAPLE_NODE_SLOTS))
		return -EBUSY;

	/* Trim the upper limit to the max. */
	if (max < mas->last)
		mas->last = max;

	mas->index = mas->last - size + 1;
	mas->end = mas_data_end(mas);
	return 0;
}
