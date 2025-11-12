static void base_crst_free(unsigned long *table)
{
	if (!table)
		return;
	pagetable_free(virt_to_ptdesc(table));
}
