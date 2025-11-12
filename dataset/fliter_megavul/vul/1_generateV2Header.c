bool generateV2Header(const Network::ProxyProtocolData& proxy_proto_data, Buffer::Instance& out,
                      bool pass_all_tlvs, const absl::flat_hash_set<uint8_t>& pass_through_tlvs) {
  uint64_t extension_length = 0;
  for (auto&& tlv : proxy_proto_data.tlv_vector_) {
    if (!pass_all_tlvs && !pass_through_tlvs.contains(tlv.type)) {
      continue;
    }
    extension_length += PROXY_PROTO_V2_TLV_TYPE_LENGTH_LEN + tlv.value.size();
    if (extension_length > std::numeric_limits<uint16_t>::max()) {
      ENVOY_LOG_MISC(
          warn, "Generating Proxy Protocol V2 header: TLVs exceed length limit {}, already got {}",
          std::numeric_limits<uint16_t>::max(), extension_length);
      return false;
    }
  }

  ASSERT(extension_length <= std::numeric_limits<uint16_t>::max());
  const auto& src = *proxy_proto_data.src_addr_->ip();
  const auto& dst = *proxy_proto_data.dst_addr_->ip();
  generateV2Header(src.addressAsString(), dst.addressAsString(), src.port(), dst.port(),
                   src.version(), static_cast<uint16_t>(extension_length), out);

  // Generate the TLV vector.
  for (auto&& tlv : proxy_proto_data.tlv_vector_) {
    if (!pass_all_tlvs && !pass_through_tlvs.contains(tlv.type)) {
      continue;
    }
    out.add(&tlv.type, 1);
    uint16_t size = htons(static_cast<uint16_t>(tlv.value.size()));
    out.add(&size, sizeof(uint16_t));
    out.add(&tlv.value.front(), tlv.value.size());
  }
  return true;
}