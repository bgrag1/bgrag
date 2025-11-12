static void dissect_readreg_ack(proto_tree *gvcp_telegram_tree, tvbuff_t *tvb, packet_info *pinfo, gint startoffset, gint length, gvcp_conv_info_t *gvcp_info, gvcp_transaction_t *gvcp_trans)
{
	guint i;
	gboolean is_custom_register = FALSE;
	const gchar* address_string = NULL;
	guint num_registers;
	gint offset;
	gboolean valid_trans = FALSE;
	guint addr_list_size = 0;

	offset = startoffset;
	num_registers = length / 4;

	if (gvcp_trans && gvcp_trans->addr_list)
	{
		valid_trans = TRUE;
		addr_list_size = wmem_array_get_count(gvcp_trans->addr_list);
	}

	if (num_registers > 1)
	{
		col_append_fstr(pinfo->cinfo, COL_INFO, "[Multiple ReadReg Ack]");
	}
	else
	{
		if (valid_trans)
		{
			if (addr_list_size > 0)
			{
				address_string = get_register_name_from_address(*((guint32*)wmem_array_index(gvcp_trans->addr_list, 0)), pinfo->pool, gvcp_info, &is_custom_register);
				col_append_str(pinfo->cinfo, COL_INFO, address_string);
			}

			if (num_registers)
			{
				col_append_sep_fstr(pinfo->cinfo, COL_INFO, " ", "Value=0x%08X", tvb_get_ntohl(tvb, offset));
			}
		}
	}

	if (gvcp_telegram_tree != NULL)
	{
		/* Subtree initialization for Payload Data: READREG_ACK */
		if (num_registers > 1)
		{
			gvcp_telegram_tree = proto_tree_add_subtree(gvcp_telegram_tree, tvb, offset, length,
												ett_gvcp_payload_ack, NULL, "Register Value List");
		}

		for (i = 0; i < num_registers; i++)
		{
			guint32 curr_register = 0;

			if (valid_trans && i < addr_list_size)
			{
				gint stream_channel_count = 0;
				curr_register = *((guint32*)wmem_array_index(gvcp_trans->addr_list, i));
				address_string = get_register_name_from_address(curr_register, pinfo->pool, gvcp_info, &is_custom_register);
				for (; stream_channel_count < GVCP_MAX_STREAM_CHANNEL_COUNT; stream_channel_count++)
				{
					if (curr_register == (guint32)GVCP_SC_EXTENDED_BOOTSTRAP_ADDRESS(stream_channel_count))
					{
						gvcp_info->extended_bootstrap_address[stream_channel_count] = tvb_get_ntohl(tvb, offset);
						break;
					}
				}

				if (!is_custom_register) /* bootstrap register */
				{
					guint32 extended_bootstrap_address_offset = 0;
					if (is_extended_bootstrap_address(gvcp_info, curr_register, &extended_bootstrap_address_offset))
					{
						proto_tree_add_uint_format_value(gvcp_telegram_tree, hf_gvcp_readregcmd_extended_bootstrap_register, tvb, offset, 4, curr_register, "%s (0x%08X)", address_string, curr_register);
						dissect_extended_bootstrap_register(curr_register - extended_bootstrap_address_offset, gvcp_telegram_tree, tvb, offset, length);
					}
					else
					{
						proto_tree_add_uint(gvcp_telegram_tree, hf_gvcp_readregcmd_bootstrap_register, tvb, 0, 4, curr_register);
						dissect_register(curr_register, gvcp_telegram_tree, tvb, offset, length);
					}
				}
				else
				{
					proto_tree_add_uint_format_value(gvcp_telegram_tree, hf_gvcp_custom_read_register_addr, tvb, offset, 4, curr_register, "%s (0x%08X)", address_string, curr_register);
					proto_tree_add_item(gvcp_telegram_tree, hf_gvcp_custom_read_register_value, tvb, offset, 4, ENC_BIG_ENDIAN);
				}
			}
			else
			{
				proto_tree_add_item(gvcp_telegram_tree, hf_gvcp_custom_register_value, tvb, offset, 4, ENC_BIG_ENDIAN);
			}

			offset += 4;
		}
	}
}