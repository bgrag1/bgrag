static void *pool_malloc(void *ctx_ptr, usize size) {
    pool_ctx *ctx = (pool_ctx *)ctx_ptr;
    pool_chunk *next, *prev = NULL, *cur = ctx->free_list;
    
    if (unlikely(size == 0 || size >= ctx->size)) return NULL;
    size = size_align_up(size, sizeof(pool_chunk)) + sizeof(pool_chunk);
    
    while (cur) {
        if (cur->size < size) {
            /* not enough space, try next chunk */
            prev = cur;
            cur = cur->next;
            continue;
        }
        if (cur->size >= size + sizeof(pool_chunk) * 2) {
            /* too much space, split this chunk */
            next = (pool_chunk *)(void *)((u8 *)cur + size);
            next->size = cur->size - size;
            next->next = cur->next;
            cur->size = size;
        } else {
            /* just enough space, use whole chunk */
            next = cur->next;
        }
        if (prev) prev->next = next;
        else ctx->free_list = next;
        return (void *)(cur + 1);
    }
    return NULL;
}