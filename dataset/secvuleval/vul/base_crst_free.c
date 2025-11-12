static void base_crst_free(unsigned long *table)
{
	pagetable_free(virt_to_ptdesc(table));
}
