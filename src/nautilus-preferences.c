/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-file-management-properties.c - Functions to create and show the nautilus preference dialog.
 *
 *   Copyright (C) 2002 Jan Arne Petersen
 *
 *   The Gnome Library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 of the
 *   License, or (at your option) any later version.
 *
 *   The Gnome Library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with the Gnome Library; see the file COPYING.LIB.  If not,
 *   write to the Free Software Foundation, Inc., 51 Franklin Street - Suite 500,
 *   Boston, MA 02110-1335, USA.
 *
 *   Authors: Jan Arne Petersen <jpetersen@uni-bonn.de>
 */

#include <config.h>

#include "nautilus-preferences.h"

#include <string.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include <glib/gi18n.h>

#include <eel/eel-glib-extensions.h>
#include <eel/eel-icons.h>
#include <eel/eel-settings.h>

#include <libnautilus-private/nautilus-file-attributes.h>
#include <libnautilus-private/nautilus-icon-info.h>
#include <libnautilus-private/nautilus-file.h>
#include <libnautilus-private/nautilus-column-chooser.h>
#include <libnautilus-private/nautilus-column-utilities.h>
#include <libnautilus-private/nautilus-global-preferences.h>
#include <libnautilus-private/nautilus-module.h>

/* string enum preferences */
#define NAUTILUS_PREFERENCES_DEFAULT_VIEW_WIDGET       "default_view_combobox"
#define NAUTILUS_PREFERENCES_ICON_VIEW_ZOOM_WIDGET     "icon_view_zoom_combobox"
#define NAUTILUS_PREFERENCES_COMPACT_VIEW_ZOOM_WIDGET  "compact_view_zoom_combobox"
#define NAUTILUS_PREFERENCES_LIST_VIEW_ZOOM_WIDGET     "list_view_zoom_combobox"
#define NAUTILUS_PREFERENCES_SORT_ORDER_WIDGET         "sort_order_combobox"
#define NAUTILUS_PREFERENCES_DATE_FORMAT_WIDGET        "date_format_combobox"
#define NAUTILUS_PREFERENCES_PREVIEW_TEXT_WIDGET       "preview_text_combobox"
#define NAUTILUS_PREFERENCES_PREVIEW_IMAGE_WIDGET      "preview_image_combobox"
#define NAUTILUS_PREFERENCES_PREVIEW_FOLDER_WIDGET     "preview_folder_combobox"
#define NAUTILUS_PREFERENCES_SIZE_PREFIXES_WIDGET      "size_prefixes_combobox"

/* bool preferences */
#define NAUTILUS_PREFERENCES_FOLDERS_FIRST_WIDGET                     "sort_folders_first_checkbutton"
#define NAUTILUS_PREFERENCES_COMPACT_LAYOUT_WIDGET                    "compact_layout_checkbutton"
#define NAUTILUS_PREFERENCES_LABELS_BESIDE_ICONS_WIDGET               "labels_beside_icons_checkbutton"
#define NAUTILUS_PREFERENCES_ALL_COLUMNS_SAME_WIDTH                   "all_columns_same_width_checkbutton"
#define NAUTILUS_PREFERENCES_ALWAYS_USE_BROWSER_WIDGET                "always_use_browser_checkbutton"
#define NAUTILUS_PREFERENCES_TRASH_CONFIRM_WIDGET                     "trash_confirm_checkbutton"
#define NAUTILUS_PREFERENCES_TRASH_DELETE_WIDGET                      "trash_delete_checkbutton"
#define NAUTILUS_PREFERENCES_SWAP_TRASH_DELETE_WIDGET                 "swap_trash_binding_checkbutton"
#define NAUTILUS_PREFERENCES_OPEN_NEW_WINDOW_WIDGET                   "new_window_checkbutton"
#define NAUTILUS_PREFERENCES_TREE_VIEW_FOLDERS_WIDGET                 "treeview_folders_checkbutton"
#define NAUTILUS_PREFERENCES_TREE_SHOW_LINES_WIDGET                   "treeview_show_lines_checkbutton"
#define NAUTILUS_PREFERENCES_TREE_ENABLE_HOVER_WIDGET                 "treeview_hover_checkbutton"
#define NAUTILUS_PREFERENCES_TREE_FIXED_HEIGHT_WIDGET                 "treeview_fixed_height_checkbutton"

