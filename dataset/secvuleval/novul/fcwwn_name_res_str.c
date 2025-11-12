static const gchar* fcwwn_name_res_str(const address* addr)
{
    const guint8 *addrp = (const guint8*)addr->data;
    int fmt;
    guint8 oui[6];

    fmt = (addrp[0] & 0xF0) >> 4;
    switch (fmt) {

    case FC_NH_NAA_IEEE:
    case FC_NH_NAA_IEEE_E:

        memcpy (oui, &addrp[2], 6);
        return get_manuf_name(oui, sizeof(oui));

    case FC_NH_NAA_IEEE_R:
        oui[0] = ((addrp[0] & 0x0F) << 4) | ((addrp[1] & 0xF0) >> 4);
        oui[1] = ((addrp[1] & 0x0F) << 4) | ((addrp[2] & 0xF0) >> 4);
        oui[2] = ((addrp[2] & 0x0F) << 4) | ((addrp[3] & 0xF0) >> 4);
        oui[3] = ((addrp[3] & 0x0F) << 4) | ((addrp[4] & 0xF0) >> 4);
        oui[4] = ((addrp[4] & 0x0F) << 4) | ((addrp[5] & 0xF0) >> 4);
        oui[5] = ((addrp[5] & 0x0F) << 4) | ((addrp[6] & 0xF0) >> 4);

        return get_manuf_name(oui, sizeof(oui));
    }

    return "";
}