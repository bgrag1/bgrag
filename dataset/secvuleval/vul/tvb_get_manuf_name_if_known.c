tvb_get_manuf_name_if_known(tvbuff_t *tvb, gint offset)
{
    return get_manuf_name_if_known(tvb_get_ptr(tvb, offset, 3));
}