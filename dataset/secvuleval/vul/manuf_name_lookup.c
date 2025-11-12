manuf_name_lookup(const guint8 *addr)
{
    guint32       manuf_key;
    guint8       oct;
    hashmanuf_t  *manuf_value;

    /* manuf needs only the 3 most significant octets of the ethernet address */
    manuf_key = addr[0];
    manuf_key = manuf_key<<8;
    oct = addr[1];
    manuf_key = manuf_key | oct;
    manuf_key = manuf_key<<8;
    oct = addr[2];
    manuf_key = manuf_key | oct;


    /* first try to find a "perfect match" */
    manuf_value = (hashmanuf_t*)wmem_map_lookup(manuf_hashtable, GUINT_TO_POINTER(manuf_key));
    if (manuf_value != NULL) {
        return manuf_value;
    }

    /* Mask out the broadcast/multicast flag but not the locally
     * administered flag as locally administered means: not assigned
     * by the IEEE but the local administrator instead.
     * 0x01 multicast / broadcast bit
     * 0x02 locally administered bit */
    if ((manuf_key & 0x00010000) != 0) {
        manuf_key &= 0x00FEFFFF;
        manuf_value = (hashmanuf_t*)wmem_map_lookup(manuf_hashtable, GUINT_TO_POINTER(manuf_key));
        if (manuf_value != NULL) {
            return manuf_value;
        }
    }

    /* Try the global manuf tables. */
    const char *short_name, *long_name;
    short_name = ws_manuf_lookup_str(addr, &long_name);
    if (short_name != NULL) {
        /* Found it */
        return manuf_hash_new_entry(addr, short_name, long_name);
    }

    /* Add the address as a hex string */
    return manuf_hash_new_entry(addr, NULL, NULL);

} /* manuf_name_lookup */