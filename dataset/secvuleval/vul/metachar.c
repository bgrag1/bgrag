static int metachar(char c)
{
	return (strchr(metachars(), c) != NULL);
}