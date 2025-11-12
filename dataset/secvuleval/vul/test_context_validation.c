static int test_context_validation(void)
{
    ogs_socknode_t *snode = NULL;
    int index = 0;

    snode = ogs_list_first(&test_self()->s1ap_list);
    if (snode) test_self()->s1ap_addr = snode->addr;
    snode = ogs_list_first(&test_self()->s1ap_list6);
    if (snode) test_self()->s1ap_addr6 = snode->addr;

    snode = ogs_list_first(&test_self()->ngap_list);
    if (snode) test_self()->ngap_addr = snode->addr;
    snode = ogs_list_first(&test_self()->ngap_list6);
    if (snode) test_self()->ngap_addr6 = snode->addr;

    if (test_self()->e_served_tai[index].list2.num) {
        memcpy(&test_self()->e_tai,
            &test_self()->e_served_tai[index].list2.tai[0], sizeof(ogs_5gs_tai_t));
    } else if (test_self()->e_served_tai[index].list1.tai[0].num) {
        test_self()->e_tai.tac =
            test_self()->e_served_tai[index].list1.tai[0].tac;
        memcpy(&test_self()->e_tai.plmn_id,
                &test_self()->e_served_tai[index].list1.tai[0].plmn_id,
                OGS_PLMN_ID_LEN);
    } else if (test_self()->e_served_tai[index].list0.tai[0].num) {
        test_self()->e_tai.tac =
            test_self()->e_served_tai[index].list0.tai[0].tac[0];
        memcpy(&test_self()->e_tai.plmn_id,
                &test_self()->e_served_tai[index].list0.tai[0].plmn_id,
                OGS_PLMN_ID_LEN);
    }

    if (test_self()->nr_served_tai[index].list2.num) {
        memcpy(&test_self()->nr_tai,
            &test_self()->nr_served_tai[index].list2.tai[0], sizeof(ogs_5gs_tai_t));
    } else if (test_self()->nr_served_tai[index].list1.tai[0].num) {
        test_self()->nr_tai.tac =
            test_self()->nr_served_tai[index].list1.tai[0].tac;
        memcpy(&test_self()->nr_tai.plmn_id,
                &test_self()->nr_served_tai[index].list1.tai[0].plmn_id,
                OGS_PLMN_ID_LEN);
    } else if (test_self()->nr_served_tai[index].list0.tai[0].num) {
        test_self()->nr_tai.tac =
            test_self()->nr_served_tai[index].list0.tai[0].tac[0];
        memcpy(&test_self()->nr_tai.plmn_id,
                &test_self()->nr_served_tai[index].list0.tai[0].plmn_id,
                OGS_PLMN_ID_LEN);
    }

    memcpy(&test_self()->nr_cgi.plmn_id, &test_self()->nr_tai.plmn_id,
            OGS_PLMN_ID_LEN);
    test_self()->nr_cgi.cell_id = 0x40001;

    return OGS_OK;
}