#define NAUTILUS_PREFERENCES_SHOW_PATH_IN_PANE_TOOLBAR_WIDGET         "show_path_in_pane_checkbutton"
#define NAUTILUS_PREFERENCES_SHOW_UP_ICON_TOOLBAR_WIDGET              "show_up_icon_toolbar_checkbutton"
#define NAUTILUS_PREFERENCES_SHOW_RELOAD_ICON_TOOLBAR_WIDGET          "show_reload_icon_toolbar_checkbutton"
#define NAUTILUS_PREFERENCES_SHOW_EDIT_ICON_TOOLBAR_WIDGET            "show_edit_icon_toolbar_checkbutton"
#define NAUTILUS_PREFERENCES_SHOW_HOME_ICON_TOOLBAR_WIDGET            "show_home_icon_toolbar_checkbutton"
#define NAUTILUS_PREFERENCES_SHOW_COMPUTER_ICON_TOOLBAR_WIDGET        "show_computer_icon_toolbar_checkbutton"
#define NAUTILUS_PREFERENCES_SHOW_CUT_TOOLBAR_WIDGET                  "show_cut_toolbar_checkbutton"
#define NAUTILUS_PREFERENCES_SHOW_COPY_TOOLBAR_WIDGET                 "show_copy_toolbar_checkbutton"
#define NAUTILUS_PREFERENCES_SHOW_PASTE_TOOLBAR_WIDGET                "show_paste_toolbar_checkbutton"
#define NAUTILUS_PREFERENCES_SHOW_SEARCH_ICON_TOOLBAR_WIDGET          "show_search_icon_toolbar_checkbutton"
#define NAUTILUS_PREFERENCES_SHOW_NEW_FOLDER_ICON_TOOLBAR_WIDGET      "show_new_folder_icon_toolbar_checkbutton"
#define NAUTILUS_PREFERENCES_SHOW_LABEL_SEARCH_ICON_TOOLBAR_WIDGET    "show_label_search_icon_toolbar_checkbutton"
#define NAUTILUS_PREFERENCES_SHOW_FULL_PATH_IN_TITLE_BARS_WIDGET      "show_full_path_in_title_bars_checkbutton"
#define NAUTILUS_PREFERENCES_CLOSE_DEVICE_VIEW_ON_EJECT_WIDGET        "close_device_view_on_eject_checkbutton"
#define NAUTILUS_PREFERENCES_AUTOMOUNT_MEDIA_WIDGET                   "media_automount_checkbutton"
#define NAUTILUS_PREFERENCES_AUTOOPEN_MEDIA_WIDGET                    "media_autoopen_checkbutton"
#define NAUTILUS_PREFERENCES_AUTORUN_MEDIA_WIDGET                     "media_autorun_checkbutton"
#define NAUTILUS_PREFERENCES_SHOW_ADVANCED_PERMISSIONS_WIDGET         "show_advanced_permissions_checkbutton"
#define NAUTILUS_PREFERENCES_START_WITH_DUAL_PANE_WIDGET              "start_with_dual_pane_checkbutton"
#define NAUTILUS_PREFERENCES_IGNORE_VIEW_METADATA_WIDGET              "ignore_view_metadata_checkbutton"
#define NAUTILUS_PREFERENCES_BOOKMARKS_IN_TO_MENUS_WIDGET             "bookmarks_in_to_checkbutton"
#define NAUTILUS_PREFERENCES_PLACES_IN_TO_MENUS_WIDGET                "places_in_to_checkbutton"
#define NAUTILUS_PREFERENCES_LIST_VIEW_AUTO_RESIZE_COLUMNS_WIDGET     "auto_resize_columns_checkbutton"
#define NAUTILUS_PREFERENCES_BULK_RENAME_WIDGET                       "bulk_rename_entry"

#define NAUTILUS_TOOLTIP_PREFERENCES_ICON_VIEW_WIDGET                 "tooltips_on_icon_view_checkbutton"
#define NAUTILUS_TOOLTIP_PREFERENCES_LIST_VIEW_WIDGET                 "tooltips_on_list_view_checkbutton"
#define NAUTILUS_TOOLTIP_PREFERENCES_LIST_TIME_OUT_WIDGET             "list_tip_timeout_spin"
#define NAUTILUS_TOOLTIP_PREFERENCES_DESKTOP_WIDGET                   "tooltips_on_desktop_checkbutton"
#define NAUTILUS_TOOLTIP_PREFERENCES_FILE_TYPE_WIDGET                 "tt_show_file_type_checkbutton"
#define NAUTILUS_TOOLTIP_PREFERENCES_MOD_DATE_WIDGET                  "tt_show_modified_date_checkbutton"
#define NAUTILUS_TOOLTIP_PREFERENCES_ACCESS_DATE_WIDGET               "tt_show_created_date_checkbutton"
#define NAUTILUS_TOOLTIP_PREFERENCES_FULL_PATH_WIDGET                 "tt_show_full_path_checkbutton"
#define NAUTILUS_TOOLTIP_PREFERENCES_PERMISSIONS_WIDGET               "tt_show_permissions_checkbutton"
#define NAUTILUS_TOOLTIP_PREFERENCES_OCTAL_WIDGET                     "tt_show_octal_checkbutton"

#define NAUTILUS_TOOLTIP_PREFERENCES_TREE_VIEW_WIDGET                 "tooltips_on_tree_view_checkbutton"
#define NAUTILUS_TOOLTIP_PREFERENCES_TREE_TIME_OUT_WIDGET             "tree_tip_timeout_spin"
#define NAUTILUS_TREETIP_PREFERENCES_FILE_TYPE_WIDGET                 "tr_show_file_type_checkbutton"
#define NAUTILUS_TREETIP_PREFERENCES_MOD_DATE_WIDGET                  "tr_show_modified_date_checkbutton"
#define NAUTILUS_TREETIP_PREFERENCES_ACCESS_DATE_WIDGET               "tr_show_created_date_checkbutton"
#define NAUTILUS_TREETIP_PREFERENCES_PERMISSIONS_WIDGET               "tr_show_permissions_checkbutton"
#define NAUTILUS_TREETIP_PREFERENCES_OCTAL_WIDGET                     "tr_show_octal_checkbutton"


/* int enums */
#define NAUTILUS_PREFERENCES_THUMBNAIL_LIMIT_WIDGET    "preview_image_size_combobox"

#define W(s) (gtk_builder_get_object (builder, s))

static const char * const default_view_values[] = {
    "icon-view",
    "list-view",
    "compact-view",
    NULL
};

static const char * const zoom_values[] = {
    "smallest",
    "smaller",
    "small",
    "standard",
    "large",
    "larger",
    "largest",
    NULL
};

static const char * const sort_order_values[] = {
    "name",
    "size",
    "type",
    "detailed_type",
    "mtime",
    "atime",
    "trash-time",
    NULL
};

static const char * const date_format_values[] = {
    "locale",
    "iso",
    "informal",
    NULL
};

static const char * const preview_values[] = {
    "always",
    "local-only",
    "never",
    NULL
};

static const char * const click_behavior_components[] = {
    "single_click_radiobutton",
    "double_click_radiobutton",
    NULL
};

static const char * const click_behavior_values[] = {
    "single",
    "double",
    NULL
};

static const char * const executable_text_components[] = {
    "scripts_execute_radiobutton",
    "scripts_view_radiobutton",
    "scripts_confirm_radiobutton",
    NULL
};

static const char * const executable_text_values[] = {
    "launch",
    "display",
    "ask",
    NULL
};

