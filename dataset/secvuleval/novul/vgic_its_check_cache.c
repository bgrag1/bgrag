static struct vgic_irq *vgic_its_check_cache(struct kvm *kvm, phys_addr_t db,
					     u32 devid, u32 eventid)
{
	struct vgic_dist *dist = &kvm->arch.vgic;
	struct vgic_irq *irq;
	unsigned long flags;

	raw_spin_lock_irqsave(&dist->lpi_list_lock, flags);

	irq = __vgic_its_check_cache(dist, db, devid, eventid);
	if (irq)
		vgic_get_irq_kref(irq);

	raw_spin_unlock_irqrestore(&dist->lpi_list_lock, flags);

	return irq;
}
