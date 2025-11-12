int
efc_nport_vport_del(struct efc *efc, struct efc_domain *domain,
		    u64 wwpn, uint64_t wwnn)
{
	struct efc_nport *nport;
	struct efc_vport *vport;
	struct efc_vport *next;
	unsigned long flags = 0;

	spin_lock_irqsave(&efc->vport_lock, flags);
	/* walk the efc_vport_list and remove from there */
	list_for_each_entry_safe(vport, next, &efc->vport_list, list_entry) {
		if (vport->wwpn == wwpn && vport->wwnn == wwnn) {
			list_del(&vport->list_entry);
			kfree(vport);
			break;
		}
	}
	spin_unlock_irqrestore(&efc->vport_lock, flags);

	if (!domain) {
		/* No domain means no nport to look for */
		return 0;
	}

	spin_lock_irqsave(&efc->lock, flags);
	list_for_each_entry(nport, &domain->nport_list, list_entry) {
		if (nport->wwpn == wwpn && nport->wwnn == wwnn) {
			/* Shutdown this NPORT */
			efc_sm_post_event(&nport->sm, EFC_EVT_SHUTDOWN, NULL);
			kref_put(&nport->ref, nport->release);
			break;
		}
	}

	spin_unlock_irqrestore(&efc->lock, flags);
	return 0;
}
