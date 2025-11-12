int of_irq_parse_one(struct device_node *device, int index, struct of_phandle_args *out_irq)
{
	struct device_node *p;
	const __be32 *addr;
	u32 intsize;
	int i, res;

	pr_debug("of_irq_parse_one: dev=%pOF, index=%d\n", device, index);

	/* OldWorld mac stuff is "special", handle out of line */
	if (of_irq_workarounds & OF_IMAP_OLDWORLD_MAC)
		return of_irq_parse_oldworld(device, index, out_irq);

	/* Get the reg property (if any) */
	addr = of_get_property(device, "reg", NULL);

	/* Try the new-style interrupts-extended first */
	res = of_parse_phandle_with_args(device, "interrupts-extended",
					"#interrupt-cells", index, out_irq);
	if (!res)
		return of_irq_parse_raw(addr, out_irq);

	/* Look for the interrupt parent. */
	p = of_irq_find_parent(device);
	if (p == NULL)
		return -EINVAL;

	/* Get size of interrupt specifier */
	if (of_property_read_u32(p, "#interrupt-cells", &intsize)) {
		res = -EINVAL;
		goto out;
	}

	pr_debug(" parent=%pOF, intsize=%d\n", p, intsize);

	/* Copy intspec into irq structure */
	out_irq->np = p;
	out_irq->args_count = intsize;
	for (i = 0; i < intsize; i++) {
		res = of_property_read_u32_index(device, "interrupts",
						 (index * intsize) + i,
						 out_irq->args + i);
		if (res)
			goto out;
	}

	pr_debug(" intspec=%d\n", *out_irq->args);


	/* Check if there are any interrupt-map translations to process */
	res = of_irq_parse_raw(addr, out_irq);
 out:
	of_node_put(p);
	return res;
}
