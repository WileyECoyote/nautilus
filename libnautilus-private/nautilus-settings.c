/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   eel-tooltip.c: enhance GTK tool tips.

   Copyright (C) 2014 Wiley Edward Hill

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the historicalied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Wiley Edward Hill <wileyhill@gmail.com>
   Date:    05/14/2014
*/

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <gio/gio.h>

#include <eel/eel-settings.h>

#include "nautilus-global-preferences.h"
#include "nautilus-settings.h"

G_DEFINE_TYPE (NautilusSettings, nautilus_settings, G_TYPE_OBJECT)

static NautilusSettings *config = NULL;

/* String structure for Viewer Type IDs */
const char* IDS_Folder_Viewers[] = {
  NAUTILUS_ICON_VIEW_IID, NAUTILUS_COMPACT_VIEW_IID, NAUTILUS_LIST_VIEW_IID,
  NULL
};

/* always_use_browser */
_Bool nautilus_settings_get_always_use_browser (void)
{
    return config->always_use_browser;
}
static void
always_use_browser_changed (GSettings *settings, char *key)
{
    config->always_use_browser = eel_settings_get_boolean (settings, key);
}


/* click_policy */
_Bool nautilus_settings_get_click_policy (void)
{
    return config->click_policy;
}
static void
click_policy_changed (GSettings *settings, char *key)
{
    config->click_policy = eel_settings_get_enum (settings, key);
}


/*enable_delete */
_Bool  nautilus_settings_get_enable_delete (void)
{
    return config->enable_delete;
}
static void
enable_delete_changed (GSettings *settings, char *key)
{
    config->enable_delete = eel_settings_get_boolean (settings, key);
}

/* desktop_is_home_dir */
_Bool  nautilus_settings_get_desktop_is_home_dir   (void)
{
    return config->desktop_is_home_dir;
}
static void
desktop_is_home_dir_changed (GSettings *settings, char *key)
{
    config->desktop_is_home_dir = eel_settings_get_boolean (settings, key);
}


/* ignore_view_metadata */
_Bool
nautilus_settings_get_ignore_view_metadata (void)
{
    return config->ignore_view_metadata;
}
static void
ignore_view_metadata_changed (GSettings *settings, char *key)
{
    config->ignore_view_metadata = eel_settings_get_boolean (settings, key);
}

/* show_fill_path_in_title */
_Bool
nautilus_settings_get_show_full_path (void)
{
    return config->show_full_path;
}
static void
show_full_path_changed (GSettings *settings, char *key)
{
    config->show_full_path = eel_settings_get_boolean (settings, key);
}

/* show_hidden */
_Bool
nautilus_settings_get_show_hidden (void)
{
    return config->show_hidden;
}
static void
show_hidden_changed (GSettings *settings, char *key)
{
    config->show_hidden = eel_settings_get_boolean (settings, key);
}

/* folder_viewer */
const char*
nautilus_settings_get_default_folder_viewer (void)
{
    return config->folder_viewer;
}

static void
default_folder_viewer_changed (GSettings *settings, char *key)
{
    int enum_viewer = eel_settings_get_enum (nautilus_preferences,
                                             NAUTILUS_PREFERENCES_DEFAULT_FOLDER_VIEWER);
    switch (enum_viewer)
    {
        case NAUTILUS_DEFAULT_FOLDER_VIEWER_ICON_VIEW:
        case NAUTILUS_DEFAULT_FOLDER_VIEWER_COMPACT_VIEW:
        case NAUTILUS_DEFAULT_FOLDER_VIEWER_LIST_VIEW:
          config->folder_viewer = IDS_Folder_Viewers[enum_viewer];
           break;
        default:
          config->folder_viewer = IDS_Folder_Viewers[2];
          break;
    }
}


/* size_prefixes */
int
nautilus_settings_get_size_prefixes (void)
{
    return config->size_prefixes;
}
static void
size_prefixes_changed (GSettings *settings, char *key)
{
    config->size_prefixes = eel_settings_get_enum(settings, key);
}