static const char * const size_prefixes_values[] = {
    "base-10",
    "base-10-full",
    "base-2",
    "base-2-full",
    NULL
};

static const guint64 thumbnail_limit_values[] = {
    102400,
    512000,
    1048576,
    3145728,
    5242880,
    10485760,
    104857600,
    1073741824,
    2147483648U,
    4294967295U
};

static const char * const icon_captions_components[] = {
    "captions_0_combobox",
    "captions_1_combobox",
    "captions_2_combobox",
    NULL
};

static GtkWidget *preferences_dialog = NULL;

static void
preferences_show_help (GtkWindow *parent,
                       char const *helpfile,
                       char const *sect_id)
{
    GError *error = NULL;
    GtkWidget *dialog;
    char *help_string;

    g_assert (helpfile != NULL);
    g_assert (sect_id != NULL);

    help_string = g_strdup_printf ("help:%s/%s", helpfile, sect_id);

    gtk_show_uri (gtk_window_get_screen (parent),
                  help_string, gtk_get_current_event_time (),
                  &error);
    g_free (help_string);

    if (error) {
        dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_OK,
                                         _("There was an error displaying help: \n%s"),
                                         error->message);

                                         g_signal_connect (G_OBJECT (dialog),
                                                           "response", G_CALLBACK (gtk_widget_destroy),
                                                           NULL);
                                         gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
                                         gtk_widget_show (dialog);
                                         g_error_free (error);
    }
}


static void
nautilus_preferences_dialog_response_cb (GtkDialog *parent,
                                         int response_id,
                                         GtkBuilder *builder)
{
    char *section;

    if (response_id == GTK_RESPONSE_HELP) {
        switch (gtk_notebook_get_current_page (GTK_NOTEBOOK (gtk_builder_get_object (builder, "notebook1")))) {
            default:
            case 0:
                section = "nautilus-views";
                break;
            case 1:
                section = "nautilus-behavior";
                break;
            case 2:
                section = "nautilus-display";
                break;
            case 3:
                section = "nautilus-list";
                break;
            case 4:
                section = "nautilus-preview";
                break;
        }
        preferences_show_help (GTK_WINDOW (parent), "gnome-help", section);
    }
    else if (response_id == GTK_RESPONSE_CLOSE) {
        gtk_widget_destroy (GTK_WIDGET (parent));
    }
}

static void
columns_changed_callback (NautilusColumnChooser *chooser, void *not_used)
{
    char **visible_columns;
    char **column_order;

    nautilus_column_chooser_get_settings (NAUTILUS_COLUMN_CHOOSER (chooser),
                                          &visible_columns,
                                          &column_order);

    eel_settings_set_strv (nautilus_list_view_preferences,
                         NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_VISIBLE_COLUMNS,
                         (const char * const *)visible_columns);

    eel_settings_set_strv (nautilus_list_view_preferences,
                         NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_COLUMN_ORDER,
                         (const char * const *)column_order);

    g_strfreev (visible_columns);
    g_strfreev (column_order);
}

static void
free_column_names_array (GPtrArray *column_names)
{
    g_ptr_array_foreach (column_names, (GFunc) g_free, NULL);
    g_ptr_array_free (column_names, TRUE);
}

static void
create_icon_caption_combo_box_items (GtkComboBoxText *combo_box,
                                     GList *columns)
{
    GList *l;
    GPtrArray *column_names;

    column_names = g_ptr_array_new ();

    /* Translators: this is referred to captions under icons. */
    gtk_combo_box_text_append_text (combo_box, _("None"));
    g_ptr_array_add (column_names, g_strdup ("none"));

    for (l = columns; l != NULL; l = l->next) {
        NautilusColumn *column;
        char *name;
        char *label;

        column = NAUTILUS_COLUMN (l->data);

        g_object_get (G_OBJECT (column),
                      "name", &name, "label", &label,
                      NULL);

        /* Don't show name here, it doesn't make sense */
        if (!strcmp (name, "name")) {
            g_free (name);
            g_free (label);
            continue;
        }

        gtk_combo_box_text_append_text (combo_box, label);
        g_ptr_array_add (column_names, name);

        g_free (label);
    }
    g_object_set_data_full (G_OBJECT (combo_box), "column_names",
                                      column_names,
                                      (GDestroyNotify) free_column_names_array);
}

static void
icon_captions_changed_callback (GtkComboBox *combo_box,
                                void * user_data)
{
    GPtrArray *captions;
    GtkBuilder *builder;
    int i;

    builder = GTK_BUILDER (user_data);

    captions = g_ptr_array_new ();

    for (i = 0; icon_captions_components[i] != NULL; i++) {
        GtkWidget *combo_box;
        int active;
        GPtrArray *column_names;
        char *name;

        combo_box = GTK_WIDGET (gtk_builder_get_object
        (builder, icon_captions_components[i]));
        active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box));

        column_names = g_object_get_data (G_OBJECT (combo_box),
                                          "column_names");

        name = g_ptr_array_index (column_names, active);
        g_ptr_array_add (captions, name);
    }
    g_ptr_array_add (captions, NULL);

    eel_settings_set_strv (nautilus_icon_view_preferences,
                         NAUTILUS_PREFERENCES_ICON_VIEW_CAPTIONS,
                         (const char **)captions->pdata);
    g_ptr_array_free (captions, TRUE);
}

static void
update_caption_combo_box (GtkBuilder *builder,
                          const char *combo_box_name,
                          const char *name)
{
    GtkWidget *combo_box;
    int i;
    GPtrArray *column_names;

    combo_box = GTK_WIDGET (gtk_builder_get_object (builder, combo_box_name));

    g_signal_handlers_block_by_func
    (combo_box,
     G_CALLBACK (icon_captions_changed_callback),
     builder);

    column_names = g_object_get_data (G_OBJECT (combo_box),
                                      "column_names");

    for (i = 0; i < column_names->len; ++i) {
        if (!strcmp (name, g_ptr_array_index (column_names, i))) {
            gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), i);
            break;
        }
    }

    g_signal_handlers_unblock_by_func
    (combo_box,
     G_CALLBACK (icon_captions_changed_callback),
     builder);
}

