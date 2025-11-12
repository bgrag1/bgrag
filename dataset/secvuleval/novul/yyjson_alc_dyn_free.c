void yyjson_alc_dyn_free(yyjson_alc *alc) {
    const yyjson_alc def = YYJSON_DEFAULT_ALC;
    dyn_ctx *ctx = (dyn_ctx *)(void *)(alc + 1);
    dyn_chunk *chunk, *next;
    if (unlikely(!alc)) return;
    for (chunk = ctx->free_list.next; chunk; chunk = next) {
        next = chunk->next;
        def.free(def.ctx, chunk);
    }
    for (chunk = ctx->used_list.next; chunk; chunk = next) {
        next = chunk->next;
        def.free(def.ctx, chunk);
    }
    def.free(def.ctx, alc);
}