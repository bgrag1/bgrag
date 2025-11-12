yyjson_mut_val *yyjson_mut_merge_patch(yyjson_mut_doc *doc,
                                       yyjson_mut_val *orig,
                                       yyjson_mut_val *patch) {
    usize idx, max;
    yyjson_mut_val *key, *orig_val, *patch_val, local_orig;
    yyjson_mut_val *builder, *mut_key, *mut_val, *merged_val;
    
    if (unlikely(!yyjson_mut_is_obj(patch))) {
        return yyjson_mut_val_mut_copy(doc, patch);
    }
    
    builder = yyjson_mut_obj(doc);
    if (unlikely(!builder)) return NULL;
    
    memset(&local_orig, 0, sizeof(local_orig));
    if (!yyjson_mut_is_obj(orig)) {
        orig = &local_orig;
        orig->tag = builder->tag;
        orig->uni = builder->uni;
    }
    
    /* If orig is contributing, copy any items not modified by the patch */
    if (orig != &local_orig) {
        yyjson_mut_obj_foreach(orig, idx, max, key, orig_val) {
            patch_val = yyjson_mut_obj_getn(patch,
                                            unsafe_yyjson_get_str(key),
                                            unsafe_yyjson_get_len(key));
            if (!patch_val) {
                mut_key = yyjson_mut_val_mut_copy(doc, key);
                mut_val = yyjson_mut_val_mut_copy(doc, orig_val);
                if (!yyjson_mut_obj_add(builder, mut_key, mut_val)) return NULL;
            }
        }
    }

    /* Merge items modified by the patch. */
    yyjson_mut_obj_foreach(patch, idx, max, key, patch_val) {
        /* null indicates the field is removed. */
        if (unsafe_yyjson_is_null(patch_val)) {
            continue;
        }
        mut_key = yyjson_mut_val_mut_copy(doc, key);
        orig_val = yyjson_mut_obj_getn(orig,
                                       unsafe_yyjson_get_str(key),
                                       unsafe_yyjson_get_len(key));
        merged_val = yyjson_mut_merge_patch(doc, orig_val, patch_val);
        if (!yyjson_mut_obj_add(builder, mut_key, merged_val)) return NULL;
    }
    
    return builder;
}