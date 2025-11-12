void *__genradix_ptr_alloc(struct __genradix *radix, size_t offset,
			   gfp_t gfp_mask)
{
	struct genradix_root *v = READ_ONCE(radix->root);
	struct genradix_node *n, *new_node = NULL;
	unsigned level;

	/* Increase tree depth if necessary: */
	while (1) {
		struct genradix_root *r = v, *new_root;

		n	= genradix_root_to_node(r);
		level	= genradix_root_to_depth(r);

		if (n && ilog2(offset) < genradix_depth_shift(level))
			break;

		if (!new_node) {
			new_node = genradix_alloc_node(gfp_mask);
			if (!new_node)
				return NULL;
		}

		new_node->children[0] = n;
		new_root = ((struct genradix_root *)
			    ((unsigned long) new_node | (n ? level + 1 : 0)));

		if ((v = cmpxchg_release(&radix->root, r, new_root)) == r) {
			v = new_root;
			new_node = NULL;
		} else {
			new_node->children[0] = NULL;
		}
	}

	while (level--) {
		struct genradix_node **p =
			&n->children[offset >> genradix_depth_shift(level)];
		offset &= genradix_depth_size(level) - 1;

		n = READ_ONCE(*p);
		if (!n) {
			if (!new_node) {
				new_node = genradix_alloc_node(gfp_mask);
				if (!new_node)
					return NULL;
			}

			if (!(n = cmpxchg_release(p, NULL, new_node)))
				swap(n, new_node);
		}
	}

	if (new_node)
		genradix_free_node(new_node);

	return &n->data[offset];
}
