eth_addr_resolve(hashether_t *tp) {
    ether_t      *eth;
    hashmanuf_t *manuf_value;
    const guint8 *addr = tp->addr;
    size_t addr_size = sizeof(tp->addr);

    if ( (eth = get_ethbyaddr(addr)) != NULL) {
        (void) g_strlcpy(tp->resolved_name, eth->name, MAXNAMELEN);
        tp->status = HASHETHER_STATUS_RESOLVED_NAME;
        return tp;
    } else {
        guint         mask;
        gchar        *name;
        address       ether_addr;

        /* Unknown name.  Try looking for it in the well-known-address
           tables for well-known address ranges smaller than 2^24. */
        mask = 7;
        do {
            /* Only the topmost 5 bytes participate fully */
            if ((name = wka_name_lookup(addr, mask+40)) != NULL) {
                snprintf(tp->resolved_name, MAXNAMELEN, "%s_%02x",
                        name, addr[5] & (0xFF >> mask));
                tp->status = HASHETHER_STATUS_RESOLVED_DUMMY;
                return tp;
            }
        } while (mask--);

        mask = 7;
        do {
            /* Only the topmost 4 bytes participate fully */
            if ((name = wka_name_lookup(addr, mask+32)) != NULL) {
                snprintf(tp->resolved_name, MAXNAMELEN, "%s_%02x:%02x",
                        name, addr[4] & (0xFF >> mask), addr[5]);
                tp->status = HASHETHER_STATUS_RESOLVED_DUMMY;
                return tp;
            }
        } while (mask--);

        mask = 7;
        do {
            /* Only the topmost 3 bytes participate fully */
            if ((name = wka_name_lookup(addr, mask+24)) != NULL) {
                snprintf(tp->resolved_name, MAXNAMELEN, "%s_%02x:%02x:%02x",
                        name, addr[3] & (0xFF >> mask), addr[4], addr[5]);
                tp->status = HASHETHER_STATUS_RESOLVED_DUMMY;
                return tp;
            }
        } while (mask--);

        /* Now try looking in the manufacturer table. */
        manuf_value = manuf_name_lookup(addr, addr_size);
        if ((manuf_value != NULL) && (manuf_value->status != HASHETHER_STATUS_UNRESOLVED)) {
            snprintf(tp->resolved_name, MAXNAMELEN, "%s_%02x:%02x:%02x",
                    manuf_value->resolved_name, addr[3], addr[4], addr[5]);
            tp->status = HASHETHER_STATUS_RESOLVED_DUMMY;
            return tp;
        }

        /* Now try looking for it in the well-known-address
           tables for well-known address ranges larger than 2^24. */
        mask = 7;
        do {
            /* Only the topmost 2 bytes participate fully */
            if ((name = wka_name_lookup(addr, mask+16)) != NULL) {
                snprintf(tp->resolved_name, MAXNAMELEN, "%s_%02x:%02x:%02x:%02x",
                        name, addr[2] & (0xFF >> mask), addr[3], addr[4],
                        addr[5]);
                tp->status = HASHETHER_STATUS_RESOLVED_DUMMY;
                return tp;
            }
        } while (mask--);

        mask = 7;
        do {
            /* Only the topmost byte participates fully */
            if ((name = wka_name_lookup(addr, mask+8)) != NULL) {
                snprintf(tp->resolved_name, MAXNAMELEN, "%s_%02x:%02x:%02x:%02x:%02x",
                        name, addr[1] & (0xFF >> mask), addr[2], addr[3],
                        addr[4], addr[5]);
                tp->status = HASHETHER_STATUS_RESOLVED_DUMMY;
                return tp;
            }
        } while (mask--);

        mask = 7;
        do {
            /* Not even the topmost byte participates fully */
            if ((name = wka_name_lookup(addr, mask)) != NULL) {
                snprintf(tp->resolved_name, MAXNAMELEN, "%s_%02x:%02x:%02x:%02x:%02x:%02x",
                        name, addr[0] & (0xFF >> mask), addr[1], addr[2],
                        addr[3], addr[4], addr[5]);
                tp->status = HASHETHER_STATUS_RESOLVED_DUMMY;
                return tp;
            }
        } while (--mask); /* Work down to the last bit */

        /* No match whatsoever. */
        set_address(&ether_addr, AT_ETHER, 6, addr);
        address_to_str_buf(&ether_addr, tp->resolved_name, MAXNAMELEN);
        tp->status = HASHETHER_STATUS_RESOLVED_DUMMY;
        return tp;
    }
    ws_assert_not_reached();
} /* eth_addr_resolve */