static void
update_icon_captions_from_settings (GtkBuilder *builder)
{
    char **captions;
    int i, j;

    captions = eel_settings_get_strv (nautilus_icon_view_preferences, NAUTILUS_PREFERENCES_ICON_VIEW_CAPTIONS);
    if (captions == NULL)
        return;

    for (i = 0, j = 0;
         icon_captions_components[i] != NULL;
    i++) {
        char *data;

        if (captions[j]) {
            data = captions[j];
            ++j;
        } else {
            data = "none";
        }

        update_caption_combo_box (builder,
                                  icon_captions_components[i],
                                  data);
    }

    g_strfreev (captions);
}

static void
nautilus_preferences_dialog_setup_icon_caption_page (GtkBuilder *builder)
{
    GList *columns;
    int i;
    _Bool writable;

    writable = g_settings_is_writable (nautilus_icon_view_preferences,
                                       NAUTILUS_PREFERENCES_ICON_VIEW_CAPTIONS);

    columns = nautilus_get_common_columns ();

    for (i = 0; icon_captions_components[i] != NULL; i++) {
        GtkWidget *combo_box;

        combo_box = GTK_WIDGET (gtk_builder_get_object (builder,
                                                        icon_captions_components[i]));

        create_icon_caption_combo_box_items (GTK_COMBO_BOX_TEXT (combo_box), columns);
        gtk_widget_set_sensitive (combo_box, writable);

        g_signal_connect (combo_box, "changed",
                          G_CALLBACK (icon_captions_changed_callback),
                          builder);
    }

    nautilus_column_list_free (columns);

    update_icon_captions_from_settings (builder);
}

static void
create_date_format_menu (GtkBuilder *builder)
{
    GtkComboBoxText *combo_box;
    char *date_string;
    time_t now_raw;
    struct tm* now;

    combo_box = GTK_COMBO_BOX_TEXT
    (gtk_builder_get_object (builder,
                             NAUTILUS_PREFERENCES_DATE_FORMAT_WIDGET));

    now_raw = time (NULL);
    now = localtime (&now_raw);

    date_string = eel_strdup_strftime ("%c", now);
    gtk_combo_box_text_append_text (combo_box, date_string);
    g_free (date_string);

    date_string = eel_strdup_strftime ("%Y-%m-%d %H:%M:%S", now);
    gtk_combo_box_text_append_text (combo_box, date_string);
    g_free (date_string);

    date_string = eel_strdup_strftime (_("today at %-I:%M:%S %p"), now);
    gtk_combo_box_text_append_text (combo_box, date_string);
    g_free (date_string);
}

static void
set_columns_from_settings (NautilusColumnChooser *chooser)
{
    char **visible_columns;
    char **column_order;

    visible_columns = eel_settings_get_strv (nautilus_list_view_preferences,
                                           NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_VISIBLE_COLUMNS);
    column_order = eel_settings_get_strv (nautilus_list_view_preferences,
                                        NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_COLUMN_ORDER);

    nautilus_column_chooser_set_settings (NAUTILUS_COLUMN_CHOOSER (chooser),
                                          visible_columns,
                                          column_order);

    g_strfreev (visible_columns);
    g_strfreev (column_order);
}

static void
use_default_callback (NautilusColumnChooser *chooser,
                      void * user_data)
{
    g_settings_reset (nautilus_list_view_preferences,
                      NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_VISIBLE_COLUMNS);
    g_settings_reset (nautilus_list_view_preferences,
                      NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_COLUMN_ORDER);
    set_columns_from_settings (chooser);
}

static void
nautilus_preferences_dialog_setup_list_column_page (GtkBuilder *builder)
{
    GtkWidget *chooser;
    GtkWidget *box;

    chooser = nautilus_column_chooser_new (NULL);

    set_columns_from_settings (NAUTILUS_COLUMN_CHOOSER (chooser));

    g_signal_connect (chooser, "changed",
                      G_CALLBACK (columns_changed_callback), chooser);

    g_signal_connect (chooser, "use_default",
                      G_CALLBACK (use_default_callback), chooser);

    gtk_widget_show (chooser);
    box = GTK_WIDGET (gtk_builder_get_object (builder, "list_columns_vbox"));

    gtk_box_pack_start (GTK_BOX (box), chooser, TRUE, TRUE, 0);
}

static void
bind_builder_bool (GtkBuilder *builder,
                   GSettings  *settings,
                   const char *widget_name,
                   const char *prefs)
{
    g_settings_bind (settings, prefs,
                     gtk_builder_get_object (builder, widget_name),
                     "active", G_SETTINGS_BIND_DEFAULT);
}

static int
round_double_val(double val)
{
 int ival = (int)val;

 return ival + (val - ival > 0.5);
}

static GVariant *
spin_set_mapping (const GValue       *value,
                  const GVariantType *expected_type,
                  void                *user_data)
{
  int new_value = round_double_val (g_value_get_double (value));
  return g_variant_new_int32 (new_value);
}

static void
bind_builder_spin (GtkBuilder *builder,
                   GSettings  *settings,
                   const char *widget_name,
                   const char *prefs)
{
    g_settings_bind_with_mapping (settings, prefs,
                                  gtk_builder_get_object (builder, widget_name),
                                  "value", G_SETTINGS_BIND_DEFAULT,
                                  NULL,
                                  spin_set_mapping,
                                  NULL, NULL);

}

static void
bind_builder_bool_inverted (GtkBuilder *builder,
                            GSettings *settings,
                            const char *widget_name,
                            const char *prefs)
{
    g_settings_bind (settings, prefs,
                     gtk_builder_get_object (builder, widget_name),
                     "active", G_SETTINGS_BIND_INVERT_BOOLEAN);
}

