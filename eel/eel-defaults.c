#include <config.h>

#include <stdlib.h>
#include <stdio.h>

_Bool eel_default_boolean (const char *key)
{

    return 0;
}
int eel_default_enum (const char *key)
{
    return -1;
}
int eel_default_int (const char *key)
{
    return -1;
}
char *eel_default_string (const char *key)
{
    return NULL;
}
char **eel_default_strv (const char *key)
{
    return NULL;
}
void *eel_default_value (const char *key)
{
    return NULL;
}
