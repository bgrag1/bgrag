static void *pool_realloc(void *ctx_ptr, void *ptr,
                          usize old_size, usize size) {
    /* assert(ptr != NULL && size != 0 && old_size < size) */
    pool_ctx *ctx = (pool_ctx *)ctx_ptr;
    pool_chunk *cur = ((pool_chunk *)ptr) - 1, *prev, *next, *tmp;
    
    /* check size */
    if (unlikely(size >= ctx->size)) return NULL;
    pool_size_align(&old_size);
    pool_size_align(&size);
    if (unlikely(old_size == size)) return ptr;
    
    /* find next and prev chunk */
    prev = NULL;
    next = ctx->free_list;
    while (next && next < cur) {
        prev = next;
        next = next->next;
    }
    
    if ((u8 *)cur + cur->size == (u8 *)next && cur->size + next->size >= size) {
        /* merge to higher chunk if they are contiguous */
        usize free_size = cur->size + next->size - size;
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
    } else {
        /* fallback to malloc and memcpy */
        void *new_ptr = pool_malloc(ctx_ptr, size - sizeof(pool_chunk));
        if (new_ptr) {
            memcpy(new_ptr, ptr, cur->size - sizeof(pool_chunk));
            pool_free(ctx_ptr, ptr);
        }
        return new_ptr;
    }
}