static esp_err_t process_segment(int index, uint32_t flash_addr, esp_image_segment_header_t *header, bool silent, bool do_load, bootloader_sha256_handle_t sha_handle, uint32_t *checksum, esp_image_metadata_t *metadata)
{
    esp_err_t err;

    /* read segment header */
    err = bootloader_flash_read(flash_addr, header, sizeof(esp_image_segment_header_t), true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bootloader_flash_read failed at 0x%08"PRIx32, flash_addr);
        return err;
    }
    if (sha_handle != NULL) {
        bootloader_sha256_data(sha_handle, header, sizeof(esp_image_segment_header_t));
    }

    intptr_t load_addr = header->load_addr;
    uint32_t data_len = header->data_len;
    uint32_t data_addr = flash_addr + sizeof(esp_image_segment_header_t);

    ESP_LOGV(TAG, "segment data length 0x%"PRIx32" data starts 0x%"PRIx32, data_len, data_addr);

    CHECK_ERR(verify_segment_header(index, header, data_addr, silent));

    if (data_len % 4 != 0) {
        FAIL_LOAD("unaligned segment length 0x%"PRIx32, data_len);
    }

    bool is_mapping = should_map(load_addr);
    do_load = do_load && should_load(load_addr);

    if (!silent) {
        ESP_LOGI(TAG, "segment %d: paddr=%08"PRIx32" vaddr=%08x size=%05"PRIx32"h (%6"PRIu32") %s",
                 index, data_addr, load_addr,
                 data_len, data_len,
                 (do_load) ? "load" : (is_mapping) ? "map" : "");
    }


#ifdef BOOTLOADER_BUILD
    /* Before loading segment, check it doesn't clobber bootloader RAM. */
    if (do_load && data_len > 0) {
        if (!verify_load_addresses(index, load_addr, load_addr + data_len, true, false)) {
            return ESP_ERR_IMAGE_INVALID;
        }
    }
#endif // BOOTLOADER_BUILD

    uint32_t free_page_count = bootloader_mmap_get_free_pages();
    ESP_LOGD(TAG, "free data page_count 0x%08"PRIx32, free_page_count);

    uint32_t data_len_remain = data_len;
    while (data_len_remain > 0) {
#if (SECURE_BOOT_CHECK_SIGNATURE == 1) && defined(BOOTLOADER_BUILD)
        /* Double check the address verification done above */
        ESP_FAULT_ASSERT(!do_load || verify_load_addresses(0, load_addr, load_addr + data_len_remain, false, false));
#endif
        uint32_t offset_page = ((data_addr & MMAP_ALIGNED_MASK) != 0) ? 1 : 0;
        /* Data we could map in case we are not aligned to PAGE boundary is one page size lesser. */
        data_len = MIN(data_len_remain, ((free_page_count - offset_page) * SPI_FLASH_MMU_PAGE_SIZE));
        CHECK_ERR(process_segment_data(index, load_addr, data_addr, data_len, do_load, sha_handle, checksum, metadata));
        data_addr += data_len;
        data_len_remain -= data_len;
    }

    return ESP_OK;

err:
    if (err == ESP_OK) {
        err = ESP_ERR_IMAGE_INVALID;
    }

    return err;
}