int test_gtpu_send_indirect_data_forwarding(
        ogs_socknode_t *node, test_bearer_t *bearer, ogs_pkbuf_t *pkbuf)
{
    test_sess_t *sess = NULL;

    ogs_gtp2_header_t gtp_hdesc;
    ogs_gtp2_extension_header_t ext_hdesc;

    ogs_assert(bearer);
    sess = bearer->sess;
    ogs_assert(sess);
    ogs_assert(pkbuf);

    memset(&gtp_hdesc, 0, sizeof(gtp_hdesc));
    memset(&ext_hdesc, 0, sizeof(ext_hdesc));

    gtp_hdesc.type = OGS_GTPU_MSGTYPE_GPDU;

    if (bearer->qfi) {
        gtp_hdesc.teid = sess->handover.upf_dl_teid;
        ext_hdesc.qos_flow_identifier = bearer->qfi;

    } else if (bearer->ebi) {
        gtp_hdesc.teid = bearer->handover.ul_teid;

    } else {
        ogs_fatal("No QFI[%d] and EBI[%d]", bearer->qfi, bearer->ebi);
        ogs_assert_if_reached();
    }

    return test_gtpu_send(node, bearer, &gtp_hdesc, &ext_hdesc, pkbuf);
}