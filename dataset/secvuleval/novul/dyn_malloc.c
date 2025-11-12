static void *dyn_malloc(void *ctx_ptr, usize size) {
    /* assert(size != 0) */
    const yyjson_alc def = YYJSON_DEFAULT_ALC;
    dyn_ctx *ctx = (dyn_ctx *)ctx_ptr;
    dyn_chunk *chunk, *prev, *next;
    if (unlikely(!dyn_size_align(&size))) return NULL;
    
    /* freelist is empty, create new chunk */
    if (!ctx->free_list.next) {
        chunk = (dyn_chunk *)def.malloc(def.ctx, size);
        if (unlikely(!chunk)) return NULL;
        chunk->size = size;
        chunk->next = NULL;
        dyn_chunk_list_add(&ctx->used_list, chunk);
        return (void *)(chunk + 1);
    }
    
    /* find a large enough chunk, or resize the largest chunk */
    prev = &ctx->free_list;
    while (true) {
        chunk = prev->next;
        if (chunk->size >= size) { /* enough size, reuse this chunk */
            prev->next = chunk->next;
            dyn_chunk_list_add(&ctx->used_list, chunk);
            return (void *)(chunk + 1);
        }
        if (!chunk->next) { /* resize the largest chunk */
            chunk = (dyn_chunk *)def.realloc(def.ctx, chunk, chunk->size, size);
            if (unlikely(!chunk)) return NULL;
            prev->next = NULL;
            chunk->size = size;
            dyn_chunk_list_add(&ctx->used_list, chunk);
            return (void *)(chunk + 1);
        }
        prev = chunk;
    }
}