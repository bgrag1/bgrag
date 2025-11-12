static lbool metachar(char c)
{
	return (strchr(metachars(), c) != NULL);
}