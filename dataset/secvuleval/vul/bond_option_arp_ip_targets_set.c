static int bond_option_arp_ip_targets_set(struct bonding *bond,
					  const struct bond_opt_value *newval)
{
	int ret = -EPERM;
	__be32 target;

	if (newval->string) {
		if (!in4_pton(newval->string+1, -1, (u8 *)&target, -1, NULL)) {
			netdev_err(bond->dev, "invalid ARP target %pI4 specified\n",
				   &target);
			return ret;
		}
		if (newval->string[0] == '+')
			ret = bond_option_arp_ip_target_add(bond, target);
		else if (newval->string[0] == '-')
			ret = bond_option_arp_ip_target_rem(bond, target);
		else
			netdev_err(bond->dev, "no command found in arp_ip_targets file - use +<addr> or -<addr>\n");
	} else {
		target = newval->value;
		ret = bond_option_arp_ip_target_add(bond, target);
	}

	return ret;
}
