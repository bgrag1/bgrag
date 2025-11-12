get_manuf_name_if_known(const guint8 *addr)
{
    hashmanuf_t *manuf_value;
    guint manuf_key;
    guint8 oct;

    /* manuf needs only the 3 most significant octets of the ethernet address */
    manuf_key = addr[0];
    manuf_key = manuf_key<<8;
    oct = addr[1];
    manuf_key = manuf_key | oct;
    manuf_key = manuf_key<<8;
    oct = addr[2];
    manuf_key = manuf_key | oct;

    manuf_value = (hashmanuf_t *)wmem_map_lookup(manuf_hashtable, GUINT_TO_POINTER(manuf_key));
    if (manuf_value != NULL && manuf_value->status != HASHETHER_STATUS_UNRESOLVED) {
        return manuf_value->resolved_longname;
    }

    /* Try the global manuf tables. */
    const char *short_name, *long_name;
    short_name = ws_manuf_lookup_str(addr, &long_name);
    if (short_name != NULL) {
        /* Found it */
        return long_name;
    }

    return NULL;

} /* get_manuf_name_if_known */