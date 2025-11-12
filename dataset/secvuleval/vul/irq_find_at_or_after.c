static unsigned int irq_find_at_or_after(unsigned int offset)
{
	unsigned long index = offset;
	struct irq_desc *desc = mt_find(&sparse_irqs, &index, nr_irqs);

	return desc ? irq_desc_get_irq(desc) : nr_irqs;
}