static void
bind_builder_string_entry (GtkBuilder *builder,
                           GSettings *settings,
                           const char *widget_name,
                           const char *prefs)
{
    g_settings_bind (settings, prefs,
                     gtk_builder_get_object (builder, widget_name),
                     "text", G_SETTINGS_BIND_DEFAULT);
}

/* Is really _Bool but glib errently defines gboolean as int */
static int
enum_get_mapping (GValue *value, GVariant *variant, void *user_data)
{
    const char **enum_values = user_data;
    const char *str;
    int i;

    str = g_variant_get_string (variant, NULL);
    for (i = 0; enum_values[i] != NULL; i++) {
        if (strcmp (enum_values[i], str) == 0) {
            g_value_set_int (value, i);
            return TRUE;
        }
    }

    return FALSE;
}

static GVariant *
enum_set_mapping (const GValue       *value,
                  const GVariantType *expected_type,
                  void                *user_data)
{
    const char **enum_values = user_data;

    return g_variant_new_string (enum_values[g_value_get_int (value)]);
}

static void
bind_builder_enum (GtkBuilder *builder,
                   GSettings  *settings,
                   const char *widget_name,
                   const char *prefs,
                   const char **enum_values)
{
    g_settings_bind_with_mapping (settings, prefs,
                                  gtk_builder_get_object (builder, widget_name),
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  enum_get_mapping,
                                  enum_set_mapping,
                                  enum_values, NULL);
}


typedef struct {
    const guint64 *values;
    int n_values;
} UIntEnumBinding;

/* Is really _Bool but glib errently defines gboolean as int */
static int
uint_enum_get_mapping (GValue             *value,
                       GVariant           *variant,
                       void                *user_data)
{
    UIntEnumBinding *binding = user_data;
    guint64 v;
    int i;

    v = g_variant_get_uint64 (variant);
    for (i = 0; i < binding->n_values; i++) {
        if (binding->values[i] >= v) {
            g_value_set_int (value, i);
            return TRUE;
        }
    }

    return FALSE;
}

static GVariant *
uint_enum_set_mapping (const GValue       *value,
                       const GVariantType *expected_type,
                       void                *user_data)
{
    UIntEnumBinding *binding = user_data;

    return g_variant_new_uint64 (binding->values[g_value_get_int (value)]);
}

static void
bind_builder_uint_enum (GtkBuilder *builder,
                        GSettings *settings,
                        const char *widget_name,
                        const char *prefs,
                        const guint64 *values,
                        int n_values)
{
    UIntEnumBinding *binding;

    binding = g_new (UIntEnumBinding, 1);
    binding->values = values;
    binding->n_values = n_values;
    g_settings_bind_with_mapping (settings, prefs,
                                  gtk_builder_get_object (builder, widget_name),
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  uint_enum_get_mapping,
                                  uint_enum_set_mapping,
                                  binding, g_free);
}

static GVariant *
radio_mapping_set (const GValue *gvalue, const GVariantType *expected_type, void *user_data)
{
    const char *widget_value = user_data;
    GVariant *retval = NULL;

    if (g_value_get_boolean (gvalue)) {
        retval = g_variant_new_string (widget_value);
    }

    return retval;
}

/* Is really _Bool but glib errently defines gboolean as int */
static int
radio_mapping_get (GValue *gvalue, GVariant *variant, void *user_data)
{
    const char *widget_value = user_data;
    const char *value;

    value = g_variant_get_string (variant, NULL);

    if (g_strcmp0 (value, widget_value) == 0) {
        g_value_set_boolean (gvalue, TRUE);
    } else {
        g_value_set_boolean (gvalue, FALSE);
    }

    return TRUE;
}

static void
bind_builder_radio (GtkBuilder *builder,
                    GSettings *settings,
                    const char **widget_names,
                    const char *prefs,
                    const char **values)
{
    GtkWidget *button;
    int i;

    for (i = 0; widget_names[i] != NULL; i++) {
        button = GTK_WIDGET (gtk_builder_get_object (builder, widget_names[i]));
        g_settings_bind_with_mapping (settings, prefs,
                                      button, "active",
                                      G_SETTINGS_BIND_DEFAULT,
                                      radio_mapping_get, radio_mapping_set,
                                      (void *) values[i], NULL);
    }
}

static void
setup_tooltip_items (GtkBuilder *builder)
{
    _Bool enabled = FALSE;

    enabled =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (W (NAUTILUS_TOOLTIP_PREFERENCES_ICON_VIEW_WIDGET))) ||
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (W (NAUTILUS_TOOLTIP_PREFERENCES_LIST_VIEW_WIDGET))) ||
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (W (NAUTILUS_TOOLTIP_PREFERENCES_DESKTOP_WIDGET)));

    gtk_widget_set_sensitive (GTK_WIDGET (W (NAUTILUS_TOOLTIP_PREFERENCES_FILE_TYPE_WIDGET)), enabled);
    gtk_widget_set_sensitive (GTK_WIDGET (W (NAUTILUS_TOOLTIP_PREFERENCES_MOD_DATE_WIDGET)), enabled);
    gtk_widget_set_sensitive (GTK_WIDGET (W (NAUTILUS_TOOLTIP_PREFERENCES_ACCESS_DATE_WIDGET)), enabled);
    gtk_widget_set_sensitive (GTK_WIDGET (W (NAUTILUS_TOOLTIP_PREFERENCES_FULL_PATH_WIDGET)), enabled);
    gtk_widget_set_sensitive (GTK_WIDGET (W (NAUTILUS_TOOLTIP_PREFERENCES_PERMISSIONS_WIDGET)), enabled);
    gtk_widget_set_sensitive (GTK_WIDGET (W (NAUTILUS_TOOLTIP_PREFERENCES_OCTAL_WIDGET)), enabled);

    enabled =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (W (NAUTILUS_TOOLTIP_PREFERENCES_LIST_VIEW_WIDGET)));

    gtk_widget_set_sensitive (GTK_WIDGET (W (NAUTILUS_TOOLTIP_PREFERENCES_LIST_TIME_OUT_WIDGET)), enabled);
}

