/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-
 */
/*! \file eel-settings.c: Consolidated gsetting API with fail-safe.
 *
 * Copyright (C) 2014 Wiley Edward Hill
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the historicalied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Wiley Edward Hill <wileyhill@gmail.com>
 * Date:    05/14/2014
*/

#define KEY_FILE "/usr/share/glib-2.0/schemas/apps.nautilus.gschema.xml"
#define BUFFER_SIZE 1024

#include <config.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <gio/gio.h>

#include "eel-defaults.h"

_Bool is_valid_key(const char *key)
{
    _Bool result = FALSE;
    FILE * fp;
    char buffer[BUFFER_SIZE];

#ifdef DEBUG
    fprintf(stderr,"looking for  %s\n", key);
#endif

    errno = 0;
    if(access(KEY_FILE, R_OK) == 0) {
        if( (fp = fopen(KEY_FILE, "r")) != NULL) {
            while (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                if (strstr(key, buffer)) {

                  result = TRUE;
                  break;
                }
            }
            fclose(fp);
        }
        else {
            fprintf(stderr,"Could not open %s, %s\n", KEY_FILE, strerror (errno));
        }
    }
    else {
        fprintf(stderr,"Could not read %s, %s\n", KEY_FILE, strerror (errno));
    }
    return TRUE;
}

_Bool
eel_settings_get_boolean (GSettings *settings, const char *key)
{
    if (is_valid_key(key)) {
        return g_settings_get_boolean (settings, key);
    }
    else {
        fprintf(stderr,"key <%s> is missing, check %s\n", key, KEY_FILE);
    }
    return eel_default_boolean (key);
}

_Bool
eel_settings_set_boolean (GSettings *settings, const char *key, _Bool value)
{
    if (is_valid_key(key)) {
        return g_settings_set_boolean (settings, key, value);
    }
    else {
        fprintf(stderr,"key <%s> is missing, check %s\n", key, KEY_FILE);
    }
}

int
eel_settings_get_enum (GSettings *settings, const char *key)
{
    if (is_valid_key(key)) {
       return g_settings_get_enum (settings, key);
    }
    else {
        fprintf(stderr,"key <%s> is missing, check %s\n", key, KEY_FILE);
    }
    return eel_default_enum (key);
}

_Bool
eel_settings_set_enum (GSettings *settings, const char *key, int value)
{
    if (is_valid_key(key)) {
        return g_settings_set_enum (settings, key, value);
    }
    else {
        fprintf(stderr,"key <%s> is missing, check %s\n", key, KEY_FILE);
    }
    return FALSE;
}

int
eel_settings_get_int (GSettings *settings, const char *key)
{
    if (is_valid_key(key)) {
       return g_settings_get_int (settings, key);
    }
    else {
        fprintf(stderr,"key <%s> is missing, check %s\n", key, KEY_FILE);
    }
    return eel_default_int(key);
}

_Bool
eel_settings_set_int (GSettings *settings, const char *key, int value)
{
    if (is_valid_key(key)) {
        return g_settings_set_int (settings, key, value);
    }
    else {
        fprintf(stderr,"key <%s> is missing, check %s\n", key, KEY_FILE);
    }
    return FALSE;
}

char*
eel_settings_get_string (GSettings *settings, const char *key)
{
    if (is_valid_key (key)) {
       return g_settings_get_string (settings, key);
    }
    else {
        fprintf(stderr,"key <%s> is missing, check %s\n", key, KEY_FILE);
    }
    return eel_default_string (key);
}

_Bool
eel_settings_set_string (GSettings *settings, const char *key, const char *value)
{
    if (is_valid_key (key)) {
        return g_settings_set_string (settings, key, value);
    }
    else {
        fprintf(stderr,"key <%s> is missing, check %s\n", key, KEY_FILE);
    }
    return FALSE;
}

char**
eel_settings_get_strv (GSettings *settings, const char *key)
{
    if (is_valid_key(key)) {
        return g_settings_get_strv (settings, key);
    }
    else {
        fprintf(stderr,"key <%s> is missing, check %s\n", key, KEY_FILE);
    }
    return eel_default_strv (key);
}

_Bool
eel_settings_set_strv (GSettings *settings, const char *key, const char *const *value)
{
    if (is_valid_key(key)) {
        return g_settings_set_strv (settings, key, value);
    }
    else {
        fprintf(stderr,"key <%s> is missing, check %s\n", key, KEY_FILE);
    }
    return FALSE;
}

GVariant*
eel_settings_get_value (GSettings *settings, const char *key)
{
    if (is_valid_key(key)) {
        return g_settings_get_value (settings, key);
    }
    else {
        fprintf(stderr,"key <%s> is missing, check %s\n", key, KEY_FILE);
    }
    return eel_default_value (key);
}

_Bool
eel_settings_set_value (GSettings *settings, const char *key, GVariant *value)
{
    if (is_valid_key(key)) {
        return g_settings_get_value (settings, key);
    }
    else {
        fprintf(stderr,"key <%s> is missing, check %s\n", key, KEY_FILE);
    }
    return FALSE;
}
