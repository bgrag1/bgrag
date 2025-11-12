static_inline yyjson_doc *read_root_pretty(u8 *hdr,
                                           u8 *cur,
                                           u8 *end,
                                           yyjson_alc alc,
                                           yyjson_read_flag flg,
                                           yyjson_read_err *err) {
    
#define return_err(_pos, _code, _msg) do { \
    if (is_truncated_end(hdr, _pos, end, YYJSON_READ_ERROR_##_code, flg)) { \
        err->pos = (usize)(end - hdr); \
        err->code = YYJSON_READ_ERROR_UNEXPECTED_END; \
        err->msg = "unexpected end of data"; \
    } else { \
        err->pos = (usize)(_pos - hdr); \
        err->code = YYJSON_READ_ERROR_##_code; \
        err->msg = _msg; \
    } \
    if (val_hdr) alc.free(alc.ctx, (void *)val_hdr); \
    return NULL; \
} while (false)
    
#define val_incr() do { \
    val++; \
    if (unlikely(val >= val_end)) { \
        usize alc_old = alc_len; \
        alc_len += alc_len / 2; \
        if ((sizeof(usize) < 8) && (alc_len >= alc_max)) goto fail_alloc; \
        val_tmp = (yyjson_val *)alc.realloc(alc.ctx, (void *)val_hdr, \
            alc_old * sizeof(yyjson_val), \
            alc_len * sizeof(yyjson_val)); \
        if ((!val_tmp)) goto fail_alloc; \
        val = val_tmp + (usize)(val - val_hdr); \
        ctn = val_tmp + (usize)(ctn - val_hdr); \
        val_hdr = val_tmp; \
        val_end = val_tmp + (alc_len - 2); \
    } \
} while (false)
    
    usize dat_len; /* data length in bytes, hint for allocator */
    usize hdr_len; /* value count used by yyjson_doc */
    usize alc_len; /* value count allocated */
    usize alc_max; /* maximum value count for allocator */
    usize ctn_len; /* the number of elements in current container */
    yyjson_val *val_hdr; /* the head of allocated values */
    yyjson_val *val_end; /* the end of allocated values */
    yyjson_val *val_tmp; /* temporary pointer for realloc */
    yyjson_val *val; /* current JSON value */
    yyjson_val *ctn; /* current container */
    yyjson_val *ctn_parent; /* parent of current container */
    yyjson_doc *doc; /* the JSON document, equals to val_hdr */
    const char *msg; /* error message */
    
    bool raw; /* read number as raw */
    bool inv; /* allow invalid unicode */
    u8 *raw_end; /* raw end for null-terminator */
    u8 **pre; /* previous raw end pointer */
    
    dat_len = has_read_flag(STOP_WHEN_DONE) ? 256 : (usize)(end - cur);
    hdr_len = sizeof(yyjson_doc) / sizeof(yyjson_val);
    hdr_len += (sizeof(yyjson_doc) % sizeof(yyjson_val)) > 0;
    alc_max = USIZE_MAX / sizeof(yyjson_val);
    alc_len = hdr_len + (dat_len / YYJSON_READER_ESTIMATED_PRETTY_RATIO) + 4;
    alc_len = yyjson_min(alc_len, alc_max);
    
    val_hdr = (yyjson_val *)alc.malloc(alc.ctx, alc_len * sizeof(yyjson_val));
    if (unlikely(!val_hdr)) goto fail_alloc;
    val_end = val_hdr + (alc_len - 2); /* padding for key-value pair reading */
    val = val_hdr + hdr_len;
    ctn = val;
    ctn_len = 0;
    raw = has_read_flag(NUMBER_AS_RAW) || has_read_flag(BIGNUM_AS_RAW);
    inv = has_read_flag(ALLOW_INVALID_UNICODE) != 0;
    raw_end = NULL;
    pre = raw ? &raw_end : NULL;
    
    if (*cur++ == '{') {
        ctn->tag = YYJSON_TYPE_OBJ;
        ctn->uni.ofs = 0;
        if (*cur == '\n') cur++;
        goto obj_key_begin;
    } else {
        ctn->tag = YYJSON_TYPE_ARR;
        ctn->uni.ofs = 0;
        if (*cur == '\n') cur++;
        goto arr_val_begin;
    }
    
