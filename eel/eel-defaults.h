#ifndef EEL_DEFAULTS_H
#define EEL_DEFAULTS_H

_Bool         eel_default_boolean (const char *key);
int           eel_default_enum    (const char *key);
int           eel_default_int     (const char *key);
char         *eel_default_string  (const char *key);
char        **eel_default_strv    (const char *key);
void         *eel_default_value   (const char *key);

#endif /* EEL_DEFAULTS_H */