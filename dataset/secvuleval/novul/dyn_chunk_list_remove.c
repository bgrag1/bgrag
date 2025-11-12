static_inline void dyn_chunk_list_remove(dyn_chunk *list, dyn_chunk *chunk) {
    dyn_chunk *prev = list, *cur;
    for (cur = prev->next; cur; cur = cur->next) {
        if (cur == chunk) {
            prev->next = cur->next;
            cur->next = NULL;
            return;
        }
        prev = cur;
    }
}