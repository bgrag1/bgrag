get_manuf_name(const guint8 *addr)
{
    hashmanuf_t *manuf_value;

    manuf_value = manuf_name_lookup(addr);
    if (gbl_resolv_flags.mac_name && manuf_value->status != HASHETHER_STATUS_UNRESOLVED)
        return manuf_value->resolved_name;

    return manuf_value->hexaddr;

} /* get_manuf_name */