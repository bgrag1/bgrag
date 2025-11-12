TEST(TransportUDP, MaliciousManipulatedDataOctetsToNextHeaderIgnore)
{
    // Force using UDP transport
    auto udp_transport = std::make_shared<UDPv4TransportDescriptor>();

    PubSubWriter<UnboundedHelloWorldPubSubType> writer(TEST_TOPIC_NAME);
    PubSubReader<UnboundedHelloWorldPubSubType> reader(TEST_TOPIC_NAME);

    struct MaliciousManipulatedDataOctetsToNextHeader
    {
        std::array<char, 4> rtps_id{ {'R', 'T', 'P', 'S'} };
        std::array<uint8_t, 2> protocol_version{ {2, 3} };
        std::array<uint8_t, 2> vendor_id{ {0x01, 0x0F} };
        GuidPrefix_t sender_prefix{};

        struct DataSubMsg
        {
            struct Header
            {
                uint8_t submessage_id = 0x15;
#if FASTDDS_IS_BIG_ENDIAN_TARGET
                uint8_t flags = 0x04;
#else
                uint8_t flags = 0x05;
#endif  // FASTDDS_IS_BIG_ENDIAN_TARGET
                uint16_t octets_to_next_header = 0x30;
                uint16_t extra_flags = 0;
                uint16_t octets_to_inline_qos = 0x2d;
                EntityId_t reader_id{};
                EntityId_t writer_id{};
                SequenceNumber_t sn{100};
            };

            struct SerializedData
            {
                uint16_t encapsulation;
                uint16_t encapsulation_opts;
                octet data[24];
            };

            Header header;
            SerializedData payload;
        }
        data;

        uint8_t additional_bytes[8] {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    };

    UDPMessageSender fake_msg_sender;

    // Set common QoS
    reader.disable_builtin_transport().add_user_transport_to_pparams(udp_transport)
            .history_depth(10).reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS);
    writer.history_depth(10).reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS);

    // Set custom reader locator so we can send malicious data to a known location
    Locator_t reader_locator;
    ASSERT_TRUE(IPLocator::setIPv4(reader_locator, "127.0.0.1"));
    reader_locator.port = 7000;
    reader.add_to_unicast_locator_list("127.0.0.1", 7000);

    // Initialize and wait for discovery
    reader.init();
    ASSERT_TRUE(reader.isInitialized());
    writer.init();
    ASSERT_TRUE(writer.isInitialized());

    reader.wait_discovery();
    writer.wait_discovery();

    auto data = default_unbounded_helloworld_data_generator();
    reader.startReception(data);
    writer.send(data);
    ASSERT_TRUE(data.empty());

    // Send malicious data
    {
        auto writer_guid = writer.datawriter_guid();

        MaliciousManipulatedDataOctetsToNextHeader malicious_packet{};
        malicious_packet.sender_prefix = writer_guid.guidPrefix;
        malicious_packet.data.header.writer_id = writer_guid.entityId;
        malicious_packet.data.header.reader_id = reader.datareader_guid().entityId;
        malicious_packet.data.payload.encapsulation = CDR_LE;

        CDRMessage_t msg(0);
        uint32_t msg_len = static_cast<uint32_t>(sizeof(malicious_packet));
        msg.init(reinterpret_cast<octet*>(&malicious_packet), msg_len);
        msg.length = msg_len;
        msg.pos = msg_len;
        fake_msg_sender.send(msg, reader_locator);
    }

    // Block reader until reception finished or timeout.
    reader.block_for_all();
}