arr_begin:
    /* save current container */
    ctn->tag = (((u64)ctn_len + 1) << YYJSON_TAG_BIT) |
               (ctn->tag & YYJSON_TAG_MASK);
    
    /* create a new array value, save parent container offset */
    val_incr();
    val->tag = YYJSON_TYPE_ARR;
    val->uni.ofs = (usize)((u8 *)val - (u8 *)ctn);
    
    /* push the new array value as current container */
    ctn = val;
    ctn_len = 0;
    if (*cur == '\n') cur++;
    
arr_val_begin:
#if YYJSON_IS_REAL_GCC
    while (true) repeat16({
        if (byte_match_2(cur, "  ")) cur += 2;
        else break;
    })
#else
    while (true) repeat16({
        if (likely(byte_match_2(cur, "  "))) cur += 2;
        else break;
    })
#endif
    
    if (*cur == '{') {
        cur++;
        goto obj_begin;
    }
    if (*cur == '[') {
        cur++;
        goto arr_begin;
    }
    if (char_is_number(*cur)) {
        val_incr();
        ctn_len++;
        if (likely(read_number(&cur, pre, flg, val, &msg))) goto arr_val_end;
        goto fail_number;
    }
    if (*cur == '"') {
        val_incr();
        ctn_len++;
        if (likely(read_string(&cur, end, inv, val, &msg))) goto arr_val_end;
        goto fail_string;
    }
    if (*cur == 't') {
        val_incr();
        ctn_len++;
        if (likely(read_true(&cur, val))) goto arr_val_end;
        goto fail_literal;
    }
    if (*cur == 'f') {
        val_incr();
        ctn_len++;
        if (likely(read_false(&cur, val))) goto arr_val_end;
        goto fail_literal;
    }
    if (*cur == 'n') {
        val_incr();
        ctn_len++;
        if (likely(read_null(&cur, val))) goto arr_val_end;
        if (has_read_flag(ALLOW_INF_AND_NAN)) {
            if (read_nan(false, &cur, pre, val)) goto arr_val_end;
        }
        goto fail_literal;
    }
    if (*cur == ']') {
        cur++;
        if (likely(ctn_len == 0)) goto arr_end;
        if (has_read_flag(ALLOW_TRAILING_COMMAS)) goto arr_end;
        while (*cur != ',') cur--;
        goto fail_trailing_comma;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur));
        goto arr_val_begin;
    }
    if (has_read_flag(ALLOW_INF_AND_NAN) &&
        (*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        val_incr();
        ctn_len++;
        if (read_inf_or_nan(false, &cur, pre, val)) goto arr_val_end;
        goto fail_character;
    }
    if (has_read_flag(ALLOW_COMMENTS)) {
        if (skip_spaces_and_comments(&cur)) goto arr_val_begin;
        if (byte_match_2(cur, "/*")) goto fail_comment;
    }
    goto fail_character;
    
arr_val_end:
    if (byte_match_2(cur, ",\n")) {
        cur += 2;
        goto arr_val_begin;
    }
    if (*cur == ',') {
        cur++;
        goto arr_val_begin;
    }
    if (*cur == ']') {
        cur++;
        goto arr_end;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur));
        goto arr_val_end;
    }
    if (has_read_flag(ALLOW_COMMENTS)) {
        if (skip_spaces_and_comments(&cur)) goto arr_val_end;
        if (byte_match_2(cur, "/*")) goto fail_comment;
    }
    goto fail_character;
    
