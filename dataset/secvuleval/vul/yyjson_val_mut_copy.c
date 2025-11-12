yyjson_mut_val *yyjson_val_mut_copy(yyjson_mut_doc *m_doc,
                                    yyjson_val *i_vals) {
    /*
     The immutable object or array stores all sub-values in a contiguous memory,
     We copy them to another contiguous memory as mutable values,
     then reconnect the mutable values with the original relationship.
     */
    
    usize i_vals_len;
    yyjson_mut_val *m_vals, *m_val;
    yyjson_val *i_val, *i_end;
    
    if (!m_doc || !i_vals) return NULL;
    i_end = unsafe_yyjson_get_next(i_vals);
    i_vals_len = (usize)(unsafe_yyjson_get_next(i_vals) - i_vals);
    m_vals = unsafe_yyjson_mut_val(m_doc, i_vals_len);
    if (!m_vals) return NULL;
    i_val = i_vals;
    m_val = m_vals;
    
    for (; i_val < i_end; i_val++, m_val++) {
        yyjson_type type = unsafe_yyjson_get_type(i_val);
        m_val->tag = i_val->tag;
        m_val->uni.u64 = i_val->uni.u64;
        if (type == YYJSON_TYPE_STR || type == YYJSON_TYPE_RAW) {
            const char *str = i_val->uni.str;
            usize str_len = unsafe_yyjson_get_len(i_val);
            m_val->uni.str = unsafe_yyjson_mut_strncpy(m_doc, str, str_len);
            if (!m_val->uni.str) return NULL;
        } else if (type == YYJSON_TYPE_ARR) {
            usize len = unsafe_yyjson_get_len(i_val);
            if (len > 0) {
                yyjson_val *ii_val = i_val + 1, *ii_next;
                yyjson_mut_val *mm_val = m_val + 1, *mm_ctn = m_val, *mm_next;
                while (len-- > 1) {
                    ii_next = unsafe_yyjson_get_next(ii_val);
                    mm_next = mm_val + (ii_next - ii_val);
                    mm_val->next = mm_next;
                    ii_val = ii_next;
                    mm_val = mm_next;
                }
                mm_val->next = mm_ctn + 1;
                mm_ctn->uni.ptr = mm_val;
            }
        } else if (type == YYJSON_TYPE_OBJ) {
            usize len = unsafe_yyjson_get_len(i_val);
            if (len > 0) {
                yyjson_val *ii_key = i_val + 1, *ii_nextkey;
                yyjson_mut_val *mm_key = m_val + 1, *mm_ctn = m_val;
                yyjson_mut_val *mm_nextkey;
                while (len-- > 1) {
                    ii_nextkey = unsafe_yyjson_get_next(ii_key + 1);
                    mm_nextkey = mm_key + (ii_nextkey - ii_key);
                    mm_key->next = mm_key + 1;
                    mm_key->next->next = mm_nextkey;
                    ii_key = ii_nextkey;
                    mm_key = mm_nextkey;
                }
                mm_key->next = mm_key + 1;
                mm_key->next->next = mm_ctn + 1;
                mm_ctn->uni.ptr = mm_key;
            }
        }
    }
    
    return m_vals;
}