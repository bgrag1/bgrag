static void bond_ipsec_del_sa_all(struct bonding *bond)
{
	struct net_device *bond_dev = bond->dev;
	struct bond_ipsec *ipsec;
	struct slave *slave;

	rcu_read_lock();
	slave = rcu_dereference(bond->curr_active_slave);
	if (!slave) {
		rcu_read_unlock();
		return;
	}

	spin_lock_bh(&bond->ipsec_lock);
	list_for_each_entry(ipsec, &bond->ipsec_list, list) {
		if (!ipsec->xs->xso.real_dev)
			continue;

		if (!slave->dev->xfrmdev_ops ||
		    !slave->dev->xfrmdev_ops->xdo_dev_state_delete ||
		    netif_is_bond_master(slave->dev)) {
			slave_warn(bond_dev, slave->dev,
				   "%s: no slave xdo_dev_state_delete\n",
				   __func__);
		} else {
			slave->dev->xfrmdev_ops->xdo_dev_state_delete(ipsec->xs);
		}
	}
	spin_unlock_bh(&bond->ipsec_lock);
	rcu_read_unlock();
}
