static void *pool_realloc(void *ctx_ptr, void *ptr,
                          usize old_size, usize size) {
    pool_ctx *ctx = (pool_ctx *)ctx_ptr;
    pool_chunk *cur = ((pool_chunk *)ptr) - 1, *prev, *next, *tmp;
    usize free_size;
    void *new_ptr;
    
    if (unlikely(size == 0 || size >= ctx->size)) return NULL;
    size = size_align_up(size, sizeof(pool_chunk)) + sizeof(pool_chunk);
    
    /* reduce size */
    if (unlikely(size <= cur->size)) {
        free_size = cur->size - size;
        if (free_size >= sizeof(pool_chunk) * 2) {
            tmp = (pool_chunk *)(void *)((u8 *)cur + cur->size - free_size);
            tmp->size = free_size;
            pool_free(ctx_ptr, (void *)(tmp + 1));
            cur->size -= free_size;
        }
        return ptr;
    }
    
    /* find next and prev chunk */
    prev = NULL;
    next = ctx->free_list;
    while (next && next < cur) {
        prev = next;
        next = next->next;
    }
    
    /* merge to higher chunk if they are contiguous */
    if ((u8 *)cur + cur->size == (u8 *)next &&
        cur->size + next->size >= size) {
        free_size = cur->size + next->size - size;
        if (free_size > sizeof(pool_chunk) * 2) {
            tmp = (pool_chunk *)(void *)((u8 *)cur + size);
            if (prev) prev->next = tmp;
            else ctx->free_list = tmp;
            tmp->next = next->next;
            tmp->size = free_size;
            cur->size = size;
        } else {
            if (prev) prev->next = next->next;
            else ctx->free_list = next->next;
            cur->size += next->size;
        }
        return ptr;
    }
    
    /* fallback to malloc and memcpy */
    new_ptr = pool_malloc(ctx_ptr, size - sizeof(pool_chunk));
    if (new_ptr) {
        memcpy(new_ptr, ptr, cur->size - sizeof(pool_chunk));
        pool_free(ctx_ptr, ptr);
    }
    return new_ptr;
}