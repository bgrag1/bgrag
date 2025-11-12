tvb_get_manuf_name_if_known(tvbuff_t *tvb, gint offset)
{
    guint8 buf[6] = { 0 };
    tvb_memcpy(tvb, buf, offset, 3);
    return get_manuf_name_if_known(buf, sizeof(buf));
}