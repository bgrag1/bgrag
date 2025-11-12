static void dyn_free(void *ctx_ptr, void *ptr) {
    /* assert(ptr != NULL) */
    dyn_ctx *ctx = (dyn_ctx *)ctx_ptr;
    dyn_chunk *chunk = (dyn_chunk *)ptr - 1, *prev;
    
    dyn_chunk_list_remove(&ctx->used_list, chunk);
    for (prev = &ctx->free_list; prev; prev = prev->next) {
        if (!prev->next || prev->next->size >= chunk->size) {
            chunk->next = prev->next;
            prev->next = chunk;
            break;
        }
    }
}