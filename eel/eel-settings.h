/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*!
 * \file eel-settings.h: Consolidated gsetting API with fail-safe.
 *
 * Copyright (C) 2014 Wiley Edward Hill <wileyhill@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef EEL_SETTINGS_H
#define EEL_SETTINGS_H

int           eel_settings_get_int               (GSettings         *settings,
                                                  const char        *key);
_Bool         eel_settings_set_int               (GSettings         *settings,
                                                  const char        *key,
                                                  int                value);
char         *eel_settings_get_string            (GSettings         *settings,
                                                  const char        *key);
_Bool         eel_settings_set_string            (GSettings         *settings,
                                                  const char        *key,
                                                  const char        *value);
_Bool         eel_settings_get_boolean           (GSettings         *settings,
                                                  const char        *key);
_Bool         eel_settings_set_boolean           (GSettings         *settings,
                                                  const char        *key,
                                                  _Bool              value);
char        **eel_settings_get_strv              (GSettings         *settings,
                                                  const char        *key);
_Bool         eel_settings_set_strv              (GSettings         *settings,
                                                  const char        *key,
                                                  const char *const *value);
int           eel_settings_get_enum              (GSettings         *settings,
                                                  const char        *key);
_Bool         eel_settings_set_enum              (GSettings         *settings,
                                                  const char        *key,
                                                  int                value);

_Bool         eel_settings_set_value             (GSettings         *settings,
                                                  const char        *key,
                                                  GVariant          *value);
GVariant     *eel_settings_get_value             (GSettings         *settings,
                                                  const char        *key);
#endif /* EEL_SETTINGS_H */