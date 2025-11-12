eui64_to_display(wmem_allocator_t *allocator, const guint64 addr_eui64)
{
    guint8 *addr = (guint8 *)wmem_alloc(NULL, 8);
    hashmanuf_t *manuf_value;
    gchar *ret;

    /* Copy and convert the address to network byte order. */
    *(guint64 *)(void *)(addr) = pntoh64(&(addr_eui64));

    manuf_value = manuf_name_lookup(addr, 8);
    if (!gbl_resolv_flags.mac_name || (manuf_value->status == HASHETHER_STATUS_UNRESOLVED)) {
        ret = wmem_strdup_printf(allocator, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
    } else {
        ret = wmem_strdup_printf(allocator, "%s_%02x:%02x:%02x:%02x:%02x", manuf_value->resolved_name, addr[3], addr[4], addr[5], addr[6], addr[7]);
    }

    wmem_free(NULL, addr);
    return ret;
} /* eui64_to_display */