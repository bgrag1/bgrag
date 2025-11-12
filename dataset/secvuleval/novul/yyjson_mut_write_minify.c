static_inline u8 *yyjson_mut_write_minify(const yyjson_mut_val *root,
                                          usize estimated_val_num,
                                          yyjson_write_flag flg,
                                          yyjson_alc alc,
                                          usize *dat_len,
                                          yyjson_write_err *err) {
    
#define return_err(_code, _msg) do { \
    *dat_len = 0; \
    err->code = YYJSON_WRITE_ERROR_##_code; \
    err->msg = _msg; \
    if (hdr) alc.free(alc.ctx, hdr); \
    return NULL; \
} while (false)
    
#define incr_len(_len) do { \
    ext_len = (usize)(_len); \
    if (unlikely((u8 *)(cur + ext_len) >= (u8 *)ctx)) { \
        alc_inc = yyjson_max(alc_len / 2, ext_len); \
        alc_inc = size_align_up(alc_inc, sizeof(yyjson_mut_write_ctx)); \
        if ((sizeof(usize) < 8) && size_add_is_overflow(alc_len, alc_inc)) \
            goto fail_alloc; \
        alc_len += alc_inc; \
        tmp = (u8 *)alc.realloc(alc.ctx, hdr, alc_len - alc_inc, alc_len); \
        if (unlikely(!tmp)) goto fail_alloc; \
        ctx_len = (usize)(end - (u8 *)ctx); \
        ctx_tmp = (yyjson_mut_write_ctx *)(void *)(tmp + (alc_len - ctx_len)); \
        memmove((void *)ctx_tmp, (void *)(tmp + ((u8 *)ctx - hdr)), ctx_len); \
        ctx = ctx_tmp; \
        cur = tmp + (cur - hdr); \
        end = tmp + alc_len; \
        hdr = tmp; \
    } \
} while (false)
    
#define check_str_len(_len) do { \
    if ((sizeof(usize) < 8) && (_len >= (USIZE_MAX - 16) / 6)) \
        goto fail_alloc; \
} while (false)
    
    yyjson_mut_val *val, *ctn;
    yyjson_type val_type;
    usize ctn_len, ctn_len_tmp;
    bool ctn_obj, ctn_obj_tmp, is_key;
    u8 *hdr, *cur, *end, *tmp;
    yyjson_mut_write_ctx *ctx, *ctx_tmp;
    usize alc_len, alc_inc, ctx_len, ext_len, str_len;
    const u8 *str_ptr;
    const char_enc_type *enc_table = get_enc_table_with_flag(flg);
    bool cpy = (enc_table == enc_table_cpy);
    bool esc = has_write_flag(ESCAPE_UNICODE) != 0;
    bool inv = has_write_flag(ALLOW_INVALID_UNICODE) != 0;
    bool newline = has_write_flag(NEWLINE_AT_END) != 0;
    
    alc_len = estimated_val_num * YYJSON_WRITER_ESTIMATED_MINIFY_RATIO + 64;
    alc_len = size_align_up(alc_len, sizeof(yyjson_mut_write_ctx));
    hdr = (u8 *)alc.malloc(alc.ctx, alc_len);
    if (!hdr) goto fail_alloc;
    cur = hdr;
    end = hdr + alc_len;
    ctx = (yyjson_mut_write_ctx *)(void *)end;
    
doc_begin:
    val = constcast(yyjson_mut_val *)root;
    val_type = unsafe_yyjson_get_type(val);
    ctn_obj = (val_type == YYJSON_TYPE_OBJ);
    ctn_len = unsafe_yyjson_get_len(val) << (u8)ctn_obj;
    *cur++ = (u8)('[' | ((u8)ctn_obj << 5));
    ctn = val;
    val = (yyjson_mut_val *)val->uni.ptr; /* tail */
    val = ctn_obj ? val->next->next : val->next;
    
