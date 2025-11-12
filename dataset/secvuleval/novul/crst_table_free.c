void crst_table_free(struct mm_struct *mm, unsigned long *table)
{
	if (!table)
		return;
	pagetable_free(virt_to_ptdesc(table));
}
