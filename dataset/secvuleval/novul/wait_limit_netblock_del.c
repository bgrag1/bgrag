static void wait_limit_netblock_del(rbnode_type* n, void* ATTR_UNUSED(arg))
{
	free(n);
}