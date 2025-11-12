static_inline bool dyn_size_align(usize *size) {
    usize alc_size = *size + sizeof(dyn_chunk);
    alc_size = size_align_up(alc_size, YYJSON_ALC_DYN_MIN_SIZE);
    if (unlikely(alc_size < *size)) return false; /* overflow */
    *size = alc_size;
    return true;
}