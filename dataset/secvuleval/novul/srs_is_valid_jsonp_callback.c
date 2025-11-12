bool srs_is_valid_jsonp_callback(std::string callback)
{
    for (int i = 0; i < (int)callback.length(); i++) {
        char ch = callback.at(i);
        bool is_alpha_beta = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
        bool is_number = (ch >= '0' && ch <= '9');
        if (!is_alpha_beta && !is_number && ch != '.' && ch != '_' && ch != '-') {
            return false;
        }
    }
    return true;
}