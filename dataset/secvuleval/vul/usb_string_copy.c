static int usb_string_copy(const char *s, char **s_copy)
{
	int ret;
	char *str;
	char *copy = *s_copy;
	ret = strlen(s);
	if (ret > USB_MAX_STRING_LEN)
		return -EOVERFLOW;

	if (copy) {
		str = copy;
	} else {
		str = kmalloc(USB_MAX_STRING_WITH_NULL_LEN, GFP_KERNEL);
		if (!str)
			return -ENOMEM;
	}
	strcpy(str, s);
	if (str[ret - 1] == '\n')
		str[ret - 1] = '\0';
	*s_copy = str;
	return 0;
}
