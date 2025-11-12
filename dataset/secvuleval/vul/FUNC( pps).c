static int FUNC(pps) (CodedBitstreamContext *ctx, RWContext *rw,
                      H266RawPPS *current)
{
    CodedBitstreamH266Context *h266 = ctx->priv_data;
    const H266RawSPS *sps;
    int err, i;
    unsigned int min_cb_size_y, divisor, ctb_size_y,
        pic_width_in_ctbs_y, pic_height_in_ctbs_y;
    uint8_t sub_width_c, sub_height_c, qp_bd_offset;

    static const uint8_t h266_sub_width_c[] = {
        1, 2, 2, 1
    };
    static const uint8_t h266_sub_height_c[] = {
        1, 2, 1, 1
    };

    HEADER("Picture Parameter Set");

    CHECK(FUNC(nal_unit_header) (ctx, rw,
                                 &current->nal_unit_header, VVC_PPS_NUT));

    ub(6, pps_pic_parameter_set_id);
    ub(4, pps_seq_parameter_set_id);
    sps = h266->sps[current->pps_seq_parameter_set_id];
    if (!sps) {
        av_log(ctx->log_ctx, AV_LOG_ERROR, "SPS id %d not available.\n",
               current->pps_seq_parameter_set_id);
        return AVERROR_INVALIDDATA;
    }

    flag(pps_mixed_nalu_types_in_pic_flag);
    ue(pps_pic_width_in_luma_samples,
       1, sps->sps_pic_width_max_in_luma_samples);
    ue(pps_pic_height_in_luma_samples,
       1, sps->sps_pic_height_max_in_luma_samples);

    min_cb_size_y = 1 << (sps->sps_log2_min_luma_coding_block_size_minus2 + 2);
    divisor = FFMAX(min_cb_size_y, 8);
    if (current->pps_pic_width_in_luma_samples % divisor ||
        current->pps_pic_height_in_luma_samples % divisor) {
        av_log(ctx->log_ctx, AV_LOG_ERROR,
               "Invalid dimensions: %ux%u not divisible "
               "by %u, MinCbSizeY = %u.\n",
               current->pps_pic_width_in_luma_samples,
               current->pps_pic_height_in_luma_samples, divisor, min_cb_size_y);
        return AVERROR_INVALIDDATA;
    }
    if (!sps->sps_res_change_in_clvs_allowed_flag &&
        (current->pps_pic_width_in_luma_samples !=
         sps->sps_pic_width_max_in_luma_samples ||
         current->pps_pic_height_in_luma_samples !=
         sps->sps_pic_height_max_in_luma_samples)) {
        av_log(ctx->log_ctx, AV_LOG_ERROR,
               "Resoltuion change is not allowed, "
               "in max resolution (%ux%u) mismatched with pps(%ux%u).\n",
               sps->sps_pic_width_max_in_luma_samples,
               sps->sps_pic_height_max_in_luma_samples,
               current->pps_pic_width_in_luma_samples,
               current->pps_pic_height_in_luma_samples);
        return AVERROR_INVALIDDATA;
    }

    ctb_size_y = 1 << (sps->sps_log2_ctu_size_minus5 + 5);
    if (sps->sps_ref_wraparound_enabled_flag) {
        if ((ctb_size_y / min_cb_size_y + 1) >
            (current->pps_pic_width_in_luma_samples / min_cb_size_y - 1)) {
            av_log(ctx->log_ctx, AV_LOG_ERROR,
                   "Invalid width(%u), ctb_size_y = %u, min_cb_size_y = %u.\n",
                   current->pps_pic_width_in_luma_samples,
                   ctb_size_y, min_cb_size_y);
            return AVERROR_INVALIDDATA;
        }
    }

    flag(pps_conformance_window_flag);
    if (current->pps_pic_width_in_luma_samples ==
        sps->sps_pic_width_max_in_luma_samples &&
        current->pps_pic_height_in_luma_samples ==
        sps->sps_pic_height_max_in_luma_samples &&
        current->pps_conformance_window_flag) {
        av_log(ctx->log_ctx, AV_LOG_ERROR,
               "Conformance window flag should not true.\n");
        return AVERROR_INVALIDDATA;
    }

    sub_width_c = h266_sub_width_c[sps->sps_chroma_format_idc];
    sub_height_c = h266_sub_height_c[sps->sps_chroma_format_idc];
    if (current->pps_conformance_window_flag) {
        ue(pps_conf_win_left_offset, 0, current->pps_pic_width_in_luma_samples);
        ue(pps_conf_win_right_offset,
           0, current->pps_pic_width_in_luma_samples);
        ue(pps_conf_win_top_offset, 0, current->pps_pic_height_in_luma_samples);
        ue(pps_conf_win_bottom_offset,
           0, current->pps_pic_height_in_luma_samples);
        if (sub_width_c *
            (current->pps_conf_win_left_offset +
             current->pps_conf_win_right_offset) >=
            current->pps_pic_width_in_luma_samples ||
            sub_height_c *
            (current->pps_conf_win_top_offset +
             current->pps_conf_win_bottom_offset) >=
            current->pps_pic_height_in_luma_samples) {
            av_log(ctx->log_ctx, AV_LOG_ERROR,
                   "Invalid pps conformance window: (%u, %u, %u, %u), "
                   "resolution is %ux%u, sub wxh is %ux%u.\n",
                   current->pps_conf_win_left_offset,
                   current->pps_conf_win_right_offset,
                   current->pps_conf_win_top_offset,
                   current->pps_conf_win_bottom_offset,
                   current->pps_pic_width_in_luma_samples,
                   current->pps_pic_height_in_luma_samples,
                   sub_width_c, sub_height_c);
            return AVERROR_INVALIDDATA;
        }
    } else {
        if (current->pps_pic_width_in_luma_samples ==
            sps->sps_pic_width_max_in_luma_samples &&
            current->pps_pic_height_in_luma_samples ==
            sps->sps_pic_height_max_in_luma_samples) {
            infer(pps_conf_win_left_offset, sps->sps_conf_win_left_offset);
            infer(pps_conf_win_right_offset, sps->sps_conf_win_right_offset);
            infer(pps_conf_win_top_offset, sps->sps_conf_win_top_offset);
            infer(pps_conf_win_bottom_offset, sps->sps_conf_win_bottom_offset);
        } else {
            infer(pps_conf_win_left_offset, 0);
            infer(pps_conf_win_right_offset, 0);
            infer(pps_conf_win_top_offset, 0);
            infer(pps_conf_win_bottom_offset, 0);
        }

    }

    flag(pps_scaling_window_explicit_signalling_flag);
    if (!sps->sps_ref_pic_resampling_enabled_flag &&
        current->pps_scaling_window_explicit_signalling_flag) {
        av_log(ctx->log_ctx, AV_LOG_ERROR,
               "Invalid data: sps_ref_pic_resampling_enabled_flag is false, "
               "but pps_scaling_window_explicit_signalling_flag is true.\n");
        return AVERROR_INVALIDDATA;
    }
    if (current->pps_scaling_window_explicit_signalling_flag) {
        se(pps_scaling_win_left_offset,
           -current->pps_pic_width_in_luma_samples * 15 / sub_width_c,
           current->pps_pic_width_in_luma_samples / sub_width_c);
        se(pps_scaling_win_right_offset,
           -current->pps_pic_width_in_luma_samples * 15 / sub_width_c,
           current->pps_pic_width_in_luma_samples / sub_width_c);
        se(pps_scaling_win_top_offset,
           -current->pps_pic_height_in_luma_samples * 15 / sub_height_c,
           current->pps_pic_height_in_luma_samples / sub_height_c);
        se(pps_scaling_win_bottom_offset,
           -current->pps_pic_height_in_luma_samples * 15 / sub_height_c,
           current->pps_pic_height_in_luma_samples / sub_height_c);
    } else {
        infer(pps_scaling_win_left_offset, current->pps_conf_win_left_offset);
        infer(pps_scaling_win_right_offset, current->pps_conf_win_right_offset);
        infer(pps_scaling_win_top_offset, current->pps_conf_win_top_offset);
        infer(pps_scaling_win_bottom_offset, current->pps_conf_win_bottom_offset);
    }

    flag(pps_output_flag_present_flag);
    flag(pps_no_pic_partition_flag);
    flag(pps_subpic_id_mapping_present_flag);

    if (current->pps_subpic_id_mapping_present_flag) {
        if (!current->pps_no_pic_partition_flag) {
            ue(pps_num_subpics_minus1,
               sps->sps_num_subpics_minus1, sps->sps_num_subpics_minus1);
        } else {
            infer(pps_num_subpics_minus1, 0);
        }
        ue(pps_subpic_id_len_minus1, sps->sps_subpic_id_len_minus1,
           sps->sps_subpic_id_len_minus1);
        for (i = 0; i <= current->pps_num_subpics_minus1; i++) {
            ubs(sps->sps_subpic_id_len_minus1 + 1, pps_subpic_id[i], 1, i);
        }
    }

    for (i = 0; i <= sps->sps_num_subpics_minus1; i++) {
        if (sps->sps_subpic_id_mapping_explicitly_signalled_flag)
            current->sub_pic_id_val[i] = current->pps_subpic_id_mapping_present_flag
                                       ? current->pps_subpic_id[i]
                                       : sps->sps_subpic_id[i];
        else
            current->sub_pic_id_val[i] = i;
    }

    pic_width_in_ctbs_y = AV_CEIL_RSHIFT
        (current->pps_pic_width_in_luma_samples, (sps->sps_log2_ctu_size_minus5 + 5));
    pic_height_in_ctbs_y = AV_CEIL_RSHIFT(
        current->pps_pic_height_in_luma_samples,(sps->sps_log2_ctu_size_minus5 + 5));
    if (!current->pps_no_pic_partition_flag) {
        unsigned int exp_tile_width = 0, exp_tile_height = 0;
        unsigned int unified_size, remaining_size;

        u(2, pps_log2_ctu_size_minus5,
          sps->sps_log2_ctu_size_minus5, sps->sps_log2_ctu_size_minus5);
        ue(pps_num_exp_tile_columns_minus1,
           0, FFMIN(pic_width_in_ctbs_y - 1, VVC_MAX_TILE_COLUMNS - 1));
        ue(pps_num_exp_tile_rows_minus1,
           0, FFMIN(pic_height_in_ctbs_y - 1, VVC_MAX_TILE_ROWS - 1));

        for (i = 0; i <= current->pps_num_exp_tile_columns_minus1; i++) {
            ues(pps_tile_column_width_minus1[i],
                0, pic_width_in_ctbs_y - exp_tile_width - 1, 1, i);
            exp_tile_width += current->pps_tile_column_width_minus1[i] + 1;
        }
        for (i = 0; i <= current->pps_num_exp_tile_rows_minus1; i++) {
            ues(pps_tile_row_height_minus1[i],
                0, pic_height_in_ctbs_y - exp_tile_height - 1, 1, i);
            exp_tile_height += current->pps_tile_row_height_minus1[i] + 1;
        }

        remaining_size = pic_width_in_ctbs_y;
        for (i = 0; i <= current->pps_num_exp_tile_columns_minus1; i++) {
          if (current->pps_tile_column_width_minus1[i] >= remaining_size) {
              av_log(ctx->log_ctx, AV_LOG_ERROR,
                     "Tile column width(%d) exceeds picture width\n",i);
              return AVERROR_INVALIDDATA;
          }
          current->col_width_val[i] = current->pps_tile_column_width_minus1[i] + 1;
          remaining_size -= (current->pps_tile_column_width_minus1[i] + 1);
        }
        unified_size = current->pps_tile_column_width_minus1[i - 1] + 1;
        while (remaining_size > 0) {
            if (current->num_tile_columns > VVC_MAX_TILE_COLUMNS) {
                av_log(ctx->log_ctx, AV_LOG_ERROR,
                       "NumTileColumns(%d) > than VVC_MAX_TILE_COLUMNS(%d)\n",
                       current->num_tile_columns, VVC_MAX_TILE_COLUMNS);
                return AVERROR_INVALIDDATA;
            }
            unified_size = FFMIN(remaining_size, unified_size);
            current->col_width_val[i] = unified_size;
            remaining_size -= unified_size;
            i++;
        }
        current->num_tile_columns = i;
        if (current->num_tile_columns > VVC_MAX_TILE_COLUMNS) {
            av_log(ctx->log_ctx, AV_LOG_ERROR,
                   "NumTileColumns(%d) > than VVC_MAX_TILE_COLUMNS(%d)\n",
                   current->num_tile_columns, VVC_MAX_TILE_COLUMNS);
            return AVERROR_INVALIDDATA;
        }

        remaining_size = pic_height_in_ctbs_y;
        for (i = 0; i <= current->pps_num_exp_tile_rows_minus1; i++) {
          if (current->pps_tile_row_height_minus1[i] >= remaining_size) {
              av_log(ctx->log_ctx, AV_LOG_ERROR,
                     "Tile row height(%d) exceeds picture height\n",i);
              return AVERROR_INVALIDDATA;
          }
          current->row_height_val[i] = current->pps_tile_row_height_minus1[i] + 1;
          remaining_size -= (current->pps_tile_row_height_minus1[i] + 1);
        }
        unified_size = current->pps_tile_row_height_minus1[i - 1] + 1;

        while (remaining_size > 0) {
            unified_size = FFMIN(remaining_size, unified_size);
            current->row_height_val[i] = unified_size;
            remaining_size -= unified_size;
            i++;
        }
        current->num_tile_rows=i;
        if (current->num_tile_rows > VVC_MAX_TILE_ROWS) {
            av_log(ctx->log_ctx, AV_LOG_ERROR,
                   "NumTileRows(%d) > than VVC_MAX_TILE_ROWS(%d)\n",
                   current->num_tile_rows, VVC_MAX_TILE_ROWS);
            return AVERROR_INVALIDDATA;
        }

        current->num_tiles_in_pic = current->num_tile_columns *
                                    current->num_tile_rows;
        if (current->num_tiles_in_pic > VVC_MAX_TILES_PER_AU) {
            av_log(ctx->log_ctx, AV_LOG_ERROR,
                   "NumTilesInPic(%d) > than VVC_MAX_TILES_PER_AU(%d)\n",
                   current->num_tiles_in_pic, VVC_MAX_TILES_PER_AU);
            return AVERROR_INVALIDDATA;
        }

        if (current->num_tiles_in_pic > 1) {
            flag(pps_loop_filter_across_tiles_enabled_flag);
            flag(pps_rect_slice_flag);
        } else {
            infer(pps_loop_filter_across_tiles_enabled_flag, 0);
            infer(pps_rect_slice_flag, 1);
        }
        if (current->pps_rect_slice_flag)
            flag(pps_single_slice_per_subpic_flag);
        else
            infer(pps_single_slice_per_subpic_flag, 1);
        if (current->pps_rect_slice_flag &&
            !current->pps_single_slice_per_subpic_flag) {
            int j;
            uint16_t tile_idx = 0, tile_x, tile_y, ctu_x, ctu_y;
            uint16_t slice_top_left_ctu_x[VVC_MAX_SLICES];
            uint16_t slice_top_left_ctu_y[VVC_MAX_SLICES];
            ue(pps_num_slices_in_pic_minus1, 0, VVC_MAX_SLICES - 1);
            if (current->pps_num_slices_in_pic_minus1 > 1)
                flag(pps_tile_idx_delta_present_flag);
            else
                infer(pps_tile_idx_delta_present_flag, 0);
            for (i = 0; i < current->pps_num_slices_in_pic_minus1; i++) {
                tile_x = tile_idx % current->num_tile_columns;
                tile_y = tile_idx / current->num_tile_columns;
                if (tile_x != current->num_tile_columns - 1) {
                    ues(pps_slice_width_in_tiles_minus1[i],
                        0, current->num_tile_columns - 1, 1, i);
                } else {
                    infer(pps_slice_width_in_tiles_minus1[i], 0);
                }
                if (tile_y != current->num_tile_rows - 1 &&
                    (current->pps_tile_idx_delta_present_flag || tile_x == 0)) {
                    ues(pps_slice_height_in_tiles_minus1[i],
                        0, current->num_tile_rows - 1, 1, i);
                } else {
                    if (tile_y == current->num_tile_rows - 1)
                        infer(pps_slice_height_in_tiles_minus1[i], 0);
                    else
                        infer(pps_slice_height_in_tiles_minus1[i],
                              current->pps_slice_height_in_tiles_minus1[i - 1]);
                }

                ctu_x = ctu_y = 0;
                for (j = 0; j < tile_x; j++) {
                    ctu_x += current->col_width_val[j];
                }
                for (j = 0; j < tile_y; j++) {
                    ctu_y += current->row_height_val[j];
                }
                if (current->pps_slice_width_in_tiles_minus1[i] == 0 &&
                    current->pps_slice_height_in_tiles_minus1[i] == 0 &&
                    current->row_height_val[tile_y] > 1) {
                    int num_slices_in_tile,
                        uniform_slice_height, remaining_height_in_ctbs_y;
                    remaining_height_in_ctbs_y =
                        current->row_height_val[tile_y];
                    ues(pps_num_exp_slices_in_tile[i],
                        0, current->row_height_val[tile_y] - 1, 1, i);
                    if (current->pps_num_exp_slices_in_tile[i] == 0) {
                        num_slices_in_tile = 1;
                        current->slice_height_in_ctus[i] = current->row_height_val[tile_y];
                        slice_top_left_ctu_x[i] = ctu_x;
                        slice_top_left_ctu_y[i] = ctu_y;
                    } else {
                        uint16_t slice_height_in_ctus;
                        for (j = 0; j < current->pps_num_exp_slices_in_tile[i];
                             j++) {
                            ues(pps_exp_slice_height_in_ctus_minus1[i][j], 0,
                                current->row_height_val[tile_y] - 1, 2,
                                i, j);
                            slice_height_in_ctus =
                                current->
                                pps_exp_slice_height_in_ctus_minus1[i][j] + 1;

                            current->slice_height_in_ctus[i + j] =
                                slice_height_in_ctus;
                            slice_top_left_ctu_x[i + j] = ctu_x;
                            slice_top_left_ctu_y[i + j] = ctu_y;
                            ctu_y += slice_height_in_ctus;

                            remaining_height_in_ctbs_y -= slice_height_in_ctus;
                        }
                        uniform_slice_height = 1 +
                            (j == 0 ? current->row_height_val[tile_y] - 1:
                            current->pps_exp_slice_height_in_ctus_minus1[i][j-1]);
                        while (remaining_height_in_ctbs_y > uniform_slice_height) {
                            current->slice_height_in_ctus[i + j] =
                                                          uniform_slice_height;
                            slice_top_left_ctu_x[i + j] = ctu_x;
                            slice_top_left_ctu_y[i + j] = ctu_y;
                            ctu_y += uniform_slice_height;

                            remaining_height_in_ctbs_y -= uniform_slice_height;
                            j++;
                        }
                        if (remaining_height_in_ctbs_y > 0) {
                            current->slice_height_in_ctus[i + j] =
                                remaining_height_in_ctbs_y;
                            slice_top_left_ctu_x[i + j] = ctu_x;
                            slice_top_left_ctu_y[i + j] = ctu_y;
                            j++;
                        }
                        num_slices_in_tile = j;
                    }
                    i += num_slices_in_tile - 1;
                } else {
                    uint16_t height = 0;
                    infer(pps_num_exp_slices_in_tile[i], 0);
                    for (j = 0;
                         j <= current->pps_slice_height_in_tiles_minus1[i];
                         j++) {
                        height +=
                           current->row_height_val[tile_y + j];
                    }
                    current->slice_height_in_ctus[i] = height;

                    slice_top_left_ctu_x[i] = ctu_x;
                    slice_top_left_ctu_y[i] = ctu_y;
                }
                if (i < current->pps_num_slices_in_pic_minus1) {
                    if (current->pps_tile_idx_delta_present_flag) {
                        ses(pps_tile_idx_delta_val[i],
                            -current->num_tiles_in_pic + 1,
                            current->num_tiles_in_pic - 1, 1, i);
                        if (current->pps_tile_idx_delta_val[i] == 0) {
                            av_log(ctx->log_ctx, AV_LOG_ERROR,
                                   "pps_tile_idx_delta_val[i] shall not be equal to 0.\n");
                        }
                        tile_idx += current->pps_tile_idx_delta_val[i];
                    } else {
                        infer(pps_tile_idx_delta_val[i], 0);
                        tile_idx +=
                            current->pps_slice_width_in_tiles_minus1[i] + 1;
                        if (tile_idx % current->num_tile_columns == 0) {
                            tile_idx +=
                                current->pps_slice_height_in_tiles_minus1[i] *
                                current->num_tile_columns;
                        }
                    }
                }
            }
            if (i == current->pps_num_slices_in_pic_minus1) {
                uint16_t height = 0;

                tile_x = tile_idx % current->num_tile_columns;
                tile_y = tile_idx / current->num_tile_columns;

                ctu_x = 0, ctu_y = 0;
                for (j = 0; j < tile_x; j++) {
                    ctu_x += current->col_width_val[j];
                }
                for (j = 0; j < tile_y; j++) {
                    ctu_y += current->row_height_val[j];
                }
                slice_top_left_ctu_x[i] = ctu_x;
                slice_top_left_ctu_y[i] = ctu_y;

                current->pps_slice_width_in_tiles_minus1[i] =
                    current->num_tile_columns - tile_x - 1;
                current->pps_slice_height_in_tiles_minus1[i] =
                    current->num_tile_rows - tile_y - 1;

                for (j = 0; j <= current->pps_slice_height_in_tiles_minus1[i];
                     j++) {
                    height +=
                        current->row_height_val[tile_y + j];
                }
                current->slice_height_in_ctus[i] = height;

                infer(pps_num_exp_slices_in_tile[i], 0);
            }
            //now, we got all slice information, let's resolve NumSlicesInSubpic
            for (i = 0; i <= sps->sps_num_subpics_minus1; i++) {
                current->num_slices_in_subpic[i] = 0;
                for (j = 0; j <= current->pps_num_slices_in_pic_minus1; j++) {
                    uint16_t pos_x = 0, pos_y = 0;
                    pos_x = slice_top_left_ctu_x[j];
                    pos_y = slice_top_left_ctu_y[j];
                    if ((pos_x >= sps->sps_subpic_ctu_top_left_x[i]) &&
                        (pos_x <
                         sps->sps_subpic_ctu_top_left_x[i] +
                         sps->sps_subpic_width_minus1[i] + 1) &&
                         (pos_y >= sps->sps_subpic_ctu_top_left_y[i]) &&
                         (pos_y < sps->sps_subpic_ctu_top_left_y[i] +
                            sps->sps_subpic_height_minus1[i] + 1)) {
                        current->num_slices_in_subpic[i]++;
                    }
                }
            }
        } else {
            if (current->pps_no_pic_partition_flag)
                infer(pps_num_slices_in_pic_minus1, 0);
            else if (current->pps_single_slice_per_subpic_flag)
                infer(pps_num_slices_in_pic_minus1,
                      sps->sps_num_subpics_minus1);
            // else?
        }
        if (!current->pps_rect_slice_flag ||
            current->pps_single_slice_per_subpic_flag ||
            current->pps_num_slices_in_pic_minus1 > 0)
            flag(pps_loop_filter_across_slices_enabled_flag);
        else
            infer(pps_loop_filter_across_slices_enabled_flag, 0);
    } else {
        infer(pps_num_exp_tile_columns_minus1, 0);
        infer(pps_tile_column_width_minus1[0], pic_width_in_ctbs_y - 1);
        infer(pps_num_exp_tile_rows_minus1, 0);
        infer(pps_tile_row_height_minus1[0], pic_height_in_ctbs_y - 1);
        current->col_width_val[0] = pic_width_in_ctbs_y;
        current->row_height_val[0] = pic_height_in_ctbs_y;
        current->num_tile_columns = 1;
        current->num_tile_rows = 1;
        current->num_tiles_in_pic = 1;
    }

    flag(pps_cabac_init_present_flag);
    for (i = 0; i < 2; i++)
        ues(pps_num_ref_idx_default_active_minus1[i], 0, 14, 1, i);
    flag(pps_rpl1_idx_present_flag);
    flag(pps_weighted_pred_flag);
    flag(pps_weighted_bipred_flag);
    flag(pps_ref_wraparound_enabled_flag);
    if (current->pps_ref_wraparound_enabled_flag) {
        ue(pps_pic_width_minus_wraparound_offset,
           0, (current->pps_pic_width_in_luma_samples / min_cb_size_y)
           - (ctb_size_y / min_cb_size_y) - 2);
    }

    qp_bd_offset = 6 * sps->sps_bitdepth_minus8;
    se(pps_init_qp_minus26, -(26 + qp_bd_offset), 37);
    flag(pps_cu_qp_delta_enabled_flag);
    flag(pps_chroma_tool_offsets_present_flag);
    if (current->pps_chroma_tool_offsets_present_flag) {
        se(pps_cb_qp_offset, -12, 12);
        se(pps_cr_qp_offset, -12, 12);
        flag(pps_joint_cbcr_qp_offset_present_flag);
        if (current->pps_joint_cbcr_qp_offset_present_flag)
            se(pps_joint_cbcr_qp_offset_value, -12, 12);
        else
            infer(pps_joint_cbcr_qp_offset_value, 0);
        flag(pps_slice_chroma_qp_offsets_present_flag);
        flag(pps_cu_chroma_qp_offset_list_enabled_flag);
        if (current->pps_cu_chroma_qp_offset_list_enabled_flag) {
            ue(pps_chroma_qp_offset_list_len_minus1, 0, 5);
            for (i = 0; i <= current->pps_chroma_qp_offset_list_len_minus1; i++) {
                ses(pps_cb_qp_offset_list[i], -12, 12, 1, i);
                ses(pps_cr_qp_offset_list[i], -12, 12, 1, i);
                if (current->pps_joint_cbcr_qp_offset_present_flag)
                    ses(pps_joint_cbcr_qp_offset_list[i], -12, 12, 1, i);
                else
                    infer(pps_joint_cbcr_qp_offset_list[i], 0);
            }
        }
    } else {
        infer(pps_cb_qp_offset, 0);
        infer(pps_cr_qp_offset, 0);
        infer(pps_joint_cbcr_qp_offset_present_flag, 0);
        infer(pps_joint_cbcr_qp_offset_value, 0);
        infer(pps_slice_chroma_qp_offsets_present_flag, 0);
        infer(pps_cu_chroma_qp_offset_list_enabled_flag, 0);
    }
    flag(pps_deblocking_filter_control_present_flag);
    if (current->pps_deblocking_filter_control_present_flag) {
        flag(pps_deblocking_filter_override_enabled_flag);
        flag(pps_deblocking_filter_disabled_flag);
        if (!current->pps_no_pic_partition_flag &&
            current->pps_deblocking_filter_override_enabled_flag)
            flag(pps_dbf_info_in_ph_flag);
        else
            infer(pps_dbf_info_in_ph_flag, 0);
        if (!current->pps_deblocking_filter_disabled_flag) {
            se(pps_luma_beta_offset_div2, -12, 12);
            se(pps_luma_tc_offset_div2, -12, 12);
            if (current->pps_chroma_tool_offsets_present_flag) {
                se(pps_cb_beta_offset_div2, -12, 12);
                se(pps_cb_tc_offset_div2, -12, 12);
                se(pps_cr_beta_offset_div2, -12, 12);
                se(pps_cr_tc_offset_div2, -12, 12);
            } else {
                infer(pps_cb_beta_offset_div2,
                      current->pps_luma_beta_offset_div2);
                infer(pps_cb_tc_offset_div2, current->pps_luma_tc_offset_div2);
                infer(pps_cr_beta_offset_div2,
                      current->pps_luma_beta_offset_div2);
                infer(pps_cr_tc_offset_div2, current->pps_luma_tc_offset_div2);
            }
        } else {
            infer(pps_luma_beta_offset_div2, 0);
            infer(pps_luma_tc_offset_div2, 0);
            infer(pps_cb_beta_offset_div2, 0);
            infer(pps_cb_tc_offset_div2, 0);
            infer(pps_cr_beta_offset_div2, 0);
            infer(pps_cr_tc_offset_div2, 0);
        }
    } else {
        infer(pps_deblocking_filter_override_enabled_flag, 0);
        infer(pps_deblocking_filter_disabled_flag, 0);
        infer(pps_dbf_info_in_ph_flag, 0);
        infer(pps_luma_beta_offset_div2, 0);
        infer(pps_luma_tc_offset_div2, 0);
        infer(pps_cb_beta_offset_div2, 0);
        infer(pps_cb_tc_offset_div2, 0);
        infer(pps_cr_beta_offset_div2, 0);
        infer(pps_cr_tc_offset_div2, 0);
    }

    if (!current->pps_no_pic_partition_flag) {
        flag(pps_rpl_info_in_ph_flag);
        flag(pps_sao_info_in_ph_flag);
        flag(pps_alf_info_in_ph_flag);
        if ((current->pps_weighted_pred_flag ||
             current->pps_weighted_bipred_flag) &&
            current->pps_rpl_info_in_ph_flag)
            flag(pps_wp_info_in_ph_flag);
        flag(pps_qp_delta_info_in_ph_flag);
    }
    flag(pps_picture_header_extension_present_flag);
    flag(pps_slice_header_extension_present_flag);

    flag(pps_extension_flag);
    if (current->pps_extension_flag)
        CHECK(FUNC(extension_data) (ctx, rw, &current->extension_data));

    CHECK(FUNC(rbsp_trailing_bits) (ctx, rw));
    return 0;
}