/* sort_reversed */
_Bool
nautilus_settings_get_sort_directories_first (void)
{
    return config->directories_first;
}
static void
sort_directories_first_changed (GSettings *settings, char *key)
{
    config->directories_first = eel_settings_get_boolean (settings, key);
}

/* sort_order */
int
nautilus_settings_get_sort_order (void)
{
    return config->sort_order;
}
static void
sort_order_changed (GSettings *settings, char *key)
{
    config->sort_order = eel_settings_get_enum (settings, key);
}
/* sort_reversed */
_Bool
nautilus_settings_get_sort_reversed (void)
{
    return config->sort_reversed;
}
static void
sort_reversed_changed (GSettings *settings, char *key)
{
    config->sort_reversed = eel_settings_get_boolean (settings, key);
}


/* compact_zoom_level */
int
nautilus_settings_get_compact_zoom_level (void)
{
    return config->compact_zoom_level;
}
static void
compact_zoom_level_changed (GSettings *settings, char *key)
{
    config->compact_zoom_level = eel_settings_get_enum(settings, key);
}

/* icon_zoom_level */
int
nautilus_settings_get_icon_zoom_level (void)
{
    return config->icon_zoom_level;
}
static void
icon_zoom_level_changed (GSettings *settings, char *key)
{
    config->icon_zoom_level = eel_settings_get_enum(settings, key);
}

/* list_zoom_level */
int
nautilus_settings_get_list_zoom_level (void)
{
    return config->list_zoom_level;
}
static void
list_zoom_level_changed (GSettings *settings, char *key)
{
    config->list_zoom_level = eel_settings_get_enum(settings, key);
}


/* start_with_sidebar */
_Bool
nautilus_settings_get_start_with_sidebar (void)
{
    return config->start_with_sidebar;
}
static void
start_with_sidebar_changed (GSettings *settings, char *key)
{
    config->start_with_sidebar = eel_settings_get_boolean (settings, key);
}

/* start_with_status_bar */
_Bool
nautilus_settings_get_start_with_status_bar (void)
{
    return config->start_with_status_bar;
}
static void
start_with_status_bar_changed (GSettings *settings, char *key)
{
    config->start_with_status_bar = eel_settings_get_boolean (settings, key);
}


/* show_desktop */
_Bool
nautilus_settings_get_show_desktop (void)
{
    return config->show_desktop;
}
static void
show_desktop_changed (GSettings *settings, char *key)
{
    config->show_desktop = eel_settings_get_boolean (settings, key);
}

/* desktop_tooltips */
_Bool
nautilus_settings_get_show_desktop_tooltips (void)
{
    return config->tooltips_desktop;
}
static void
show_desktop_tooltips_changed (GSettings *settings, char *key)
{
    config->tooltips_desktop = eel_settings_get_boolean (settings, key);
}

/* icon_view_tooltips */
_Bool
nautilus_settings_get_show_icon_tooltips (void)
{
    return config->tooltips_icon_view;
}
static void
show_icon_tooltips_changed (GSettings *settings, char *key)
{
    config->tooltips_icon_view = eel_settings_get_boolean (settings, key);
}

/* tooltips_show_file_type */
_Bool
nautilus_settings_get_show_file_type_tooltip (void)
{
    return config->tooltips_file_type;
}
static void
show_file_type_tooltip_changed (GSettings *settings, char *key)
{
    config->tooltips_file_type = eel_settings_get_boolean (settings, key);
}

/* tooltips_show_mod_date */
_Bool
nautilus_settings_get_show_mod_date_tooltip (void)
{
    return config->tooltips_mod_date;
}
static void
show_mod_date_tooltip_changed (GSettings *settings, char *key)
{
    config->tooltips_mod_date = eel_settings_get_boolean (settings, key);
}

