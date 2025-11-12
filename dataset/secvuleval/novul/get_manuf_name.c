get_manuf_name(const guint8 *addr, size_t size)
{
    hashmanuf_t *manuf_value;

    manuf_value = manuf_name_lookup(addr, size);
    if (gbl_resolv_flags.mac_name && manuf_value->status != HASHETHER_STATUS_UNRESOLVED)
        return manuf_value->resolved_name;

    return manuf_value->hexaddr;

} /* get_manuf_name */