val_begin:
    val_type = unsafe_yyjson_get_type(val);
    if (val_type == YYJSON_TYPE_STR) {
        is_key = ((u8)ctn_obj & (u8)~ctn_len);
        str_len = unsafe_yyjson_get_len(val);
        str_ptr = (const u8 *)unsafe_yyjson_get_str(val);
        check_str_len(str_len);
        incr_len(str_len * 6 + 16);
        if (likely(cpy) && unsafe_yyjson_get_subtype(val)) {
            cur = write_string_noesc(cur, str_ptr, str_len);
        } else {
            cur = write_string(cur, esc, inv, str_ptr, str_len, enc_table);
            if (unlikely(!cur)) goto fail_str;
        }
        *cur++ = is_key ? ':' : ',';
        goto val_end;
    }
    if (val_type == YYJSON_TYPE_NUM) {
        incr_len(32);
        cur = write_number(cur, (yyjson_val *)val, flg);
        if (unlikely(!cur)) goto fail_num;
        *cur++ = ',';
        goto val_end;
    }
    if ((val_type & (YYJSON_TYPE_ARR & YYJSON_TYPE_OBJ)) ==
                    (YYJSON_TYPE_ARR & YYJSON_TYPE_OBJ)) {
        ctn_len_tmp = unsafe_yyjson_get_len(val);
        ctn_obj_tmp = (val_type == YYJSON_TYPE_OBJ);
        incr_len(16);
        if (unlikely(ctn_len_tmp == 0)) {
            /* write empty container */
            *cur++ = (u8)('[' | ((u8)ctn_obj_tmp << 5));
            *cur++ = (u8)(']' | ((u8)ctn_obj_tmp << 5));
            *cur++ = ',';
            goto val_end;
        } else {
            /* push context, setup new container */
            yyjson_mut_write_ctx_set(--ctx, ctn, ctn_len, ctn_obj);
            ctn_len = ctn_len_tmp << (u8)ctn_obj_tmp;
            ctn_obj = ctn_obj_tmp;
            *cur++ = (u8)('[' | ((u8)ctn_obj << 5));
            ctn = val;
            val = (yyjson_mut_val *)ctn->uni.ptr; /* tail */
            val = ctn_obj ? val->next->next : val->next;
            goto val_begin;
        }
    }
    if (val_type == YYJSON_TYPE_BOOL) {
        incr_len(16);
        cur = write_bool(cur, unsafe_yyjson_get_bool(val));
        cur++;
        goto val_end;
    }
    if (val_type == YYJSON_TYPE_NULL) {
        incr_len(16);
        cur = write_null(cur);
        cur++;
        goto val_end;
    }
    if (val_type == YYJSON_TYPE_RAW) {
        str_len = unsafe_yyjson_get_len(val);
        str_ptr = (const u8 *)unsafe_yyjson_get_str(val);
        check_str_len(str_len);
        incr_len(str_len + 2);
        cur = write_raw(cur, str_ptr, str_len);
        *cur++ = ',';
        goto val_end;
    }
    goto fail_type;
    
val_end:
    ctn_len--;
    if (unlikely(ctn_len == 0)) goto ctn_end;
    val = val->next;
    goto val_begin;
    
ctn_end:
    cur--;
    *cur++ = (u8)(']' | ((u8)ctn_obj << 5));
    *cur++ = ',';
    if (unlikely((u8 *)ctx >= end)) goto doc_end;
    val = ctn->next;
    yyjson_mut_write_ctx_get(ctx++, &ctn, &ctn_len, &ctn_obj);
    ctn_len--;
    if (likely(ctn_len > 0)) {
        goto val_begin;
    } else {
        goto ctn_end;
    }
    
doc_end:
    if (newline) {
        incr_len(2);
        *(cur - 1) = '\n';
        cur++;
    }
    *--cur = '\0';
    *dat_len = (usize)(cur - hdr);
    err->code = YYJSON_WRITE_SUCCESS;
    err->msg = "success";
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
#undef incr_len
#undef check_str_len
}