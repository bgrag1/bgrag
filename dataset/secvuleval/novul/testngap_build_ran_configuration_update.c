ogs_pkbuf_t *testngap_build_ran_configuration_update(bool supported_ta_list)
{
    ogs_pkbuf_t *pkbuf = NULL;
    int i, j, k, num;
    ogs_plmn_id_t *plmn_id = NULL;

    NGAP_NGAP_PDU_t pdu;
    NGAP_InitiatingMessage_t *initiatingMessage = NULL;
    NGAP_RANConfigurationUpdate_t *RANConfigurationUpdate = NULL;

    NGAP_RANConfigurationUpdateIEs_t *ie = NULL;
    NGAP_GlobalRANNodeID_t *GlobalRANNodeID = NULL;
    NGAP_RANNodeName_t *RANNodeName = NULL;
    NGAP_GlobalGNB_ID_t *globalGNB_ID = NULL;
    NGAP_SupportedTAList_t *SupportedTAList = NULL;
    NGAP_SupportedTAItem_t *SupportedTAItem = NULL;
    NGAP_BroadcastPLMNItem_t *BroadcastPLMNItem = NULL;
    NGAP_SliceSupportItem_t *SliceSupportItem = NULL;
    NGAP_PLMNIdentity_t *pLMNIdentity = NULL;

    memset(&pdu, 0, sizeof (NGAP_NGAP_PDU_t));
    pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = CALLOC(1, sizeof(NGAP_InitiatingMessage_t));

    initiatingMessage = pdu.choice.initiatingMessage;
    initiatingMessage->procedureCode =
        NGAP_ProcedureCode_id_RANConfigurationUpdate;
    initiatingMessage->criticality = NGAP_Criticality_reject;
    initiatingMessage->value.present =
        NGAP_InitiatingMessage__value_PR_RANConfigurationUpdate;

    RANConfigurationUpdate =
        &initiatingMessage->value.choice.RANConfigurationUpdate;

    if (supported_ta_list == true) {
        ie = CALLOC(1, sizeof(NGAP_RANConfigurationUpdateIEs_t));
        ASN_SEQUENCE_ADD(&RANConfigurationUpdate->protocolIEs, ie);

        ie->id = NGAP_ProtocolIE_ID_id_SupportedTAList;
        ie->criticality = NGAP_Criticality_reject;
        ie->value.present =
            NGAP_RANConfigurationUpdateIEs__value_PR_SupportedTAList;

        SupportedTAList = &ie->value.choice.SupportedTAList;

        if (test_self()->nr_served_tai[0].list2.num)
            num = test_self()->nr_served_tai[0].list2.num;
        else if (test_self()->nr_served_tai[0].list0.tai[0].num)
            num = test_self()->nr_served_tai[0].list0.tai[0].num;
        else
            ogs_assert_if_reached();

        for (i = 0; i < num; i++) {
            SupportedTAItem = CALLOC(1, sizeof(NGAP_SupportedTAItem_t));
            if (test_self()->nr_served_tai[0].list2.num)
                ogs_asn_uint24_to_OCTET_STRING(
                    test_self()->nr_served_tai[0].list2.tai[i].tac,
                    &SupportedTAItem->tAC);
            else if (test_self()->nr_served_tai[0].list0.tai[0].num)
                ogs_asn_uint24_to_OCTET_STRING(
                    test_self()->nr_served_tai[0].list0.tai[0].tac[i],
                        &SupportedTAItem->tAC);
            else
                ogs_assert_if_reached();

            for (j = 0; j < test_self()->num_of_plmn_support; j++) {
                plmn_id = &test_self()->plmn_support[j].plmn_id;

                BroadcastPLMNItem = CALLOC(1, sizeof(NGAP_BroadcastPLMNItem_t));

                ogs_asn_buffer_to_OCTET_STRING(
                        plmn_id, OGS_PLMN_ID_LEN,
                        &BroadcastPLMNItem->pLMNIdentity);

                for (k = 0; k < test_self()->plmn_support[j].num_of_s_nssai;
                        k++) {
                    ogs_s_nssai_t *s_nssai =
                        &test_self()->plmn_support[j].s_nssai[k];

                    SliceSupportItem = CALLOC(1,
                            sizeof(NGAP_SliceSupportItem_t));
                    ogs_asn_uint8_to_OCTET_STRING(s_nssai->sst,
                            &SliceSupportItem->s_NSSAI.sST);
                    if (s_nssai->sd.v != OGS_S_NSSAI_NO_SD_VALUE) {
                        SliceSupportItem->s_NSSAI.sD = CALLOC(
                                1, sizeof(NGAP_SD_t));
                        ogs_asn_uint24_to_OCTET_STRING(
                                s_nssai->sd, SliceSupportItem->s_NSSAI.sD);
                    }

                    ASN_SEQUENCE_ADD(
                            &BroadcastPLMNItem->tAISliceSupportList.list,
                            SliceSupportItem);
                }

                ASN_SEQUENCE_ADD(&SupportedTAItem->broadcastPLMNList.list,
                        BroadcastPLMNItem);
            }

            ASN_SEQUENCE_ADD(&SupportedTAList->list, SupportedTAItem);
        }
    }

    return ogs_ngap_encode(&pdu);
}