arr_end:
    /* get parent container */
    ctn_parent = (yyjson_val *)(void *)((u8 *)ctn - ctn->uni.ofs);
    
    /* save the next sibling value offset */
    ctn->uni.ofs = (usize)((u8 *)val - (u8 *)ctn) + sizeof(yyjson_val);
    ctn->tag = ((ctn_len) << YYJSON_TAG_BIT) | YYJSON_TYPE_ARR;
    if (unlikely(ctn == ctn_parent)) goto doc_end;
    
    /* pop parent as current container */
    ctn = ctn_parent;
    ctn_len = (usize)(ctn->tag >> YYJSON_TAG_BIT);
    if (*cur == '\n') cur++;
    if ((ctn->tag & YYJSON_TYPE_MASK) == YYJSON_TYPE_OBJ) {
        goto obj_val_end;
    } else {
        goto arr_val_end;
    }
    
obj_begin:
    /* push container */
    ctn->tag = (((u64)ctn_len + 1) << YYJSON_TAG_BIT) |
               (ctn->tag & YYJSON_TAG_MASK);
    val_incr();
    val->tag = YYJSON_TYPE_OBJ;
    /* offset to the parent */
    val->uni.ofs = (usize)((u8 *)val - (u8 *)ctn);
    ctn = val;
    ctn_len = 0;
    if (*cur == '\n') cur++;
    
obj_key_begin:
#if YYJSON_IS_REAL_GCC
    while (true) repeat16({
        if (byte_match_2(cur, "  ")) cur += 2;
        else break;
    })
#else
    while (true) repeat16({
        if (likely(byte_match_2(cur, "  "))) cur += 2;
        else break;
    })
#endif
    if (likely(*cur == '"')) {
        val_incr();
        ctn_len++;
        if (likely(read_string(&cur, end, inv, val, &msg))) goto obj_key_end;
        goto fail_string;
    }
    if (likely(*cur == '}')) {
        cur++;
        if (likely(ctn_len == 0)) goto obj_end;
        if (has_read_flag(ALLOW_TRAILING_COMMAS)) goto obj_end;
        while (*cur != ',') cur--;
        goto fail_trailing_comma;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur));
        goto obj_key_begin;
    }
    if (has_read_flag(ALLOW_COMMENTS)) {
        if (skip_spaces_and_comments(&cur)) goto obj_key_begin;
        if (byte_match_2(cur, "/*")) goto fail_comment;
    }
    goto fail_character;
    
obj_key_end:
    if (byte_match_2(cur, ": ")) {
        cur += 2;
        goto obj_val_begin;
    }
    if (*cur == ':') {
        cur++;
        goto obj_val_begin;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur));
        goto obj_key_end;
    }
    if (has_read_flag(ALLOW_COMMENTS)) {
        if (skip_spaces_and_comments(&cur)) goto obj_key_end;
        if (byte_match_2(cur, "/*")) goto fail_comment;
    }
    goto fail_character;
    
