static_inline bool unsafe_yyjson_num_equals(void *lhs, void *rhs) {
    yyjson_val_uni *luni = &((yyjson_val *)lhs)->uni;
    yyjson_val_uni *runi = &((yyjson_val *)rhs)->uni;
    yyjson_subtype lt = unsafe_yyjson_get_subtype(lhs);
    yyjson_subtype rt = unsafe_yyjson_get_subtype(rhs);
    if (lt == rt)
        return luni->u64 == runi->u64;
    if (lt == YYJSON_SUBTYPE_SINT && rt == YYJSON_SUBTYPE_UINT)
        return luni->i64 >= 0 && luni->u64 == runi->u64;
    if (lt == YYJSON_SUBTYPE_UINT && rt == YYJSON_SUBTYPE_SINT)
        return runi->i64 >= 0 && luni->u64 == runi->u64;
    return false;
}