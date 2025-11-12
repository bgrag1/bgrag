static yyjson_mut_val *unsafe_yyjson_mut_val_mut_copy(yyjson_mut_doc *m_doc,
                                                      yyjson_mut_val *m_vals) {
    /*
     The mutable object or array stores all sub-values in a circular linked
     list, so we can traverse them in the same loop. The traversal starts from
     the last item, continues with the first item in a list, and ends with the
     second to last item, which needs to be linked to the last item to close the
     circle.
     */
    
    yyjson_mut_val *m_val = unsafe_yyjson_mut_val(m_doc, 1);
    if (unlikely(!m_val)) return NULL;
    m_val->tag = m_vals->tag;
    
    switch (unsafe_yyjson_get_type(m_vals)) {
        case YYJSON_TYPE_OBJ:
        case YYJSON_TYPE_ARR:
            if (unsafe_yyjson_get_len(m_vals) > 0) {
                yyjson_mut_val *last = (yyjson_mut_val *)m_vals->uni.ptr;
                yyjson_mut_val *next = last->next, *prev;
                prev = unsafe_yyjson_mut_val_mut_copy(m_doc, last);
                if (!prev) return NULL;
                m_val->uni.ptr = (void *)prev;
                while (next != last) {
                    prev->next = unsafe_yyjson_mut_val_mut_copy(m_doc, next);
                    if (!prev->next) return NULL;
                    prev = prev->next;
                    next = next->next;
                }
                prev->next = (yyjson_mut_val *)m_val->uni.ptr;
            }
            break;
        
        case YYJSON_TYPE_RAW:
        case YYJSON_TYPE_STR: {
            const char *str = m_vals->uni.str;
            usize str_len = unsafe_yyjson_get_len(m_vals);
            m_val->uni.str = unsafe_yyjson_mut_strncpy(m_doc, str, str_len);
            if (!m_val->uni.str) return NULL;
            break;
        }
        
        default:
            m_val->uni = m_vals->uni;
            break;
    }
    
    return m_val;
}