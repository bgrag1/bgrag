static NETWORK_INTERFACE_DESCRIPTION* create_network_interface_description(struct ifreq *ifr, NETWORK_INTERFACE_DESCRIPTION* previous_nid)
{
    NETWORK_INTERFACE_DESCRIPTION* result;
    size_t malloc_size;

    if ((result = (NETWORK_INTERFACE_DESCRIPTION*)malloc(sizeof(NETWORK_INTERFACE_DESCRIPTION))) == NULL)
    {
        LogError("Failed allocating NETWORK_INTERFACE_DESCRIPTION");
    }
    else if ((malloc_size = safe_multiply_size_t(safe_add_size_t(strlen(ifr->ifr_name), 1), sizeof(char))) == SIZE_MAX)
    {
        LogError("invalid malloc size");
        destroy_network_interface_descriptions(result);
        result = NULL;
    }
    else if ((result->name = (char*)malloc(malloc_size)) == NULL)
    {
        LogError("failed setting interface description name (malloc failed)");
        destroy_network_interface_descriptions(result);
        result = NULL;
    }
    else
    {
        strcpy(result->name, ifr->ifr_name);

        char* ip_address;
        unsigned char* mac = (unsigned char*)ifr->ifr_hwaddr.sa_data;
        size_t malloc_size = safe_multiply_size_t(sizeof(char), MAC_ADDRESS_STRING_LENGTH);

        if (malloc_size == SIZE_MAX ||
                (result->mac_address = (char*)malloc(malloc_size)) == NULL)
        {
            LogError("failed formatting mac address (malloc failed) size:%zu", malloc_size);
            destroy_network_interface_descriptions(result);
            result = NULL;
        }
        else if (sprintf(result->mac_address, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]) <= 0)
        {
            LogError("failed formatting mac address (sprintf failed)");
            destroy_network_interface_descriptions(result);
            result = NULL;
        }
        else if ((ip_address = inet_ntoa(((struct sockaddr_in*)&ifr->ifr_addr)->sin_addr)) == NULL)
        {
            LogError("failed setting the ip address (inet_ntoa failed)");
            destroy_network_interface_descriptions(result);
            result = NULL;
        }
        else if ((malloc_size = safe_multiply_size_t(safe_add_size_t(strlen(ip_address), 1), sizeof(char))) == SIZE_MAX)
        {
            LogError("invalid malloc size");
            destroy_network_interface_descriptions(result);
            result = NULL;
        }
        else if ((result->ip_address = (char*)malloc(malloc_size)) == NULL)
        {
            LogError("failed setting the ip address (malloc failed)");
            destroy_network_interface_descriptions(result);
            result = NULL;
        }
        else
        {
            strcpy(result->ip_address, ip_address);
            result->next = NULL;

            if (previous_nid != NULL)
            {
                previous_nid->next = result;
            }
        }
    }

    return result;
}