/* tooltips_show_access_date */
_Bool
nautilus_settings_get_show_access_date_tooltip (void)
{
    return config->tooltips_access_date;
}
static void
show_access_date_tooltip_changed (GSettings *settings, char *key)
{
    config->tooltips_access_date = eel_settings_get_boolean (settings, key);
}

/* tooltips_show_path */
_Bool
nautilus_settings_get_show_path_tooltip (void)
{
    return config->tooltips_show_path;
}
static void
show_path_tooltip_changed (GSettings *settings, char *key)
{
    config->tooltips_show_path = eel_settings_get_boolean (settings, key);
}

/* tooltips_show_permissions */
_Bool
nautilus_settings_get_show_permissions_tooltip (void)
{
    return config->tooltips_show_missions;
}
static void
show_permissions_tooltip_changed (GSettings *settings, char *key)
{
    config->tooltips_show_missions = eel_settings_get_boolean (settings, key);
}

/* tooltips_show_octal */
_Bool
nautilus_settings_get_show_octal_tooltip (void)
{
    return config->tooltips_show_octal;
}
static void
show_octal_tooltips_changed (GSettings *settings, char *key)
{
    config->tooltips_show_octal = eel_settings_get_boolean (settings, key);
}


/* nautilus_list_view_preferences */
char **nautilus_settings_default_column_order(void)
{
    return eel_settings_get_strv (nautilus_list_view_preferences,
                                  NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_COLUMN_ORDER);
}

char **nautilus_settings_default_visible_columns(void)
{
    return eel_settings_get_strv (nautilus_list_view_preferences,
                                  NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_VISIBLE_COLUMNS);
}

/* -------------------- NautilusSettings Class ------------------ */
static void
nautilus_settings_finalize (GObject *object)
{
  NautilusSettings *settings = NAUTILUS_SETTINGS (object);

  G_OBJECT_CLASS (nautilus_settings_parent_class)->finalize (object);
}

static void
nautilus_settings_class_init (NautilusSettingsClass *class)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (class);

  object_class->finalize  = nautilus_settings_finalize;

}

