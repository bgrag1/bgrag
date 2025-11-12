static_inline u8 *yyjson_write_single(yyjson_val *val,
                                      yyjson_write_flag flg,
                                      yyjson_alc alc,
                                      usize *dat_len,
                                      yyjson_write_err *err) {
    
#define return_err(_code, _msg) do { \
    if (hdr) alc.free(alc.ctx, (void *)hdr); \
    *dat_len = 0; \
    err->code = YYJSON_WRITE_ERROR_##_code; \
    err->msg = _msg; \
    return NULL; \
} while (false)
    
#define incr_len(_len) do { \
    hdr = (u8 *)alc.malloc(alc.ctx, _len); \
    if (!hdr) goto fail_alloc; \
    cur = hdr; \
} while (false)
    
#define check_str_len(_len) do { \
    if ((sizeof(usize) < 8) && (_len >= (USIZE_MAX - 16) / 6)) \
        goto fail_alloc; \
} while (false)
    
    u8 *hdr = NULL, *cur;
    usize str_len;
    const u8 *str_ptr;
    const char_enc_type *enc_table = get_enc_table_with_flag(flg);
    bool cpy = (enc_table == enc_table_cpy);
    bool esc = has_write_flag(ESCAPE_UNICODE) != 0;
    bool inv = has_write_flag(ALLOW_INVALID_UNICODE) != 0;
    
    switch (unsafe_yyjson_get_type(val)) {
        case YYJSON_TYPE_RAW:
            str_len = unsafe_yyjson_get_len(val);
            str_ptr = (const u8 *)unsafe_yyjson_get_str(val);
            check_str_len(str_len);
            incr_len(str_len + 1);
            cur = write_raw(cur, str_ptr, str_len);
            break;
            
        case YYJSON_TYPE_STR:
            str_len = unsafe_yyjson_get_len(val);
            str_ptr = (const u8 *)unsafe_yyjson_get_str(val);
            check_str_len(str_len);
            incr_len(str_len * 6 + 4);
            if (likely(cpy) && unsafe_yyjson_get_subtype(val)) {
                cur = write_string_noesc(cur, str_ptr, str_len);
            } else {
                cur = write_string(cur, esc, inv, str_ptr, str_len, enc_table);
                if (unlikely(!cur)) goto fail_str;
            }
            break;
            
        case YYJSON_TYPE_NUM:
            incr_len(32);
            cur = write_number(cur, val, flg);
            if (unlikely(!cur)) goto fail_num;
            break;
            
        case YYJSON_TYPE_BOOL:
            incr_len(8);
            cur = write_bool(cur, unsafe_yyjson_get_bool(val));
            break;
            
        case YYJSON_TYPE_NULL:
            incr_len(8);
            cur = write_null(cur);
            break;
            
        case YYJSON_TYPE_ARR:
            incr_len(4);
            byte_copy_2(cur, "[]");
            cur += 2;
            break;
            
        case YYJSON_TYPE_OBJ:
            incr_len(4);
            byte_copy_2(cur, "{}");
            cur += 2;
            break;
            
        default:
            goto fail_type;
    }
    
    *cur = '\0';
    *dat_len = (usize)(cur - hdr);
    memset(err, 0, sizeof(yyjson_write_err));
    return hdr;
    
fail_alloc:
    return_err(MEMORY_ALLOCATION, "memory allocation failed");
fail_type:
    return_err(INVALID_VALUE_TYPE, "invalid JSON value type");
fail_num:
    return_err(NAN_OR_INF, "nan or inf number is not allowed");
fail_str:
    return_err(INVALID_STRING, "invalid utf-8 encoding in string");
    
#undef return_err
#undef check_str_len
#undef incr_len
}