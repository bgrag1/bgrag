static esp_err_t process_segment_data(int segment, intptr_t load_addr, uint32_t data_addr, uint32_t data_len, bool do_load, bootloader_sha256_handle_t sha_handle, uint32_t *checksum, esp_image_metadata_t *metadata)
{
    // If we are not loading, and the checksum is empty, skip processing this
    // segment for data
    if (!do_load && checksum == NULL) {
        ESP_LOGD(TAG, "skipping checksum for segment");
        return ESP_OK;
    }

    const uint32_t *data = (const uint32_t *)bootloader_mmap(data_addr, data_len);
    if (!data) {
        ESP_LOGE(TAG, "bootloader_mmap(0x%"PRIx32", 0x%"PRIx32") failed",
                 data_addr, data_len);
        return ESP_FAIL;
    }

    if (checksum == NULL && sha_handle == NULL) {
        memcpy((void *)load_addr, data, data_len);
        bootloader_munmap(data);
        return ESP_OK;
    }

#ifdef BOOTLOADER_BUILD
    // Set up the obfuscation value to use for loading
    while (ram_obfs_value[0] == 0 || ram_obfs_value[1] == 0) {
        bootloader_fill_random(ram_obfs_value, sizeof(ram_obfs_value));
#if CONFIG_IDF_ENV_FPGA
        /* FPGA doesn't always emulate the RNG */
        ram_obfs_value[0] ^= 0x33;
        ram_obfs_value[1] ^= 0x66;
#endif
    }
    uint32_t *dest = (uint32_t *)load_addr;
#endif // BOOTLOADER_BUILD

    const uint32_t *src = data;

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    // Case I: Bootloader verifying application
    // Case II: Bootloader verifying bootloader
    // Anti-rollback check should handle only Case I from above.
    if (segment == 0 && metadata->start_addr != ESP_BOOTLOADER_OFFSET) {
        ESP_LOGD(TAG, "additional anti-rollback check 0x%"PRIx32, data_addr);
        // The esp_app_desc_t structure is located in DROM and is always in segment #0.
        size_t len = process_esp_app_desc_data(src, sha_handle, checksum, metadata);
        data_len -= len;
        src += len / 4;
        // In BOOTLOADER_BUILD, for DROM (segment #0) we do not load it into dest (only map it), do_load = false.
    }
#endif // CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK

    for (size_t i = 0; i < data_len; i += 4) {
        int w_i = i / 4; // Word index
        uint32_t w = src[w_i];
        if (checksum != NULL) {
            *checksum ^= w;
        }
#ifdef BOOTLOADER_BUILD
        if (do_load) {
            dest[w_i] = w ^ ((w_i & 1) ? ram_obfs_value[0] : ram_obfs_value[1]);
        }
#endif
        // SHA_CHUNK determined experimentally as the optimum size
        // to call bootloader_sha256_data() with. This is a bit
        // counter-intuitive, but it's ~3ms better than using the
        // SHA256 block size.
        const size_t SHA_CHUNK = 1024;
        if (sha_handle != NULL && i % SHA_CHUNK == 0) {
            bootloader_sha256_data(sha_handle, &src[w_i],
                                   MIN(SHA_CHUNK, data_len - i));
        }
    }
#if SOC_CACHE_INTERNAL_MEM_VIA_L1CACHE
    if (do_load && esp_ptr_in_diram_iram((uint32_t *)load_addr)) {
        cache_ll_writeback_all(CACHE_LL_LEVEL_INT_MEM, CACHE_TYPE_DATA, CACHE_LL_ID_ALL);
    }
#endif

    bootloader_munmap(data);

    return ESP_OK;
}