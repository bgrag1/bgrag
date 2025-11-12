getString(DcmDataset *obj, DcmTagKey t, char *s, int maxlen, OFBool *spacePadded)
{
    DcmElement *elem;
    DcmStack stack;
    OFCondition ec = EC_Normal;
    char* aString;

    ec = obj->search(t, stack);
    elem = (DcmElement*)stack.top();
    if (ec == EC_Normal && elem != NULL) {
        if (elem->getLength() == 0) {
            s[0] = '\0';
        } else if (elem->getLength() > (Uint32)maxlen) {
            return parseErrorWithMsg("dimcmd:getString: string too small", t);
        } else {
            ec =  elem->getString(aString);
            if (ec.good())
            {
                strncpy(s, aString, maxlen);
                if (spacePadded)
                {
                    /* before we remove leading and tailing spaces we want to know
                     * whether the string is actually space padded. Required to communicate
                     * with dumb peers which send space padded UIDs and fail if they
                     * receive correct UIDs back.
                     *
                     * This test can only detect space padded strings if
                     * dcmEnableAutomaticInputDataCorrection is false; otherwise the padding
                     * has already been removed by dcmdata at this stage.
                     */
                    size_t s_len = strlen(s);
                    if ((s_len > 0)&&(s[s_len-1] == ' ')) *spacePadded = OFTrue; else *spacePadded = OFFalse;
                }
                DU_stripLeadingAndTrailingSpaces(s);
            }
        }
    }
    return (ec.good())? ec : DIMSE_PARSEFAILED;
}