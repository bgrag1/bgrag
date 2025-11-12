static esp_err_t process_segments(esp_image_metadata_t *data, bool silent, bool do_load, bootloader_sha256_handle_t sha_handle, uint32_t *checksum)
{
    esp_err_t err = ESP_OK;
    uint32_t start_segments = data->start_addr + data->image_len;
    uint32_t next_addr = start_segments;
    for (int i = 0; i < data->image.segment_count; i++) {
        esp_image_segment_header_t *header = &data->segments[i];
        ESP_LOGV(TAG, "loading segment header %d at offset 0x%"PRIx32, i, next_addr);
        CHECK_ERR(process_segment(i, next_addr, header, silent, do_load, sha_handle, checksum));
        next_addr += sizeof(esp_image_segment_header_t);
        data->segment_data[i] = next_addr;
        next_addr += header->data_len;
    }
    // Segments all loaded, verify length
    uint32_t end_addr = next_addr;
    if (end_addr < data->start_addr) {
        FAIL_LOAD("image offset has wrapped");
    }

    data->image_len += end_addr - start_segments;
    ESP_LOGV(TAG, "image start 0x%08"PRIx32" end of last section 0x%08"PRIx32, data->start_addr, end_addr);
    return err;
err:
    if (err == ESP_OK) {
        err = ESP_ERR_IMAGE_INVALID;
    }
    return err;
}