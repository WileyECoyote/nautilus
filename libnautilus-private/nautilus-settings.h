/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 *   eel-tooltips.h: Make GTK better.
 *
 * Copyright (C) 2014 Wiley Edward Hill wileyhill@gmail.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __NAUTILUS_SETTINGS_H__
#define __NAUTILUS_SETTINGS_H__

#define NAUTILUS_TYPE_SETTINGS              (nautilus_settings_get_type ())
#define NAUTILUS_SETTINGS(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), NAUTILUS_TYPE_SETTINGS, NautilusSettings))
#define NAUTILUS_SETTINGS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), NAUTILUS_TYPE_SETTINGS, NautilusSettingsClass))
#define NAUTILUS_IS_SETTINGS(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NAUTILUS_TYPE_SETTINGS))
#define NAUTILUS_IS_SETTINGS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), NAUTILUS_TYPE_SETTINGS))
#define NAUTILUS_SETTINGS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EEL_TYPE_SETTINGS, NautilusSettingsClass))

typedef struct _NautilusSettings      NautilusSettings;
typedef struct _NautilusSettingsClass NautilusSettingsClass;

struct _NautilusSettings
{
  GObject parent;

  _Bool            always_use_browser;
  _Bool            click_policy;
  _Bool            desktop_is_home_dir;
  _Bool            enable_delete;
  _Bool            ignore_view_metadata;
  _Bool            show_full_path;
  _Bool            show_hidden;

  _Bool            directories_first;
   int             sort_order;
  _Bool            sort_reversed;

  _Bool            start_with_sidebar;
  _Bool            start_with_status_bar;

   const char     *folder_viewer;

   int             size_prefixes;

   int             compact_zoom_level;
   int             icon_zoom_level;
   int             list_zoom_level;

  _Bool            show_desktop;

  _Bool            tooltips_desktop;
  _Bool            tooltips_icon_view;

  _Bool            tooltips_file_type;
  _Bool            tooltips_mod_date;
  _Bool            tooltips_access_date;
  _Bool            tooltips_show_path;
  _Bool            tooltips_show_missions;
  _Bool            tooltips_show_octal;

   //int             tip_timeout;
   //unsigned long   tip_timeout_id;

};

struct _NautilusSettingsClass
{
  GObjectClass parent_class;

};

_Bool  nautilus_settings_perpetuate(void);    /* called by nautilus_application_startup */
void   nautilus_settings_terminate(void);    /* called by nautilus_application_finalize */

/* nautilus_preferences */
     _Bool    nautilus_settings_get_always_use_browser     (void);
     _Bool    nautilus_settings_get_click_policy           (void);
     _Bool    nautilus_settings_get_enable_delete          (void);
     _Bool    nautilus_settings_get_desktop_is_home_dir    (void);
     _Bool    nautilus_settings_get_ignore_view_metadata   (void);
     _Bool    nautilus_settings_get_show_full_path         (void);
     _Bool    nautilus_settings_get_show_hidden            (void);
const char   *nautilus_settings_get_default_folder_viewer  (void);
      int     nautilus_settings_get_size_prefixes          (void);
     _Bool    nautilus_settings_get_sort_directories_first (void);
      int     nautilus_settings_get_sort_order             (void);
     _Bool    nautilus_settings_get_sort_reversed          (void);

/* nautilus_tooltips_preferences */
     _Bool    nautilus_settings_get_show_desktop_tooltips  (void);
     _Bool    nautilus_settings_get_show_icon_tooltips     (void);

     _Bool    nautilus_settings_get_show_file_type_tooltip   (void);
     _Bool    nautilus_settings_get_show_mod_date_tooltip    (void);
     _Bool    nautilus_settings_get_show_access_date_tooltip (void);
     _Bool    nautilus_settings_get_show_path_tooltip        (void);
     _Bool    nautilus_settings_get_show_permissions_tooltip (void);
     _Bool    nautilus_settings_get_show_octal_tooltip       (void);

/* nautilus_xxx_view_preferences */
      int     nautilus_settings_get_compact_zoom_level     (void);
      int     nautilus_settings_get_icon_zoom_level        (void);
      int     nautilus_settings_get_list_zoom_level        (void);

/* nautilus_window_state */
     _Bool    nautilus_settings_get_start_with_sidebar     (void);
     _Bool    nautilus_settings_get_start_with_status_bar  (void);

/* gnome_background_preferences */
     _Bool    nautilus_settings_get_show_desktop           (void);

/* nautilus_list_view_preferences */
     char   **nautilus_settings_default_column_order      (void);
     char   **nautilus_settings_default_visible_columns   (void);


#endif /* __NAUTILUS_SETTINGS_H__ */

