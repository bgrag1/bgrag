static void comphy_gbe_phy_init(struct mvebu_a3700_comphy_lane *lane,
				bool is_1gbps)
{
	int addr, fix_idx;
	u16 val;

	fix_idx = 0;
	for (addr = 0; addr < 512; addr++) {
		/*
		 * All PHY register values are defined in full for 3.125Gbps
		 * SERDES speed. The values required for 1.25 Gbps are almost
		 * the same and only few registers should be "fixed" in
		 * comparison to 3.125 Gbps values. These register values are
		 * stored in "gbe_phy_init_fix" array.
		 */
		if (!is_1gbps &&
		    fix_idx < ARRAY_SIZE(gbe_phy_init_fix) &&
		    gbe_phy_init_fix[fix_idx].addr == addr) {
			/* Use new value */
			val = gbe_phy_init_fix[fix_idx].value;
			fix_idx++;
		} else {
			val = gbe_phy_init[addr];
		}

		comphy_lane_reg_set(lane, addr, val, 0xFFFF);
	}
}
