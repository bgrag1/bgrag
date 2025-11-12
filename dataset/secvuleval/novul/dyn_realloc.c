static void *dyn_realloc(void *ctx_ptr, void *ptr,
                          usize old_size, usize size) {
    /* assert(ptr != NULL && size != 0 && old_size < size) */
    const yyjson_alc def = YYJSON_DEFAULT_ALC;
    dyn_ctx *ctx = (dyn_ctx *)ctx_ptr;
    dyn_chunk *prev, *next, *new_chunk;
    dyn_chunk *chunk = (dyn_chunk *)ptr - 1;
    if (unlikely(!dyn_size_align(&size))) return NULL;
    if (chunk->size >= size) return ptr;
    
    dyn_chunk_list_remove(&ctx->used_list, chunk);
    new_chunk = (dyn_chunk *)def.realloc(def.ctx, chunk, chunk->size, size);
    if (likely(new_chunk)) {
        new_chunk->size = size;
        chunk = new_chunk;
    }
    dyn_chunk_list_add(&ctx->used_list, chunk);
    return new_chunk ? (void *)(new_chunk + 1) : NULL;
}