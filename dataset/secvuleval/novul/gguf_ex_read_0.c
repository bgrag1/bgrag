static bool gguf_ex_read_0(const std::string & fname) {
    struct gguf_init_params params = {
        /*.no_alloc = */ false,
        /*.ctx      = */ NULL,
    };

    struct gguf_context * ctx = gguf_init_from_file(fname.c_str(), params);

    if (!ctx) {
        fprintf(stderr, "%s: failed to load '%s'\n", __func__, fname.c_str());
        return false;
    }

    printf("%s: version:      %d\n", __func__, gguf_get_version(ctx));
    printf("%s: alignment:   %zu\n", __func__, gguf_get_alignment(ctx));
    printf("%s: data offset: %zu\n", __func__, gguf_get_data_offset(ctx));

    // kv
    {
        const int n_kv = gguf_get_n_kv(ctx);

        printf("%s: n_kv: %d\n", __func__, n_kv);

        for (int i = 0; i < n_kv; ++i) {
            const char * key = gguf_get_key(ctx, i);

            printf("%s: kv[%d]: key = %s\n", __func__, i, key);
        }
    }

    // find kv string
    {
        const char * findkey = "some.parameter.string";

        const int keyidx = gguf_find_key(ctx, findkey);
        if (keyidx == -1) {
            printf("%s: find key: %s not found.\n", __func__, findkey);
        } else {
            const char * key_value = gguf_get_val_str(ctx, keyidx);
            printf("%s: find key: %s found, kv[%d] value = %s\n", __func__, findkey, keyidx, key_value);
        }
    }

    // tensor info
    {
        const int n_tensors = gguf_get_n_tensors(ctx);

        printf("%s: n_tensors: %d\n", __func__, n_tensors);

        for (int i = 0; i < n_tensors; ++i) {
            const char * name   = gguf_get_tensor_name  (ctx, i);
            const size_t offset = gguf_get_tensor_offset(ctx, i);

            printf("%s: tensor[%d]: name = %s, offset = %zu\n", __func__, i, name, offset);
        }
    }

    gguf_free(ctx);

    return true;
}