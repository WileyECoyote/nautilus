/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-global-preferences.c - Nautilus specific preference keys and
                                   functions.

   Copyright (C) 1999, 2000, 2001 Eazel, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

#include <config.h>

#include <gtk/gtk.h> /* For inclusion of nautilus-file-utilities.h */
#include <glib/gi18n.h>

#include <eel/eel.h>

#include "nautilus-global-preferences.h"

#include "nautilus-file-utilities.h"
#include "nautilus-file-attributes.h"
#include <nautilus-icon-info.h>

#include "nautilus-file.h"

/*
 * Public functions
 */

int
nautilus_global_preferences_get_tooltip_flags (void)
{
    NautilusFileTooltipFlags flags = NAUTILUS_FILE_TOOLTIP_FLAGS_NONE;

    if (nautilus_settings_get_show_file_type_tooltip())
        flags |= NAUTILUS_FILE_TOOLTIP_FLAGS_FILE_TYPE;

    if (nautilus_settings_get_show_mod_date_tooltip())
        flags |= NAUTILUS_FILE_TOOLTIP_FLAGS_MOD_DATE;

    if (nautilus_settings_get_show_access_date_tooltip())
        flags |= NAUTILUS_FILE_TOOLTIP_FLAGS_ACCESS_DATE;

    if (nautilus_settings_get_show_path_tooltip())
        flags |= NAUTILUS_FILE_TOOLTIP_FLAGS_PATH;

    if (nautilus_settings_get_show_permissions_tooltip())
        flags |= NAUTILUS_FILE_TOOLTIP_FLAGS_PERMISSIONS;

    if (nautilus_settings_get_show_octal_tooltip())
        flags |= NAUTILUS_FILE_TOOLTIP_FLAGS_OCTAL;

    return flags;
}
int
nautilus_global_preferences_get_treetip_flags (void)
{
    NautilusFileTooltipFlags flags = NAUTILUS_FILE_TOOLTIP_FLAGS_NONE;

    if (eel_settings_get_boolean (nautilus_tooltips_preferences, NAUTILUS_TREETIP_PREFERENCES_FILE_TYPE))
        flags |= NAUTILUS_FILE_TOOLTIP_FLAGS_FILE_TYPE;

    if (eel_settings_get_boolean (nautilus_tooltips_preferences, NAUTILUS_TREETIP_PREFERENCES_MOD_DATE))
        flags |= NAUTILUS_FILE_TOOLTIP_FLAGS_MOD_DATE;

    if (eel_settings_get_boolean (nautilus_tooltips_preferences, NAUTILUS_TREETIP_PREFERENCES_ACCESS_DATE))
        flags |= NAUTILUS_FILE_TOOLTIP_FLAGS_ACCESS_DATE;

    if (eel_settings_get_boolean (nautilus_tooltips_preferences, NAUTILUS_TREETIP_PREFERENCES_PERMISSIONS))
        flags |= NAUTILUS_FILE_TOOLTIP_FLAGS_PERMISSIONS;

    if (eel_settings_get_boolean (nautilus_tooltips_preferences, NAUTILUS_TREETIP_PREFERENCES_OCTAL))
        flags |= NAUTILUS_FILE_TOOLTIP_FLAGS_OCTAL;

    return flags;
}

void
nautilus_global_preferences_init (void)
{
  static _Bool initialized = FALSE;

  if (initialized) {
    return;
  }

  initialized = TRUE;

  nautilus_preferences              = g_settings_new("apps.nautilus.preferences");
  nautilus_window_state             = g_settings_new("apps.nautilus.window-state");
  nautilus_icon_view_preferences    = g_settings_new("apps.nautilus.icon-view");
  nautilus_list_view_preferences    = g_settings_new("apps.nautilus.list-view");
  nautilus_compact_view_preferences = g_settings_new("apps.nautilus.compact-view");
  nautilus_desktop_preferences      = g_settings_new("apps.nautilus.desktop");
  nautilus_tooltips_preferences     = g_settings_new("apps.nautilus.tool-tips");
  nautilus_tree_sidebar_preferences = g_settings_new("apps.nautilus.sidebar-panels.tree");
  gnome_lockdown_preferences        = g_settings_new("org.gnome.desktop.lockdown");
  gnome_background_preferences      = g_settings_new("org.gnome.desktop.background");

}
