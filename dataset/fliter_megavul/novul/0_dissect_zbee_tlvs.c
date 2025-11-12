guint
dissect_zbee_tlvs(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, guint offset, void *data, guint8 source_type, guint cmd_id)
{
    proto_tree  *subtree;
    guint8       length;
    unsigned     recursion_depth = p_get_proto_depth(pinfo, proto_zbee_tlv);

   if (++recursion_depth >= ZBEE_TLV_MAX_RECURSION_DEPTH) {
      proto_tree_add_expert(tree, pinfo, &ei_zbee_tlv_max_recursion_depth_reached, tvb, 0, 0);
      return tvb_reported_length_remaining(tvb, offset);
   }

   p_set_proto_depth(pinfo, proto_zbee_tlv, recursion_depth);

    while (tvb_bytes_exist(tvb, offset, ZBEE_TLV_HEADER_LENGTH)) {
        length = tvb_get_guint8(tvb, offset + 1) + 1;
        subtree = proto_tree_add_subtree(tree, tvb, offset, ZBEE_TLV_HEADER_LENGTH + length, ett_zbee_tlv, NULL, "TLV");
        offset = dissect_zbee_tlv(tvb, pinfo, subtree, offset, data, source_type, cmd_id);
    }

    recursion_depth = p_get_proto_depth(pinfo, proto_zbee_tlv);
    p_set_proto_depth(pinfo, proto_zbee_tlv, recursion_depth - 1);

    return offset;
}