static void
connect_tooltip_items (GtkBuilder *builder)
{
    GtkToggleButton *w;

    w = GTK_TOGGLE_BUTTON (W (NAUTILUS_TOOLTIP_PREFERENCES_ICON_VIEW_WIDGET));
    g_signal_connect_swapped (w, "toggled", G_CALLBACK (setup_tooltip_items), builder);

    w = GTK_TOGGLE_BUTTON (W (NAUTILUS_TOOLTIP_PREFERENCES_LIST_VIEW_WIDGET));
    g_signal_connect_swapped (w, "toggled", G_CALLBACK (setup_tooltip_items), builder);

    w = GTK_TOGGLE_BUTTON (W (NAUTILUS_TOOLTIP_PREFERENCES_DESKTOP_WIDGET));
    g_signal_connect_swapped (w, "toggled", G_CALLBACK (setup_tooltip_items), builder);

}

static void
setup_treetip_items (GtkBuilder *builder)
{
    _Bool enabled = FALSE;

    enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (W (NAUTILUS_TOOLTIP_PREFERENCES_TREE_VIEW_WIDGET)));

    gtk_widget_set_sensitive (GTK_WIDGET (W (NAUTILUS_TOOLTIP_PREFERENCES_TREE_TIME_OUT_WIDGET)), enabled);
    gtk_widget_set_sensitive (GTK_WIDGET (W (NAUTILUS_TREETIP_PREFERENCES_FILE_TYPE_WIDGET)), enabled);
    gtk_widget_set_sensitive (GTK_WIDGET (W (NAUTILUS_TREETIP_PREFERENCES_MOD_DATE_WIDGET)), enabled);
    gtk_widget_set_sensitive (GTK_WIDGET (W (NAUTILUS_TREETIP_PREFERENCES_ACCESS_DATE_WIDGET)), enabled);
    gtk_widget_set_sensitive (GTK_WIDGET (W (NAUTILUS_TREETIP_PREFERENCES_PERMISSIONS_WIDGET)), enabled);
    gtk_widget_set_sensitive (GTK_WIDGET (W (NAUTILUS_TREETIP_PREFERENCES_OCTAL_WIDGET)), enabled);
}

static void
connect_treetip_items (GtkBuilder *builder)
{
    GtkToggleButton *w;

    w = GTK_TOGGLE_BUTTON (W (NAUTILUS_TOOLTIP_PREFERENCES_TREE_VIEW_WIDGET));
    g_signal_connect_swapped (w, "toggled", G_CALLBACK (setup_treetip_items), builder);

}
static void
nautilus_preferences_size_group_create (GtkBuilder *builder, char *prefix, int items)
{
    GtkSizeGroup *size_group;
    int i;
    char *item_name;
    GtkWidget *widget;

    size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    for (i = 0; i < items; i++) {
        item_name = g_strdup_printf ("%s_%d", prefix, i);
        widget = GTK_WIDGET (gtk_builder_get_object (builder, item_name));
        gtk_size_group_add_widget (size_group, widget);
        g_free (item_name);
    }
    g_object_unref (G_OBJECT (size_group));
}