static void
nautilus_settings_init (NautilusSettings *settings)
{
  config = settings;

  /* General preferences == nautilus_preferences */
  always_use_browser_changed(nautilus_preferences, NAUTILUS_PREFERENCES_ALWAYS_USE_BROWSER);
  g_signal_connect (nautilus_preferences, "changed::" NAUTILUS_PREFERENCES_ALWAYS_USE_BROWSER,
                    G_CALLBACK (always_use_browser_changed), NULL);

  click_policy_changed(nautilus_preferences, NAUTILUS_PREFERENCES_CLICK_POLICY);
  g_signal_connect (nautilus_preferences, "changed::" NAUTILUS_PREFERENCES_CLICK_POLICY,
                    G_CALLBACK (click_policy_changed), NULL);

  enable_delete_changed(nautilus_preferences, NAUTILUS_PREFERENCES_ENABLE_DELETE);
  g_signal_connect (nautilus_preferences, "changed::" NAUTILUS_PREFERENCES_ENABLE_DELETE,
                    G_CALLBACK (enable_delete_changed), NULL);

  desktop_is_home_dir_changed(nautilus_preferences, NAUTILUS_PREFERENCES_DESKTOP_IS_HOME_DIR);
  g_signal_connect (nautilus_preferences, "changed::" NAUTILUS_PREFERENCES_DESKTOP_IS_HOME_DIR,
                    G_CALLBACK (desktop_is_home_dir_changed), NULL);

  ignore_view_metadata_changed(nautilus_preferences, NAUTILUS_PREFERENCES_IGNORE_VIEW_METADATA);
  g_signal_connect (nautilus_preferences, "changed::" NAUTILUS_PREFERENCES_IGNORE_VIEW_METADATA,
                    G_CALLBACK (ignore_view_metadata_changed), NULL);

  show_full_path_changed(nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_FULL_PATH_TITLES);
  g_signal_connect (nautilus_preferences, "changed::" NAUTILUS_PREFERENCES_SHOW_FULL_PATH_TITLES,
                    G_CALLBACK (show_full_path_changed), NULL);

  show_hidden_changed(nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES);
  g_signal_connect (nautilus_preferences, "changed::" NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES,
                    G_CALLBACK (show_hidden_changed), NULL);

  default_folder_viewer_changed(nautilus_preferences, NAUTILUS_PREFERENCES_DEFAULT_FOLDER_VIEWER);
  g_signal_connect (nautilus_preferences, "changed::" NAUTILUS_PREFERENCES_DEFAULT_FOLDER_VIEWER,
                    G_CALLBACK (default_folder_viewer_changed), NULL);

  size_prefixes_changed(nautilus_preferences, NAUTILUS_PREFERENCES_SIZE_PREFIXES);
  g_signal_connect (nautilus_preferences, "changed::" NAUTILUS_PREFERENCES_SIZE_PREFIXES,
                    G_CALLBACK (size_prefixes_changed), NULL);

  sort_directories_first_changed(nautilus_preferences, NAUTILUS_PREFERENCES_SORT_DIRECTORIES_FIRST);
  g_signal_connect (nautilus_preferences, "changed::" NAUTILUS_PREFERENCES_SORT_DIRECTORIES_FIRST,
                    G_CALLBACK (sort_directories_first_changed), NULL);

  sort_order_changed(nautilus_preferences, NAUTILUS_PREFERENCES_DEFAULT_SORT_ORDER);
  g_signal_connect (nautilus_preferences, "changed::" NAUTILUS_PREFERENCES_DEFAULT_SORT_ORDER,
                    G_CALLBACK (sort_order_changed), NULL);

  sort_reversed_changed(nautilus_preferences, NAUTILUS_PREFERENCES_DEFAULT_SORT_IN_REVERSE_ORDER);
  g_signal_connect (nautilus_preferences, "changed::" NAUTILUS_PREFERENCES_DEFAULT_SORT_IN_REVERSE_ORDER,
                    G_CALLBACK (sort_reversed_changed), NULL);

    /* Tooltip preferences == nautilus_tooltips_preferences */
  show_desktop_tooltips_changed(nautilus_tooltips_preferences, NAUTILUS_TOOLTIP_PREFERENCES_DESKTOP);
  g_signal_connect (nautilus_tooltips_preferences, "changed::" NAUTILUS_TOOLTIP_PREFERENCES_DESKTOP,
                    G_CALLBACK (show_desktop_tooltips_changed), NULL);

  show_icon_tooltips_changed(nautilus_tooltips_preferences, NAUTILUS_TOOLTIP_PREFERENCES_ICON_VIEW);
  g_signal_connect (nautilus_tooltips_preferences, "changed::" NAUTILUS_TOOLTIP_PREFERENCES_ICON_VIEW,
                    G_CALLBACK (show_icon_tooltips_changed), NULL);

  show_file_type_tooltip_changed(nautilus_tooltips_preferences, NAUTILUS_TOOLTIP_PREFERENCES_FILE_TYPE);
  g_signal_connect (nautilus_tooltips_preferences, "changed::" NAUTILUS_TOOLTIP_PREFERENCES_FILE_TYPE,
                    G_CALLBACK (show_file_type_tooltip_changed), NULL);

  show_mod_date_tooltip_changed(nautilus_tooltips_preferences, NAUTILUS_TOOLTIP_PREFERENCES_MOD_DATE);
  g_signal_connect (nautilus_tooltips_preferences, "changed::" NAUTILUS_TOOLTIP_PREFERENCES_MOD_DATE,
                    G_CALLBACK (show_mod_date_tooltip_changed), NULL);

  show_access_date_tooltip_changed(nautilus_tooltips_preferences, NAUTILUS_TOOLTIP_PREFERENCES_ACCESS_DATE);
  g_signal_connect (nautilus_tooltips_preferences, "changed::" NAUTILUS_TOOLTIP_PREFERENCES_ACCESS_DATE,
                    G_CALLBACK (show_access_date_tooltip_changed), NULL);

  show_path_tooltip_changed(nautilus_tooltips_preferences, NAUTILUS_TOOLTIP_PREFERENCES_FULL_PATH);
  g_signal_connect (nautilus_tooltips_preferences, "changed::" NAUTILUS_TOOLTIP_PREFERENCES_FULL_PATH,
                    G_CALLBACK (show_path_tooltip_changed), NULL);

  show_permissions_tooltip_changed(nautilus_tooltips_preferences, NAUTILUS_TOOLTIP_PREFERENCES_PERMISSIONS);
  g_signal_connect (nautilus_tooltips_preferences, "changed::" NAUTILUS_TOOLTIP_PREFERENCES_PERMISSIONS,
                    G_CALLBACK (show_permissions_tooltip_changed), NULL);

  show_octal_tooltips_changed(nautilus_tooltips_preferences, NAUTILUS_TOOLTIP_PREFERENCES_OCTAL);
  g_signal_connect (nautilus_tooltips_preferences, "changed::" NAUTILUS_TOOLTIP_PREFERENCES_OCTAL,
                    G_CALLBACK (show_octal_tooltips_changed), NULL);

/* nautilus_xxx_view_preferences */

  compact_zoom_level_changed(nautilus_compact_view_preferences, NAUTILUS_PREFERENCES_COMPACT_VIEW_DEFAULT_ZOOM_LEVEL);
  g_signal_connect (nautilus_compact_view_preferences, "changed::" NAUTILUS_PREFERENCES_COMPACT_VIEW_DEFAULT_ZOOM_LEVEL,
                    G_CALLBACK (compact_zoom_level_changed), NULL);

  icon_zoom_level_changed(nautilus_icon_view_preferences, NAUTILUS_PREFERENCES_ICON_VIEW_DEFAULT_ZOOM_LEVEL);
  g_signal_connect (nautilus_icon_view_preferences, "changed::" NAUTILUS_PREFERENCES_ICON_VIEW_DEFAULT_ZOOM_LEVEL,
                    G_CALLBACK (icon_zoom_level_changed), NULL);

  list_zoom_level_changed(nautilus_list_view_preferences, NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_ZOOM_LEVEL);
  g_signal_connect (nautilus_list_view_preferences, "changed::" NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_ZOOM_LEVEL,
                    G_CALLBACK (list_zoom_level_changed), NULL);

/* nautilus_window_state */

  start_with_sidebar_changed(nautilus_window_state, NAUTILUS_WINDOW_STATE_START_WITH_SIDEBAR);
  g_signal_connect (nautilus_window_state, "changed::" NAUTILUS_WINDOW_STATE_START_WITH_SIDEBAR,
                    G_CALLBACK (start_with_sidebar_changed), NULL);

  start_with_status_bar_changed(nautilus_window_state, NAUTILUS_WINDOW_STATE_START_WITH_STATUS_BAR);
  g_signal_connect (nautilus_window_state, "changed::" NAUTILUS_WINDOW_STATE_START_WITH_STATUS_BAR,
                    G_CALLBACK (start_with_status_bar_changed), NULL);

  /* Desktop preferences == gnome_background_preferences */
  show_desktop_changed(gnome_background_preferences, NAUTILUS_PREFERENCES_SHOW_DESKTOP);
  g_signal_connect (gnome_background_preferences, "changed::" NAUTILUS_PREFERENCES_SHOW_DESKTOP,
                    G_CALLBACK (show_desktop_changed), NULL);

}

 /* Must not be called before nautilus_global_preferences_init */
_Bool  nautilus_settings_perpetuate(void)
{
    if (!config) {
      g_object_new (NAUTILUS_TYPE_SETTINGS, NULL);
    }
     return NAUTILUS_IS_SETTINGS(config);
}
void nautilus_settings_terminate(void)
{
    if (config) {
      g_object_unref (config);
      config = NULL;
    }
}

