static lbool must_quote(char c)
{
	/* {{ Maybe the set of must_quote chars should be configurable? }} */
	return (c == '\n'); 
}