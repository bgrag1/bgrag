static_inline void dyn_chunk_list_add(dyn_chunk *list, dyn_chunk *chunk) {
    chunk->next = list->next;
    list->next = chunk;
}