static  void
nautilus_preferences_dialog_setup (GtkBuilder *builder, GtkWindow *window)
{
    GtkWidget *dialog;

    /* setup UI */
    nautilus_preferences_size_group_create (builder, "views_label", 5);
    nautilus_preferences_size_group_create (builder, "captions_label", 3);
    nautilus_preferences_size_group_create (builder,  "preview_label", 4);
    create_date_format_menu (builder);


    /* nautilus patch */
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_PATH_IN_PANE_TOOLBAR_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_PATH_IN_PANE);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_UP_ICON_TOOLBAR_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_UP_ICON_TOOLBAR);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_RELOAD_ICON_TOOLBAR_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_RELOAD_ICON_TOOLBAR);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_EDIT_ICON_TOOLBAR_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_EDIT_ICON_TOOLBAR);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_HOME_ICON_TOOLBAR_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_HOME_ICON_TOOLBAR);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_COMPUTER_ICON_TOOLBAR_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_COMPUTER_ICON_TOOLBAR);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_CUT_TOOLBAR_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_CUT_TOOLBAR);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_COPY_TOOLBAR_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_COPY_TOOLBAR);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_PASTE_TOOLBAR_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_PASTE_TOOLBAR);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_SEARCH_ICON_TOOLBAR_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_SEARCH_ICON_TOOLBAR);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_LABEL_SEARCH_ICON_TOOLBAR_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_LABEL_SEARCH_ICON_TOOLBAR);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_NEW_FOLDER_ICON_TOOLBAR_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_NEW_FOLDER_ICON_TOOLBAR);

    /* setup preferences */
    bind_builder_bool (builder, nautilus_icon_view_preferences,
                       NAUTILUS_PREFERENCES_COMPACT_LAYOUT_WIDGET,
                       NAUTILUS_PREFERENCES_ICON_VIEW_DEFAULT_USE_TIGHTER_LAYOUT);
    bind_builder_bool (builder, nautilus_icon_view_preferences,
                       NAUTILUS_PREFERENCES_LABELS_BESIDE_ICONS_WIDGET,
                       NAUTILUS_PREFERENCES_ICON_VIEW_LABELS_BESIDE_ICONS);
    bind_builder_bool (builder, nautilus_compact_view_preferences,
                       NAUTILUS_PREFERENCES_ALL_COLUMNS_SAME_WIDTH,
                       NAUTILUS_PREFERENCES_COMPACT_VIEW_ALL_COLUMNS_SAME_WIDTH);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_FOLDERS_FIRST_WIDGET,
                       NAUTILUS_PREFERENCES_SORT_DIRECTORIES_FIRST);
    bind_builder_bool_inverted (builder, nautilus_preferences,
                                NAUTILUS_PREFERENCES_ALWAYS_USE_BROWSER_WIDGET,
                                NAUTILUS_PREFERENCES_ALWAYS_USE_BROWSER);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_TRASH_CONFIRM_WIDGET,
                       NAUTILUS_PREFERENCES_CONFIRM_TRASH);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_TRASH_DELETE_WIDGET,
                       NAUTILUS_PREFERENCES_ENABLE_DELETE);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SWAP_TRASH_DELETE_WIDGET,
                       NAUTILUS_PREFERENCES_SWAP_TRASH_DELETE);
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_FULL_PATH_IN_TITLE_BARS_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_FULL_PATH_TITLES);
    bind_builder_bool (builder, nautilus_tree_sidebar_preferences,
                       NAUTILUS_PREFERENCES_TREE_VIEW_FOLDERS_WIDGET,
                       NAUTILUS_PREFERENCES_TREE_SHOW_ONLY_DIRECTORIES);
    bind_builder_bool (builder, nautilus_tree_sidebar_preferences,
                       NAUTILUS_PREFERENCES_TREE_SHOW_LINES_WIDGET,
                       NAUTILUS_PREFERENCES_TREE_SHOW_LINES);
    bind_builder_bool (builder, nautilus_tree_sidebar_preferences,
                       NAUTILUS_PREFERENCES_TREE_ENABLE_HOVER_WIDGET,
                       NAUTILUS_PREFERENCES_TREE_ENABLE_HOVER);
    bind_builder_bool (builder, nautilus_tree_sidebar_preferences,
                       NAUTILUS_PREFERENCES_TREE_FIXED_HEIGHT_WIDGET,
                       NAUTILUS_PREFERENCES_TREE_FIXED_HEIGHT);

    bind_builder_enum (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DEFAULT_VIEW_WIDGET,
                       NAUTILUS_PREFERENCES_DEFAULT_FOLDER_VIEWER,
                       (const char **) default_view_values);
    bind_builder_enum (builder, nautilus_icon_view_preferences,
                       NAUTILUS_PREFERENCES_ICON_VIEW_ZOOM_WIDGET,
                       NAUTILUS_PREFERENCES_ICON_VIEW_DEFAULT_ZOOM_LEVEL,
                       (const char **) zoom_values);
    bind_builder_enum (builder, nautilus_compact_view_preferences,
                       NAUTILUS_PREFERENCES_COMPACT_VIEW_ZOOM_WIDGET,
                       NAUTILUS_PREFERENCES_COMPACT_VIEW_DEFAULT_ZOOM_LEVEL,
                       (const char **) zoom_values);
    bind_builder_enum (builder, nautilus_list_view_preferences,
                       NAUTILUS_PREFERENCES_LIST_VIEW_ZOOM_WIDGET,
                       NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_ZOOM_LEVEL,
                       (const char **) zoom_values);
    bind_builder_enum (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SORT_ORDER_WIDGET,
                       NAUTILUS_PREFERENCES_DEFAULT_SORT_ORDER,
                       (const char **) sort_order_values);
    bind_builder_enum (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_PREVIEW_TEXT_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_TEXT_IN_ICONS,
                       (const char **) preview_values);
    bind_builder_enum (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_PREVIEW_IMAGE_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_IMAGE_FILE_THUMBNAILS,
                       (const char **) preview_values);
    bind_builder_enum (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_PREVIEW_FOLDER_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_DIRECTORY_ITEM_COUNTS,
                       (const char **) preview_values);
    bind_builder_enum (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SIZE_PREFIXES_WIDGET,
                       NAUTILUS_PREFERENCES_SIZE_PREFIXES,
                       (const char **) size_prefixes_values);
    bind_builder_enum (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_DATE_FORMAT_WIDGET,
                       NAUTILUS_PREFERENCES_DATE_FORMAT,
                       (const char **) date_format_values);

    bind_builder_radio (builder, nautilus_preferences,
                        (const char **) click_behavior_components,
                        NAUTILUS_PREFERENCES_CLICK_POLICY,
                        (const char **) click_behavior_values);
    bind_builder_radio (builder, nautilus_preferences,
                        (const char **) executable_text_components,
                        NAUTILUS_PREFERENCES_EXECUTABLE_TEXT_ACTIVATION,
                        (const char **) executable_text_values);

    bind_builder_uint_enum (builder, nautilus_preferences,
                            NAUTILUS_PREFERENCES_THUMBNAIL_LIMIT_WIDGET,
                            NAUTILUS_PREFERENCES_IMAGE_FILE_THUMBNAIL_LIMIT,
                            thumbnail_limit_values,
                            G_N_ELEMENTS (thumbnail_limit_values));
#if HAVE_GNOME_DESKTOP
    bind_builder_bool (builder, gnome_media_handling_preferences,
                       NAUTILUS_PREFERENCES_AUTOMOUNT_MEDIA_WIDGET,
                       GNOME_DESKTOP_MEDIA_HANDLING_AUTOMOUNT);

    bind_builder_bool (builder, gnome_media_handling_preferences,
                       NAUTILUS_PREFERENCES_AUTOOPEN_MEDIA_WIDGET,
                       GNOME_DESKTOP_MEDIA_HANDLING_AUTOMOUNT_OPEN);

    bind_builder_bool_inverted (builder, gnome_media_handling_preferences,
                                NAUTILUS_PREFERENCES_AUTORUN_MEDIA_WIDGET,
                                GNOME_DESKTOP_MEDIA_HANDLING_AUTORUN);
