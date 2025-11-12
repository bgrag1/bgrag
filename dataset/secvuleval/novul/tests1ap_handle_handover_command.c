void tests1ap_handle_handover_command(
        test_ue_t *test_ue, ogs_s1ap_message_t *message)
{
    int i;
    char buf[OGS_ADDRSTRLEN];

    test_sess_t *sess = NULL;
    test_bearer_t *bearer = NULL;

    S1AP_S1AP_PDU_t pdu;
    S1AP_SuccessfulOutcome_t *successfulOutcome = NULL;
    S1AP_HandoverCommand_t *HandoverCommand = NULL;

    S1AP_HandoverCommandIEs_t *ie = NULL;
    S1AP_MME_UE_S1AP_ID_t *MME_UE_S1AP_ID = NULL;
    S1AP_ENB_UE_S1AP_ID_t *ENB_UE_S1AP_ID = NULL;
    S1AP_E_RABSubjecttoDataForwardingList_t
        *E_RABSubjecttoDataForwardingList = NULL;

    ogs_assert(test_ue);
    ogs_assert(message);

    successfulOutcome = message->choice.successfulOutcome;
    ogs_assert(successfulOutcome);
    HandoverCommand = &successfulOutcome->value.choice.HandoverCommand;
    ogs_assert(HandoverCommand);

    for (i = 0; i < HandoverCommand->protocolIEs.list.count; i++) {
        ie = HandoverCommand->protocolIEs.list.array[i];
        switch (ie->id) {
        case S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID:
            MME_UE_S1AP_ID = &ie->value.choice.MME_UE_S1AP_ID;
            break;
        case S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID:
            ENB_UE_S1AP_ID = &ie->value.choice.ENB_UE_S1AP_ID;
            break;
        case S1AP_ProtocolIE_ID_id_E_RABSubjecttoDataForwardingList:
            E_RABSubjecttoDataForwardingList =
                &ie->value.choice.E_RABSubjecttoDataForwardingList;
            break;
        default:
            break;
        }
    }

    if (MME_UE_S1AP_ID)
        test_ue->mme_ue_s1ap_id = *MME_UE_S1AP_ID;
    if (ENB_UE_S1AP_ID)
        test_ue->enb_ue_s1ap_id = *ENB_UE_S1AP_ID;

    if (E_RABSubjecttoDataForwardingList) {
        for (i = 0; i < E_RABSubjecttoDataForwardingList->list.count; i++) {
            S1AP_E_RABDataForwardingItemIEs_t *ie = NULL;
            S1AP_E_RABDataForwardingItem_t *e_rab = NULL;

            ie = (S1AP_E_RABDataForwardingItemIEs_t *)
                    E_RABSubjecttoDataForwardingList->list.array[i];
            ogs_assert(ie);
            e_rab = &ie->value.choice.E_RABDataForwardingItem;

            bearer = test_bearer_find_by_ue_ebi(test_ue, e_rab->e_RAB_ID);
            ogs_assert(bearer);

            if (e_rab->dL_gTP_TEID) {
                memcpy(&bearer->handover.dl_teid, e_rab->dL_gTP_TEID->buf,
                        sizeof(bearer->handover.dl_teid));
                bearer->handover.dl_teid = be32toh(bearer->handover.dl_teid);
            }
            if (e_rab->dL_transportLayerAddress) {
                ogs_assert(OGS_OK ==
                        ogs_asn_BIT_STRING_to_ip(
                            e_rab->dL_transportLayerAddress,
                            &bearer->handover.dl_ip));
            }
            if (e_rab->uL_GTP_TEID) {
                memcpy(&bearer->handover.ul_teid, e_rab->uL_GTP_TEID->buf,
                        sizeof(bearer->handover.ul_teid));
                bearer->handover.ul_teid = be32toh(bearer->handover.ul_teid);
            }
            if (e_rab->uL_TransportLayerAddress) {
                ogs_assert(OGS_OK ==
                        ogs_asn_BIT_STRING_to_ip(
                            e_rab->uL_TransportLayerAddress,
                            &bearer->handover.ul_ip));
            }
        }
    }
}