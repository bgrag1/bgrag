TEST(TransportUDP, DatagramInjection)
{
    using eprosima::fastdds::rtps::DatagramInjectionTransportDescriptor;
    using eprosima::fastdds::rtps::DatagramInjectionTransport;

    auto low_level_transport = std::make_shared<UDPv4TransportDescriptor>();
    auto transport = std::make_shared<DatagramInjectionTransportDescriptor>(low_level_transport);

    PubSubWriter<HelloWorldPubSubType> writer(TEST_TOPIC_NAME);
    writer.disable_builtin_transport().add_user_transport_to_pparams(transport).init();
    ASSERT_TRUE(writer.isInitialized());

    auto receivers = transport->get_receivers();
    ASSERT_FALSE(receivers.empty());

    DatagramInjectionTransport::deliver_datagram_from_file(receivers, "datagrams/16784.bin");
    DatagramInjectionTransport::deliver_datagram_from_file(receivers, "datagrams/20140.bin");
    DatagramInjectionTransport::deliver_datagram_from_file(receivers, "datagrams/20574.bin");
    DatagramInjectionTransport::deliver_datagram_from_file(receivers, "datagrams/20660.bin");
}