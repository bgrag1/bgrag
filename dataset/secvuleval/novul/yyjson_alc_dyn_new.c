yyjson_alc *yyjson_alc_dyn_new(void) {
    const yyjson_alc def = YYJSON_DEFAULT_ALC;
    usize hdr_len = sizeof(yyjson_alc) + sizeof(dyn_ctx);
    yyjson_alc *alc = (yyjson_alc *)def.malloc(def.ctx, hdr_len);
    dyn_ctx *ctx = (dyn_ctx *)(void *)(alc + 1);
    if (unlikely(!alc)) return NULL;
    alc->malloc = dyn_malloc;
    alc->realloc = dyn_realloc;
    alc->free = dyn_free;
    alc->ctx = alc + 1;
    memset(ctx, 0, sizeof(*ctx));
    return alc;
}