obj_val_begin:
    if (*cur == '"') {
        val++;
        ctn_len++;
        if (likely(read_string(&cur, end, inv, val, &msg))) goto obj_val_end;
        goto fail_string;
    }
    if (char_is_number(*cur)) {
        val++;
        ctn_len++;
        if (likely(read_number(&cur, pre, flg, val, &msg))) goto obj_val_end;
        goto fail_number;
    }
    if (*cur == '{') {
        cur++;
        goto obj_begin;
    }
    if (*cur == '[') {
        cur++;
        goto arr_begin;
    }
    if (*cur == 't') {
        val++;
        ctn_len++;
        if (likely(read_true(&cur, val))) goto obj_val_end;
        goto fail_literal;
    }
    if (*cur == 'f') {
        val++;
        ctn_len++;
        if (likely(read_false(&cur, val))) goto obj_val_end;
        goto fail_literal;
    }
    if (*cur == 'n') {
        val++;
        ctn_len++;
        if (likely(read_null(&cur, val))) goto obj_val_end;
        if (has_read_flag(ALLOW_INF_AND_NAN)) {
            if (read_nan(false, &cur, pre, val)) goto obj_val_end;
        }
        goto fail_literal;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur));
        goto obj_val_begin;
    }
    if (has_read_flag(ALLOW_INF_AND_NAN) &&
        (*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        val++;
        ctn_len++;
        if (read_inf_or_nan(false, &cur, pre, val)) goto obj_val_end;
        goto fail_character;
    }
    if (has_read_flag(ALLOW_COMMENTS)) {
        if (skip_spaces_and_comments(&cur)) goto obj_val_begin;
        if (byte_match_2(cur, "/*")) goto fail_comment;
    }
    goto fail_character;
    
obj_val_end:
    if (byte_match_2(cur, ",\n")) {
        cur += 2;
        goto obj_key_begin;
    }
    if (likely(*cur == ',')) {
        cur++;
        goto obj_key_begin;
    }
    if (likely(*cur == '}')) {
        cur++;
        goto obj_end;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur));
        goto obj_val_end;
    }
    if (has_read_flag(ALLOW_COMMENTS)) {
        if (skip_spaces_and_comments(&cur)) goto obj_val_end;
        if (byte_match_2(cur, "/*")) goto fail_comment;
    }
    goto fail_character;
    
obj_end:
    /* pop container */
    ctn_parent = (yyjson_val *)(void *)((u8 *)ctn - ctn->uni.ofs);
    /* point to the next value */
    ctn->uni.ofs = (usize)((u8 *)val - (u8 *)ctn) + sizeof(yyjson_val);
    ctn->tag = (ctn_len << (YYJSON_TAG_BIT - 1)) | YYJSON_TYPE_OBJ;
    if (unlikely(ctn == ctn_parent)) goto doc_end;
    ctn = ctn_parent;
    ctn_len = (usize)(ctn->tag >> YYJSON_TAG_BIT);
    if (*cur == '\n') cur++;
    if ((ctn->tag & YYJSON_TYPE_MASK) == YYJSON_TYPE_OBJ) {
        goto obj_val_end;
    } else {
        goto arr_val_end;
    }
    
doc_end:
    /* check invalid contents after json document */
    if (unlikely(cur < end) && !has_read_flag(STOP_WHEN_DONE)) {
        if (has_read_flag(ALLOW_COMMENTS)) {
            skip_spaces_and_comments(&cur);
            if (byte_match_2(cur, "/*")) goto fail_comment;
        } else {
            while (char_is_space(*cur)) cur++;
        }
        if (unlikely(cur < end)) goto fail_garbage;
    }
    
    if (pre && *pre) **pre = '\0';
    doc = (yyjson_doc *)val_hdr;
    doc->root = val_hdr + hdr_len;
    doc->alc = alc;
    doc->dat_read = (usize)(cur - hdr);
    doc->val_read = (usize)((val - val_hdr)) - hdr_len + 1;
    doc->str_pool = has_read_flag(INSITU) ? NULL : (char *)hdr;
    return doc;
    
fail_string:
    return_err(cur, INVALID_STRING, msg);
fail_number:
    return_err(cur, INVALID_NUMBER, msg);
fail_alloc:
    return_err(cur, MEMORY_ALLOCATION, "memory allocation failed");
fail_trailing_comma:
    return_err(cur, JSON_STRUCTURE, "trailing comma is not allowed");
fail_literal:
    return_err(cur, LITERAL, "invalid literal");
fail_comment:
    return_err(cur, INVALID_COMMENT, "unclosed multiline comment");
fail_character:
    return_err(cur, UNEXPECTED_CHARACTER, "unexpected character");
fail_garbage:
    return_err(cur, UNEXPECTED_CONTENT, "unexpected content after document");
    
#undef val_incr
#undef return_err
}