#endif
    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_CLOSE_DEVICE_VIEW_ON_EJECT_WIDGET,
                       NAUTILUS_PREFERENCES_CLOSE_DEVICE_VIEW_ON_EJECT);

    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_SHOW_ADVANCED_PERMISSIONS_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_ADVANCED_PERMISSIONS);

    bind_builder_bool (builder, nautilus_list_view_preferences,
                       NAUTILUS_PREFERENCES_LIST_VIEW_AUTO_RESIZE_COLUMNS_WIDGET,
                       NAUTILUS_PREFERENCES_LIST_VIEW_AUTO_RESIZE_COLUMNS);

    bind_builder_string_entry (builder, nautilus_preferences,
                               NAUTILUS_PREFERENCES_BULK_RENAME_WIDGET,
                               NAUTILUS_PREFERENCES_BULK_RENAME_TOOL);

    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_START_WITH_DUAL_PANE_WIDGET,
                       NAUTILUS_PREFERENCES_START_WITH_DUAL_PANE);

    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_IGNORE_VIEW_METADATA_WIDGET,
                       NAUTILUS_PREFERENCES_IGNORE_VIEW_METADATA);

    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_BOOKMARKS_IN_TO_MENUS_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_BOOKMARKS_IN_TO_MENUS);

    bind_builder_bool (builder, nautilus_preferences,
                       NAUTILUS_PREFERENCES_PLACES_IN_TO_MENUS_WIDGET,
                       NAUTILUS_PREFERENCES_SHOW_PLACES_IN_TO_MENUS);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TOOLTIP_PREFERENCES_DESKTOP_WIDGET,
                       NAUTILUS_TOOLTIP_PREFERENCES_DESKTOP);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TOOLTIP_PREFERENCES_ICON_VIEW_WIDGET,
                       NAUTILUS_TOOLTIP_PREFERENCES_ICON_VIEW);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TOOLTIP_PREFERENCES_LIST_VIEW_WIDGET,
                       NAUTILUS_TOOLTIP_PREFERENCES_LIST_VIEW);

    bind_builder_spin (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TOOLTIP_PREFERENCES_LIST_TIME_OUT_WIDGET,
                       NAUTILUS_TOOLTIP_PREFERENCES_LIST_TIME_OUT);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TOOLTIP_PREFERENCES_FILE_TYPE_WIDGET,
                       NAUTILUS_TOOLTIP_PREFERENCES_FILE_TYPE);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TOOLTIP_PREFERENCES_MOD_DATE_WIDGET,
                       NAUTILUS_TOOLTIP_PREFERENCES_MOD_DATE);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TOOLTIP_PREFERENCES_ACCESS_DATE_WIDGET,
                       NAUTILUS_TOOLTIP_PREFERENCES_ACCESS_DATE);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TOOLTIP_PREFERENCES_FULL_PATH_WIDGET,
                       NAUTILUS_TOOLTIP_PREFERENCES_FULL_PATH);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TOOLTIP_PREFERENCES_PERMISSIONS_WIDGET,
                       NAUTILUS_TOOLTIP_PREFERENCES_PERMISSIONS);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TOOLTIP_PREFERENCES_OCTAL_WIDGET,
                       NAUTILUS_TOOLTIP_PREFERENCES_OCTAL);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TOOLTIP_PREFERENCES_TREE_VIEW_WIDGET,
                       NAUTILUS_TOOLTIP_PREFERENCES_TREE_VIEW);

    bind_builder_spin (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TOOLTIP_PREFERENCES_TREE_TIME_OUT_WIDGET,
                       NAUTILUS_TOOLTIP_PREFERENCES_TREE_TIME_OUT);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TREETIP_PREFERENCES_FILE_TYPE_WIDGET,
                       NAUTILUS_TREETIP_PREFERENCES_FILE_TYPE);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TREETIP_PREFERENCES_MOD_DATE_WIDGET,
                       NAUTILUS_TREETIP_PREFERENCES_MOD_DATE);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TREETIP_PREFERENCES_ACCESS_DATE_WIDGET,
                       NAUTILUS_TREETIP_PREFERENCES_ACCESS_DATE);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TREETIP_PREFERENCES_PERMISSIONS_WIDGET,
                       NAUTILUS_TREETIP_PREFERENCES_PERMISSIONS);

    bind_builder_bool (builder, nautilus_tooltips_preferences,
                       NAUTILUS_TREETIP_PREFERENCES_OCTAL_WIDGET,
                       NAUTILUS_TREETIP_PREFERENCES_OCTAL);

    setup_tooltip_items (builder);
    connect_tooltip_items (builder);
    setup_treetip_items (builder);
    connect_treetip_items (builder);

    nautilus_preferences_dialog_setup_icon_caption_page (builder);
    nautilus_preferences_dialog_setup_list_column_page (builder);

    /* UI callbacks */
    dialog = GTK_WIDGET (gtk_builder_get_object (builder, "nautilus_preferences_dialog"));
    g_signal_connect_data (dialog, "response",
                           G_CALLBACK (nautilus_preferences_dialog_response_cb),
                           g_object_ref (builder),
                           (GClosureNotify)g_object_unref,
                           0);
    g_signal_connect (dialog, "delete-event",
                      G_CALLBACK (gtk_widget_destroy), NULL);

    gtk_window_set_icon_name (GTK_WINDOW (dialog), "folder");

    if (window) {
        gtk_window_set_screen (GTK_WINDOW (dialog), gtk_window_get_screen(window));
    }

    preferences_dialog = dialog;
    g_object_add_weak_pointer (G_OBJECT (dialog), (void * *) &preferences_dialog);
    gtk_widget_show (dialog);
}

void
nautilus_preferences_dialog_show (GtkWindow *window)
{
    GtkBuilder *builder;

    if (preferences_dialog != NULL) {
        gtk_window_present (GTK_WINDOW (preferences_dialog));
        return;
    }

    builder = gtk_builder_new ();
    gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);

    gtk_builder_add_from_resource (builder,
                                   "/apps/nautilus/nautilus-preferences-ui.xml",
                                   NULL);

    nautilus_preferences_dialog_setup (builder, window);

    g_object_unref (builder);
}
