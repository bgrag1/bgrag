void vmci_resource_remove(struct vmci_resource *resource)
{
	struct vmci_handle handle = resource->handle;
	unsigned int idx = vmci_resource_hash(handle);
	struct vmci_resource *r;

	/* Remove resource from hash table. */
	spin_lock(&vmci_resource_table.lock);

	hlist_for_each_entry(r, &vmci_resource_table.entries[idx], node) {
		if (vmci_handle_is_equal(r->handle, resource->handle) &&
		    resource->type == r->type) {
			hlist_del_init_rcu(&r->node);
			break;
		}
	}

	spin_unlock(&vmci_resource_table.lock);
	synchronize_rcu();

	vmci_resource_put(resource);
	wait_for_completion(&resource->done);
}
