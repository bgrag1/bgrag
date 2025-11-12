bool unsafe_yyjson_mut_equals(yyjson_mut_val *lhs, yyjson_mut_val *rhs) {
    yyjson_type type = unsafe_yyjson_get_type(lhs);
    if (type != unsafe_yyjson_get_type(rhs)) return false;
    
    switch (type) {
        case YYJSON_TYPE_OBJ: {
            usize len = unsafe_yyjson_get_len(lhs);
            if (len != unsafe_yyjson_get_len(rhs)) return false;
            if (len > 0) {
                yyjson_mut_obj_iter iter;
                yyjson_mut_obj_iter_init(rhs, &iter);
                lhs = (yyjson_mut_val *)lhs->uni.ptr;
                while (len-- > 0) {
                    rhs = yyjson_mut_obj_iter_getn(&iter, lhs->uni.str,
                                                   unsafe_yyjson_get_len(lhs));
                    if (!rhs || !unsafe_yyjson_mut_equals(lhs->next, rhs))
                        return false;
                    lhs = lhs->next->next;
                }
            }
            /* yyjson allows duplicate keys, so the check may be inaccurate */
            return true;
        }
        
        case YYJSON_TYPE_ARR: {
            usize len = unsafe_yyjson_get_len(lhs);
            if (len != unsafe_yyjson_get_len(rhs)) return false;
            if (len > 0) {
                lhs = (yyjson_mut_val *)lhs->uni.ptr;
                rhs = (yyjson_mut_val *)rhs->uni.ptr;
                while (len-- > 0) {
                    if (!unsafe_yyjson_mut_equals(lhs, rhs)) return false;
                    lhs = lhs->next;
                    rhs = rhs->next;
                }
            }
            return true;
        }
        
        case YYJSON_TYPE_NUM:
            return unsafe_yyjson_num_equals(lhs, rhs);
        
        case YYJSON_TYPE_RAW:
        case YYJSON_TYPE_STR:
            return unsafe_yyjson_str_equals(lhs, rhs);
        
        case YYJSON_TYPE_NULL:
        case YYJSON_TYPE_BOOL:
            return lhs->tag == rhs->tag;
        
        default:
            return false;
    }
}