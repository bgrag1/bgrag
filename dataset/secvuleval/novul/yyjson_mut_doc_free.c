void yyjson_mut_doc_free(yyjson_mut_doc *doc) {
    if (doc) {
        yyjson_alc alc = doc->alc;
        memset(&doc->alc, 0, sizeof(alc));
        unsafe_yyjson_str_pool_release(&doc->str_pool, &alc);
        unsafe_yyjson_val_pool_release(&doc->val_pool, &alc);
        alc.free(alc.ctx, doc);
    }
}