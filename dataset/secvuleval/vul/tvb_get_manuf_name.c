tvb_get_manuf_name(tvbuff_t *tvb, gint offset)
{
    return get_manuf_name(tvb_get_ptr(tvb, offset, 3));
}