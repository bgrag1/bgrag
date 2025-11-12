static_inline void pool_size_align(usize *size) {
    *size = size_align_up(*size, sizeof(pool_chunk)) + sizeof(pool_chunk);
}