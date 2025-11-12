static_noinline yyjson_doc *read_root_single(u8 *hdr,
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
    
    usize hdr_len; /* value count used by doc */
    usize alc_num; /* value count capacity */
    yyjson_val *val_hdr; /* the head of allocated values */
    yyjson_val *val; /* current value */
    yyjson_doc *doc; /* the JSON document, equals to val_hdr */
    const char *msg; /* error message */
    
    bool raw; /* read number as raw */
    bool inv; /* allow invalid unicode */
    u8 *raw_end; /* raw end for null-terminator */
    u8 **pre; /* previous raw end pointer */
    
    hdr_len = sizeof(yyjson_doc) / sizeof(yyjson_val);
    hdr_len += (sizeof(yyjson_doc) % sizeof(yyjson_val)) > 0;
    alc_num = hdr_len + 1; /* single value */
    
    val_hdr = (yyjson_val *)alc.malloc(alc.ctx, alc_num * sizeof(yyjson_val));
    if (unlikely(!val_hdr)) goto fail_alloc;
    val = val_hdr + hdr_len;
    raw = has_read_flag(NUMBER_AS_RAW) || has_read_flag(BIGNUM_AS_RAW);
    inv = has_read_flag(ALLOW_INVALID_UNICODE) != 0;
    raw_end = NULL;
    pre = raw ? &raw_end : NULL;
    
    if (char_is_number(*cur)) {
        if (likely(read_number(&cur, pre, flg, val, &msg))) goto doc_end;
        goto fail_number;
    }
    if (*cur == '"') {
        if (likely(read_string(&cur, end, inv, val, &msg))) goto doc_end;
        goto fail_string;
    }
    if (*cur == 't') {
        if (likely(read_true(&cur, val))) goto doc_end;
        goto fail_literal;
    }
    if (*cur == 'f') {
        if (likely(read_false(&cur, val))) goto doc_end;
        goto fail_literal;
    }
    if (*cur == 'n') {
        if (likely(read_null(&cur, val))) goto doc_end;
        if (has_read_flag(ALLOW_INF_AND_NAN)) {
            if (read_nan(false, &cur, pre, val)) goto doc_end;
        }
        goto fail_literal;
    }
    if (has_read_flag(ALLOW_INF_AND_NAN)) {
        if (read_inf_or_nan(false, &cur, pre, val)) goto doc_end;
    }
    goto fail_character;
    
doc_end:
    /* check invalid contents after json document */
    if (unlikely(cur < end) && !has_read_flag(STOP_WHEN_DONE)) {
        if (has_read_flag(ALLOW_COMMENTS)) {
            if (!skip_spaces_and_comments(&cur)) {
                if (byte_match_2(cur, "/*")) goto fail_comment;
            }
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
    doc->val_read = 1;
    doc->str_pool = has_read_flag(INSITU) ? NULL : (char *)hdr;
    return doc;
    
fail_string:
    return_err(cur, INVALID_STRING, msg);
fail_number:
    return_err(cur, INVALID_NUMBER, msg);
fail_alloc:
    return_err(cur, MEMORY_ALLOCATION, "memory allocation failed");
fail_literal:
    return_err(cur, LITERAL, "invalid literal");
fail_comment:
    return_err(cur, INVALID_COMMENT, "unclosed multiline comment");
fail_character:
    return_err(cur, UNEXPECTED_CHARACTER, "unexpected character");
fail_garbage:
    return_err(cur, UNEXPECTED_CONTENT, "unexpected content after document");
fail_recursion:
    return_err(cur, RECURSION_DEPTH, "array and object recursion depth exceeded");
    
#undef return_err
}