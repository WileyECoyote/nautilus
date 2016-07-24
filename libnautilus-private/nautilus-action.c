/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street - Suite 500,
   Boston, MA 02110-1335, USA.

*/

#include <glib.h>
#include <gtk/gtk.h>

#include <eel/eel-icons.h>
#include <eel/eel-settings.h>
#include <eel/eel-string.h>

#include <nautilus-file-attributes.h>
#include <nautilus-icon-info.h>
#include "nautilus-file.h"
#include "nautilus-action.h"
#include "nautilus-file-utilities.h"

#include <glib/gi18n.h>

#define DEBUG_FLAG NAUTILUS_DEBUG_ACTIONS
#include <nautilus-debug.h>

G_DEFINE_TYPE (NautilusAction, nautilus_action, GTK_TYPE_ACTION);

static void     nautilus_action_init       (NautilusAction      *action);

static void     nautilus_action_class_init (NautilusActionClass *klass);

static void     nautilus_action_get_property  (GObject                    *object,
                                               unsigned int                param_id,
                                               GValue                     *value,
                                               GParamSpec                 *pspec);

static void     nautilus_action_set_property  (GObject                    *object,
                                               unsigned int                param_id,
                                               const GValue               *value,
                                               GParamSpec                 *pspec);

static void     nautilus_action_constructed (GObject *object);

static void     nautilus_action_finalize (GObject *gobject);

static void    *parent_class;

#define ACTION_FILE_GROUP "Nautilus Action"

#define KEY_ACTIVE "Active"
#define KEY_NAME "Name"
#define KEY_COMMENT "Comment"
#define KEY_EXEC "Exec"
#define KEY_ICON_NAME "Icon-Name"
#define KEY_STOCK_ID "Stock-Id"
#define KEY_SELECTION "Selection"
#define KEY_EXTENSIONS "Extensions"
#define KEY_MIME_TYPES "Mimetypes"
#define KEY_SEPARATOR "Separator"
#define KEY_QUOTE_TYPE "Quote"
#define KEY_DEPENDENCIES "Dependencies"
#define KEY_CONDITIONS "Conditions"

enum
{
  PROP_0,
  PROP_KEY_FILE_PATH,
  PROP_SELECTION_TYPE,
  PROP_EXTENSIONS,
  PROP_MIMES,
  PROP_EXEC,
  PROP_PARENT_DIR,
  PROP_USE_PARENT_DIR,
  PROP_ORIG_LABEL,
  PROP_ORIG_TT,
  PROP_SEPARATOR,
  PROP_QUOTE_TYPE,
  PROP_CONDITIONS
};

typedef struct {
    NautilusAction *action;
    char           *name;
    unsigned int    watch_id;
   _Bool            exists;
} DBusCondition;

static void
dbus_condition_free (void *data)
{
    DBusCondition *cond = (DBusCondition *) data;
    g_free (cond->name);
    g_bus_unwatch_name (cond->watch_id);
}

static void
nautilus_action_init (NautilusAction *action)
{
    action->key_file_path = NULL;
    action->selection_type = SELECTION_SINGLE;
    action->extensions = NULL;
    action->mimetypes = NULL;
    action->exec = NULL;
    action->parent_dir = NULL;
    action->use_parent_dir = FALSE;
    action->orig_label = NULL;
    action->orig_tt = NULL;
    action->quote_type = QUOTE_TYPE_NONE;
    action->separator = NULL;
    action->conditions = NULL;
    action->dbus = NULL;
    action->dbus_satisfied = TRUE;
    action->escape_underscores = FALSE;
    action->log_output = g_getenv ("NAUTILUS_ACTION_VERBOSE") != NULL;
}

static void
nautilus_action_class_init (NautilusActionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    parent_class               = g_type_class_peek_parent (klass);
    object_class->finalize     = nautilus_action_finalize;
    object_class->set_property = nautilus_action_set_property;
    object_class->get_property = nautilus_action_get_property;
    object_class->constructed  = nautilus_action_constructed;

    g_object_class_install_property (object_class,
                                     PROP_KEY_FILE_PATH,
                                     g_param_spec_string ("key-file-path",
                                                          "Key File Path",
                                                          "The key file path associated with this action",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)
                                     );

    g_object_class_install_property (object_class,
                                     PROP_SELECTION_TYPE,
                                     g_param_spec_int ("selection-type",
                                                       "Selection Type",
                                                       "The action selection type",
                                                       0,
                                                       SELECTION_NONE,
                                                       SELECTION_SINGLE,
                                                       G_PARAM_READWRITE)
                                     );

    g_object_class_install_property (object_class,
                                     PROP_EXTENSIONS,
                                     g_param_spec_pointer ("extensions",
                                                           "Extensions",
                                                           "String array of file extensions",
                                                           G_PARAM_READWRITE)
                                     );

    g_object_class_install_property (object_class,
                                     PROP_MIMES,
                                     g_param_spec_pointer ("mimetypes",
                                                           "Mimetypes",
                                                           "String array of file mimetypes",
                                                           G_PARAM_READWRITE)
                                     );

    g_object_class_install_property (object_class,
                                     PROP_EXEC,
                                     g_param_spec_string ("exec",
                                                          "Executable String",
                                                          "The command line to run",
                                                          NULL,
                                                          G_PARAM_READWRITE)
                                     );

    g_object_class_install_property (object_class,
                                     PROP_PARENT_DIR,
                                     g_param_spec_string ("parent-dir",
                                                          "Parent directory",
                                                          "The directory the action file resides in",
                                                          NULL,
                                                          G_PARAM_READWRITE)
                                     );
    g_object_class_install_property (object_class,
                                     PROP_USE_PARENT_DIR,
                                     g_param_spec_boolean ("use-parent-dir",
                                                           "Use Parent Directory",
                                                           "Execute using the full action path",
                                                           FALSE,
                                                           G_PARAM_READWRITE)
                                     );
    g_object_class_install_property (object_class,
                                     PROP_ORIG_LABEL,
                                     g_param_spec_string ("orig-label",
                                                          "Original label string",
                                                          "The starting label - with token",
                                                          NULL,
                                                          G_PARAM_READWRITE)
                                     );
    g_object_class_install_property (object_class,
                                     PROP_ORIG_TT,
                                     g_param_spec_string ("orig-tooltip",
                                                          "Original tooltip string",
                                                          "The starting tooltip - with token",
                                                          NULL,
                                                          G_PARAM_READWRITE)
                                     );

    g_object_class_install_property (object_class,
                                     PROP_SEPARATOR,
                                     g_param_spec_string ("separator",
                                                          "Separator to insert between files in the exec line",
                                                          "Separator to use between files, like comma, space, etc",
                                                          NULL,
                                                          G_PARAM_READWRITE)
                                     );

    g_object_class_install_property (object_class,
                                     PROP_QUOTE_TYPE,
                                     g_param_spec_int ("quote-type",
                                                       "Type of quotes to use to enclose individual file names",
                                                       "Type of quotes to use to enclose individual file names - none, single or double",
                                                       QUOTE_TYPE_SINGLE,
                                                       QUOTE_TYPE_NONE,
                                                       QUOTE_TYPE_SINGLE,
                                                       G_PARAM_READWRITE)
                                     );

    g_object_class_install_property (object_class,
                                     PROP_CONDITIONS,
                                     g_param_spec_pointer ("conditions",
                                                           "Special show conditions",
                                                           "Special conditions, like a bool gsettings key, or 'desktop'",
                                                           G_PARAM_READWRITE)
                                     );
}

static void
recalc_dbus_conditions (NautilusAction *action)
{
    GList *list;
    DBusCondition *c;
    _Bool cumul_found = TRUE;

    for (list = action->dbus; list != NULL; list = list->next) {
        c = list->data;
        if (!c->exists) {
            cumul_found = FALSE;
            break;
        }
    }

    action->dbus_satisfied = cumul_found;
}

static void
on_dbus_appeared (GDBusConnection *connection,
                  const char      *name,
                  const char      *name_owner,
                  void            *user_data)
{
    DBusCondition *cond = user_data;

    cond->exists = TRUE;
    recalc_dbus_conditions (cond->action);
}

static void
on_dbus_disappeared (GDBusConnection *connection,
                     const char      *name,
                     void            *user_data)
{
    DBusCondition *cond = user_data;

    cond->exists = FALSE;
    recalc_dbus_conditions (cond->action);
}

static void
setup_dbus_condition (NautilusAction *action, const char *condition)
{
    char **split = g_strsplit (condition, " ", 2);

    if (g_strv_length (split) != 2) {
        g_strfreev (split);
        return;
    }

    if (g_strcmp0 (split[0], "dbus") != 0) {
        g_strfreev (split);
        return;
    }

    DBusCondition *cond = g_new0 (DBusCondition, 1);

    cond->name = g_strdup (split[0]);
    cond->exists = FALSE;
    cond->action = action;
    action->dbus = g_list_append (action->dbus, cond);
    cond->watch_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                                       split[1],
                                       0,
                                       on_dbus_appeared,
                                       on_dbus_disappeared,
                                       cond,
                                       NULL);
}

static void
strip_custom_modifier (const char *raw, _Bool *custom, char **out)
{
    if (g_str_has_prefix (raw, "<") && g_str_has_suffix (raw, ">")) {
        char **split = g_strsplit_set (raw, "<>", 3);
        *out = g_strdup (split[1]);
        *custom = TRUE;
        g_free (split);
    } else {
        *out = g_strdup (raw);
        *custom = FALSE;
    }
}

void
nautilus_action_constructed (GObject *object)
{
    G_OBJECT_CLASS (parent_class)->constructed (object);

    NautilusAction *action = NAUTILUS_ACTION (object);

    GKeyFile *key_file = g_key_file_new();

    g_key_file_load_from_file (key_file, action->key_file_path, G_KEY_FILE_NONE, NULL);

    char *orig_label = g_key_file_get_locale_string (key_file,
                                                      ACTION_FILE_GROUP,
                                                      KEY_NAME,
                                                      NULL,
                                                      NULL);

    char *orig_tt = g_key_file_get_locale_string (key_file,
                                                   ACTION_FILE_GROUP,
                                                   KEY_COMMENT,
                                                   NULL,
                                                   NULL);

    char *icon_name = g_key_file_get_string (key_file,
                                              ACTION_FILE_GROUP,
                                              KEY_ICON_NAME,
                                              NULL);

    char *stock_id = g_key_file_get_string (key_file,
                                             ACTION_FILE_GROUP,
                                             KEY_STOCK_ID,
                                             NULL);


    char *exec_raw = g_key_file_get_string (key_file,
                                             ACTION_FILE_GROUP,
                                             KEY_EXEC,
                                             NULL);

    char *selection_string_raw = g_key_file_get_string (key_file,
                                                         ACTION_FILE_GROUP,
                                                         KEY_SELECTION,
                                                         NULL);

    char *selection_string = g_ascii_strdown (selection_string_raw, -1);

    g_free (selection_string_raw);

    char *separator = g_key_file_get_string (key_file,
                                              ACTION_FILE_GROUP,
                                              KEY_SEPARATOR,
                                              NULL);

    char *quote_type_string = g_key_file_get_string (key_file,
                                                      ACTION_FILE_GROUP,
                                                      KEY_QUOTE_TYPE,
                                                      NULL);

    QuoteType quote_type = QUOTE_TYPE_NONE;

    if (quote_type_string != NULL) {
        if (g_strcmp0 (quote_type_string, "single") == 0)
            quote_type = QUOTE_TYPE_SINGLE;
        else if (g_strcmp0 (quote_type_string, "double") == 0)
            quote_type = QUOTE_TYPE_DOUBLE;
        else if (g_strcmp0 (quote_type_string, "backtick") == 0)
            quote_type = QUOTE_TYPE_BACKTICK;
    }

    SelectionType type;

    if (g_strcmp0 (selection_string, SELECTION_SINGLE_KEY) == 0)
        type = SELECTION_SINGLE;
    else if (g_strcmp0 (selection_string, SELECTION_MULTIPLE_KEY) == 0)
        type = SELECTION_MULTIPLE;
    else if (g_strcmp0 (selection_string, SELECTION_ANY_KEY) == 0)
        type = SELECTION_ANY;
    else if (g_strcmp0 (selection_string, SELECTION_NONE_KEY) == 0)
        type = SELECTION_NONE;
    else if (g_strcmp0 (selection_string, SELECTION_NOT_NONE_KEY) == 0)
        type = SELECTION_NOT_NONE;
    else {
        int val = (int) g_ascii_strtoll (selection_string, NULL, 10);
        type = val > 0 ? val : SELECTION_SINGLE;
    }

    g_free (selection_string);

    size_t count;

    char **ext = g_key_file_get_string_list (key_file,
                                              ACTION_FILE_GROUP,
                                              KEY_EXTENSIONS,
                                              &count,
                                              NULL);

    size_t mime_count;

    char **mimes = g_key_file_get_string_list (key_file,
                                                ACTION_FILE_GROUP,
                                                KEY_MIME_TYPES,
                                                &mime_count,
                                                NULL);

    size_t condition_count;

    char **conditions = g_key_file_get_string_list (key_file,
                                                     ACTION_FILE_GROUP,
                                                     KEY_CONDITIONS,
                                                     &condition_count,
                                                     NULL);

    if (conditions && condition_count > 0) {
        int j;
        char *condition;
        for (j = 0; j < condition_count; j++) {
            condition = conditions[j];
            if (g_str_has_prefix (condition, "dbus")) {
                setup_dbus_condition (action, condition);
            }
        }
    }

    char *exec = NULL;
    _Bool use_parent_dir = FALSE;

    strip_custom_modifier (exec_raw, &use_parent_dir, &exec);
    g_free (exec_raw);

    GFile *file = g_file_new_for_path (action->key_file_path);
    GFile *parent = g_file_get_parent (file);

    char *parent_dir = g_file_get_path (parent);

    g_object_unref (file);
    g_object_unref (parent);

    g_object_set  (action,
                   "label", orig_label,
                   "tooltip", orig_tt,
                   "icon-name", icon_name,
                   "stock-id", stock_id,
                   "exec", exec,
                   "selection-type", type,
                   "extensions", ext,
                   "mimetypes", mimes,
                   "parent-dir", parent_dir,
                   "use-parent-dir", use_parent_dir,
                   "orig-label", orig_label,
                   "orig-tooltip", orig_tt,
                   "quote-type", quote_type,
                   "separator", separator,
                   "conditions", conditions,
                    NULL);

    g_free (orig_label);
    g_free (orig_tt);
    g_free (icon_name);
    g_free (stock_id);
    g_free (exec);
    g_strfreev (ext);
    g_free (parent_dir);
    g_free (quote_type_string);
    g_free (separator);
    g_strfreev (conditions);
    g_key_file_free (key_file);
}

NautilusAction *
nautilus_action_new (const char *name,
                 const char *path)
{
    GKeyFile *key_file = g_key_file_new();

    g_key_file_load_from_file (key_file, path, G_KEY_FILE_NONE, NULL);

    if (!g_key_file_has_group (key_file, ACTION_FILE_GROUP))
        return NULL;

    if (g_key_file_has_key (key_file, ACTION_FILE_GROUP, KEY_ACTIVE, NULL)) {
        if (!g_key_file_get_boolean (key_file, ACTION_FILE_GROUP, KEY_ACTIVE, NULL))
            return NULL;
    }

    char *orig_label = g_key_file_get_locale_string (key_file,
                                                      ACTION_FILE_GROUP,
                                                      KEY_NAME,
                                                      NULL,
                                                      NULL);

    char *exec_raw = g_key_file_get_string (key_file,
                                             ACTION_FILE_GROUP,
                                             KEY_EXEC,
                                             NULL);

    char **ext = g_key_file_get_string_list (key_file,
                                              ACTION_FILE_GROUP,
                                              KEY_EXTENSIONS,
                                              NULL,
                                              NULL);

    char **mimes = g_key_file_get_string_list (key_file,
                                                ACTION_FILE_GROUP,
                                                KEY_MIME_TYPES,
                                                NULL,
                                                NULL);

    char **deps  = g_key_file_get_string_list (key_file,
                                                ACTION_FILE_GROUP,
                                                KEY_DEPENDENCIES,
                                                NULL,
                                                NULL);

    char *selection_string = g_key_file_get_string (key_file,
                                                     ACTION_FILE_GROUP,
                                                     KEY_SELECTION,
                                                     NULL);

    _Bool finish = TRUE;

    if (deps != NULL) {
        int i = 0;
        for (i = 0; i < g_strv_length (deps); i++) {
            if (g_path_is_absolute (deps[i])) {
                GFile *f = g_file_new_for_path (deps[i]);
                if (!g_file_query_exists (f, NULL)) {
                    finish = FALSE;
                    DEBUG ("Missing action dependency: %s", deps[i]);
                }
                g_object_unref (f);
            } else {
                char *p = g_find_program_in_path (deps[i]);
                if (p == NULL) {
                    finish = FALSE;
                    DEBUG ("Missing action dependency: %s", deps[i]);
                    g_free (p);
                    break;
                }
                g_free (p);
            }
        }
    }

    if (orig_label == NULL || exec_raw == NULL || (ext == NULL && mimes == NULL) || selection_string == NULL) {
        g_printerr ("An action definition requires, at minimum, "
                    "a Label field, an Exec field, a Selection field, and an either an Extensions or Mimetypes field.\n"
                    "Check the %s file for missing fields.\n", path);
        finish = FALSE;
    }

    g_free (orig_label);
    g_free (exec_raw);
    g_free (selection_string);
    g_strfreev (ext);
    g_strfreev (mimes);
    g_strfreev (deps);
    g_key_file_free (key_file);

    return finish ? g_object_new (NAUTILUS_TYPE_ACTION,
                                  "name", name,
                                  "key-file-path", path,
                                  NULL): NULL;
}

static void
nautilus_action_finalize (GObject *object)
{
    NautilusAction *action = NAUTILUS_ACTION (object);

    g_free (action->key_file_path);
    g_strfreev (action->extensions);
    g_strfreev (action->mimetypes);
    g_strfreev (action->conditions);
    g_free (action->exec);
    g_free (action->parent_dir);
    g_free (action->orig_label);
    g_free (action->orig_tt);

    if (action->dbus) {
        g_list_free_full (action->dbus, (GDestroyNotify) dbus_condition_free);
    }

    G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
nautilus_action_set_property (GObject         *object,
                          guint            prop_id,
                          const GValue    *value,
                          GParamSpec      *pspec)
{
  NautilusAction *action;

  action = NAUTILUS_ACTION (object);

  switch (prop_id)
    {
    case PROP_KEY_FILE_PATH:
      nautilus_action_set_key_file_path (action, g_value_get_string (value));
      break;
    case PROP_SELECTION_TYPE:
      action->selection_type = g_value_get_int (value);
      break;
    case PROP_EXTENSIONS:
      nautilus_action_set_extensions (action, g_value_get_pointer (value));
      break;
    case PROP_MIMES:
      nautilus_action_set_mimetypes (action, g_value_get_pointer (value));
      break;
    case PROP_EXEC:
      nautilus_action_set_exec (action, g_value_get_string (value));
      break;
    case PROP_PARENT_DIR:
      nautilus_action_set_parent_dir (action, g_value_get_string (value));
      break;
    case PROP_USE_PARENT_DIR:
      action->use_parent_dir = g_value_get_boolean (value);
      break;
    case PROP_ORIG_LABEL:
      nautilus_action_set_orig_label (action, g_value_get_string (value));
      break;
    case PROP_ORIG_TT:
      nautilus_action_set_orig_tt (action, g_value_get_string (value));
      break;
    case PROP_QUOTE_TYPE:
      action->quote_type = g_value_get_int (value);
      break;
    case PROP_SEPARATOR:
      nautilus_action_set_separator (action, g_value_get_string (value));
      break;
    case PROP_CONDITIONS:
      nautilus_action_set_conditions (action, g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
nautilus_action_get_property (GObject    *object,
             guint       prop_id,
             GValue     *value,
             GParamSpec *pspec)
{
  NautilusAction *action;

  action = NAUTILUS_ACTION (object);

  switch (prop_id)
    {
    case PROP_KEY_FILE_PATH:
      g_value_set_string (value, action->key_file_path);
      break;
    case PROP_SELECTION_TYPE:
      g_value_set_int (value, action->selection_type);
      break;
    case PROP_EXTENSIONS:
      g_value_set_pointer (value, action->extensions);
      break;
    case PROP_MIMES:
      g_value_set_pointer (value, action->mimetypes);
      break;
    case PROP_EXEC:
      g_value_set_string (value, action->exec);
      break;
    case PROP_PARENT_DIR:
      g_value_set_string (value, action->parent_dir);
      break;
    case PROP_USE_PARENT_DIR:
      g_value_set_boolean (value, action->use_parent_dir);
      break;
    case PROP_ORIG_LABEL:
      g_value_set_string (value, action->orig_label);
      break;
    case PROP_ORIG_TT:
      g_value_set_string (value, action->orig_tt);
      break;
    case PROP_QUOTE_TYPE:
      g_value_set_int (value, action->quote_type);
      break;
    case PROP_SEPARATOR:
      g_value_set_string (value, action->separator);
      break;
    case PROP_CONDITIONS:
      g_value_set_pointer (value, action->conditions);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static char *
find_token_type (const char *str, TokenType *token_type)
{
    char *ptr = NULL;
    *token_type = TOKEN_NONE;

    ptr = g_strstr_len (str, -1, TOKEN_EXEC_FILE_LIST);
    if (ptr != NULL) {
        *token_type = TOKEN_PATH_LIST;
        return ptr;
    }
    ptr = g_strstr_len (str, -1, TOKEN_EXEC_URI_LIST);
    if (ptr != NULL) {
        *token_type = TOKEN_URI_LIST;
        return ptr;
    }
    ptr = g_strstr_len (str, -1, TOKEN_EXEC_PARENT);
    if (ptr != NULL) {
        *token_type = TOKEN_PARENT_PATH;
        return ptr;
    }
    ptr = g_strstr_len (str, -1, TOKEN_EXEC_FILE_NAME);
    if (ptr != NULL) {
        *token_type = TOKEN_FILE_DISPLAY_NAME;
        return ptr;
    }
    ptr = g_strstr_len (str, -1, TOKEN_EXEC_PARENT_NAME);
    if (ptr != NULL) {
        *token_type = TOKEN_PARENT_DISPLAY_NAME;
        return ptr;
    }
    ptr = g_strstr_len (str, -1, TOKEN_LABEL_FILE_NAME);
    if (ptr != NULL) {
        *token_type = TOKEN_FILE_DISPLAY_NAME;
        return ptr;
    }
    ptr = g_strstr_len (str, -1, TOKEN_EXEC_DEVICE);
    if (ptr != NULL) {
        *token_type = TOKEN_DEVICE;
        return ptr;
    }
    return NULL;
}

static GString *
_score_append (NautilusAction *action, GString *str, const char *c)
{
    if (action->escape_underscores) {
        char *escaped = eel_str_double_underscores (c);
        str = g_string_append (str, escaped);
        g_free (escaped);
        return str;
    } else {
        return g_string_append (str, c);
    }
}

static GString *
insert_separator (NautilusAction *action, GString *str)
{
    if (action->separator == NULL)
        str = g_string_append (str, " ");
    else
        str = _score_append (action, str, action->separator);

    return str;
}

static GString *
insert_quote (NautilusAction *action, GString *str)
{
    switch (action->quote_type) {
        case QUOTE_TYPE_SINGLE:
            str = g_string_append (str, "'");
            break;
        case QUOTE_TYPE_DOUBLE:
            str = g_string_append (str, "\"");
            break;
        case QUOTE_TYPE_BACKTICK:
            str = g_string_append (str, "`");
            break;
        case QUOTE_TYPE_NONE:
            break;
    }

    return str;
}

static char *
get_device_path (NautilusFile *file)
{
    GMount *mount = nautilus_file_get_mount (file);
    GVolume *volume = g_mount_get_volume (mount);
    char *id = g_volume_get_identifier (volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);

    g_object_unref (mount);
    g_object_unref (volume);

    return id;
}

static char *
get_insertion_string (NautilusAction *action, TokenType token_type, GList *selection, NautilusFile *parent)
{
    GList *l;

    GString *str = g_string_new("");
    _Bool first = TRUE;

    switch (token_type) {
        case TOKEN_PATH_LIST:
            if (g_list_length (selection) > 0) {
                for (l = selection; l != NULL; l = l->next) {
                    if (!first)
                        str = insert_separator (action, str);
                    str = insert_quote (action, str);
                    char *path = nautilus_file_get_path (NAUTILUS_FILE (l->data));
                    if (path)
                        str = _score_append (action, str, path);
                    g_free (path);
                    str = insert_quote (action, str);
                    first = FALSE;
                }
            } else {
                goto default_parent_path;
            }
            break;
        case TOKEN_URI_LIST:
            if (g_list_length (selection) > 0) {
                for (l = selection; l != NULL; l = l->next) {
                    if (!first)
                        str = insert_separator (action, str);
                    str = insert_quote (action, str);
                    char *uri = nautilus_file_get_uri (NAUTILUS_FILE (l->data));
                    char *escaped = g_uri_unescape_string (uri, NULL);
                    if (escaped)
                        str = _score_append (action, str, escaped);
                    g_free (uri);
                    g_free (escaped);
                    str = insert_quote (action, str);
                    first = FALSE;
                }
            } else {
                goto default_parent_path;
            }
            break;
        case TOKEN_PARENT_PATH:
            ;
default_parent_path:
            ;
            char *path = nautilus_file_get_path (parent);
            if (path == NULL) {
                char *name = nautilus_file_get_display_name (parent);
                if (g_strcmp0 (name, "x-nautilus-desktop") == 0)
                    path = nautilus_get_desktop_directory ();
                else
                    path = g_strdup_printf (" ");
                g_free (name);
            }
            str = insert_quote (action, str);
            str = _score_append (action, str, path);
            str = insert_quote (action, str);
            g_free (path);
            break;
        case TOKEN_FILE_DISPLAY_NAME:
            if (g_list_length (selection) > 0) {
                char *file_display_name = nautilus_file_get_display_name (NAUTILUS_FILE (selection->data));
                str = _score_append (action, str, file_display_name);
                g_free (file_display_name);
            } else {
                goto default_parent_display_name;
            }
            break;
        case TOKEN_PARENT_DISPLAY_NAME:
            ;
default_parent_display_name:
            ;
            char *parent_display_name;
            char *real_display_name = nautilus_file_get_display_name (parent);
            if (g_strcmp0 (real_display_name, "x-nautilus-desktop") == 0)
                parent_display_name = g_strdup_printf (_("Desktop"));
            else
                parent_display_name = nautilus_file_get_display_name (parent);
            g_free (real_display_name);
            str = insert_quote (action, str);
            str = _score_append (action, str, parent_display_name);
            str = insert_quote (action, str);
            g_free (parent_display_name);
            break;
        case TOKEN_DEVICE:
            if (g_list_length (selection) > 0) {
                for (l = selection; l != NULL; l = l->next) {
                    if (!first)
                        str = insert_separator (action, str);
                    str = insert_quote (action, str);
                    char *dev = get_device_path (NAUTILUS_FILE (l->data));
                    if (dev)
                        str = _score_append (action, str, dev);
                    g_free (dev);
                    str = insert_quote (action, str);
                    first = FALSE;
                }
            } else {
                goto default_parent_path;
            }
            break;
    }

    char *ret = str->str;

    g_string_free (str, FALSE);

    return ret;
}

static GString *
expand_action_string (NautilusAction *action, GList *selection, NautilusFile *parent, GString *str)
{
    char *ptr;
    TokenType token_type;

    ptr = find_token_type (str->str, &token_type);

    while (ptr != NULL) {
        int shift = ptr - str->str;

        char *insertion = get_insertion_string (action, token_type, selection, parent);
        str = g_string_erase (str, shift, 2);
        str = g_string_insert (str, shift, insertion);

        token_type = TOKEN_NONE;
        g_free  (insertion);
        ptr = find_token_type (str->str, &token_type);
    }

    return str;
}

void
nautilus_action_activate (NautilusAction *action, GList *selection, NautilusFile *parent)
{
    GString *exec = g_string_new (action->exec);

    action->escape_underscores = FALSE;

    exec = expand_action_string (action, selection, parent, exec);

    if (action->use_parent_dir) {
        exec = g_string_prepend (exec, G_DIR_SEPARATOR_S);
        exec = g_string_prepend (exec, action->parent_dir);
    }

    DEBUG ("Action Spawning: %s", exec->str);
    if (action->log_output)
        g_printerr ("Action Spawning: %s\n", exec->str);

    g_spawn_command_line_async (exec->str, NULL);

    g_string_free (exec, TRUE);
}

SelectionType
nautilus_action_get_selection_type (NautilusAction *action)
{
    return action->selection_type;
}

char **
nautilus_action_get_extension_list (NautilusAction *action)
{
    return action->extensions;
}

char **
nautilus_action_get_mimetypes_list (NautilusAction *action)
{
    return action->mimetypes;
}

void
nautilus_action_set_key_file_path (NautilusAction *action, const char *path)
{
    char *tmp;
    tmp = action->key_file_path;
    action->key_file_path = g_strdup (path);
    g_free (tmp);
}

void
nautilus_action_set_exec (NautilusAction *action, const char *exec)
{
    char *tmp;

    tmp = action->exec;
    action->exec = g_strdup (exec);
    g_free (tmp);
}

void
nautilus_action_set_parent_dir (NautilusAction *action, const char *parent_dir)
{
    char *tmp;

    tmp = action->parent_dir;
    action->parent_dir = g_strdup (parent_dir);
    g_free (tmp);
}

void
nautilus_action_set_separator (NautilusAction *action, const char *separator)
{
    char *tmp;

    tmp = action->separator;
    action->separator = g_strdup (separator);
    g_free (tmp);
}

void
nautilus_action_set_conditions (NautilusAction *action, char **conditions)
{
    char **tmp;

    tmp = action->conditions;
    action->conditions = g_strdupv (conditions);
    g_strfreev (tmp);
}

void
nautilus_action_set_orig_label (NautilusAction *action, const char *orig_label)
{
    char *tmp;

    tmp = action->orig_label;
    action->orig_label = g_strdup (orig_label);
    g_free (tmp);
}

const char *
nautilus_action_get_orig_label (NautilusAction *action)
{
    return action->orig_label;
}

void
nautilus_action_set_orig_tt (NautilusAction *action, const char *orig_tt)
{
    char *tmp;

    tmp = action->orig_tt;
    action->orig_tt = g_strdup (orig_tt);
    g_free (tmp);
}

const char *
nautilus_action_get_orig_tt (NautilusAction *action)
{
    return action->orig_tt;
}

char **
nautilus_action_get_conditions (NautilusAction *action)
{
    return action->conditions;
}

char *
nautilus_action_get_label (NautilusAction *action, GList *selection, NautilusFile *parent)
{
    const char *orig_label = nautilus_action_get_orig_label (action);

    if (orig_label == NULL)
        return;

    action->escape_underscores = TRUE;

    GString *str = g_string_new (orig_label);

    str = expand_action_string (action, selection, parent, str);

    DEBUG ("Action Label: %s", str->str);
    if (action->log_output)
        g_printerr ("Action Label: %s\n", str->str);

    char *ret = str->str;
    g_string_free (str, FALSE);
    return ret;
}

char *
nautilus_action_get_tt (NautilusAction *action, GList *selection, NautilusFile *parent)
{
    const char *orig_tt = nautilus_action_get_orig_tt (action);

    if (orig_tt == NULL)
        return;

    action->escape_underscores = FALSE;

    GString *str = g_string_new (orig_tt);

    str = expand_action_string (action, selection, parent, str);

    DEBUG ("Action Tooltip: %s", str->str);
    if (action->log_output)
        g_printerr ("Action Tooltip: %s\n", str->str);

    char *ret = str->str;
    g_string_free (str, FALSE);
    return ret;
}

void
nautilus_action_set_extensions (NautilusAction *action, char **extensions)
{
    char **tmp;

    tmp = action->extensions;
    action->extensions = g_strdupv (extensions);
    g_strfreev (tmp);
}

void
nautilus_action_set_mimetypes (NautilusAction *action, char **mimetypes)
{
    char **tmp;

    tmp = action->mimetypes;
    action->mimetypes = g_strdupv (mimetypes);
    g_strfreev (tmp);
}

_Bool
nautilus_action_get_dbus_satisfied (NautilusAction *action)
{
    return action->dbus_satisfied;
}


static _Bool
check_gsettings_condition (NautilusAction *action, const char *condition)
{

    char **split = g_strsplit (condition, " ", 3);

    if (g_strv_length (split) != 3) {
        g_strfreev (split);
        return FALSE;
    }

    if (g_strcmp0 (split[0], "gsettings") != 0) {
        g_strfreev (split);
        return FALSE;
    }

    GSettingsSchemaSource *schema_source;

    schema_source = g_settings_schema_source_get_default();

    if (g_settings_schema_source_lookup (schema_source, split[1], TRUE)) {
        GSettings *s = g_settings_new (split[1]);
        char **keys = g_settings_list_keys (s);
        _Bool ret = FALSE;
        int i;
        for (i = 0; i < g_strv_length (keys); i++) {
            if (g_strcmp0 (keys[i], split[2]) == 0) {
                GVariant *var = eel_settings_get_value (s, split[2]);
                const GVariantType *type = g_variant_get_type (var);
                if (g_variant_type_equal (type, G_VARIANT_TYPE_BOOLEAN))
                    ret = g_variant_get_boolean (var);
                g_variant_unref (var);
            }
        }
        g_strfreev (keys);
        g_object_unref (s);
        g_strfreev (split);
        return ret;
    } else {
        g_strfreev (split);
        return FALSE;
    }
}

static _Bool
get_is_dir_hack (NautilusFile *file)
{
    _Bool ret = FALSE;

    GFile *f = nautilus_file_get_location (file);
    GFileType type = g_file_query_file_type (f, 0, NULL);
    ret = type == G_FILE_TYPE_DIRECTORY;
    return ret;
}

_Bool
nautilus_action_get_visibility (NautilusAction *action, GList *selection, NautilusFile *parent)
{

  _Bool selection_type_show = FALSE;
  _Bool extension_type_show = TRUE;
  _Bool condition_type_show = TRUE;

  unsigned int condition_count;
  unsigned int selected_count;
  unsigned int ext_count;
  unsigned int mime_count;

  char **extensions;
  char **mimetypes;

  recalc_dbus_conditions (action);

  if (nautilus_action_get_dbus_satisfied (action)) {


    char **conditions = nautilus_action_get_conditions (action);

    condition_count = conditions != NULL ? g_strv_length (conditions) : 0;

    if (condition_count > 0) {
      int j;
      char *condition;
      for (j = 0; j < condition_count; j++) {
        condition = conditions[j];
        if (g_strcmp0 (condition, "desktop") == 0) {
          char *name = nautilus_file_get_display_name (parent);
          if (g_strcmp0 (name, "x-nautilus-desktop") != 0)
            condition_type_show = FALSE;
          g_free (name);
          break;
        }
        else if (g_strcmp0 (condition, "removable") == 0) {
          _Bool is_removable = FALSE;
          if (g_list_length (selection) > 0) {
            GMount *mount = nautilus_file_get_mount (selection->data);
            if (mount) {
              GDrive *drive = g_mount_get_drive (mount);
              if (drive) {
                if (g_drive_is_media_removable (drive))
                  is_removable = TRUE;
                g_object_unref (drive);
              }
            }
          }
          condition_type_show = is_removable;
        } else if (g_str_has_prefix (condition, "gsettings")) {
          condition_type_show = check_gsettings_condition (action, condition);
          if (!condition_type_show)
            break;
        }
        if (!condition_type_show)
          break;
      }
    }

    if (!condition_type_show)
      goto out;

    SelectionType selection_type = nautilus_action_get_selection_type (action);
    GList *iter;

    selected_count = g_list_length (selection);

    switch (selection_type) {
      case SELECTION_SINGLE:
        selection_type_show = selected_count == 1;
        break;
      case SELECTION_MULTIPLE:
        selection_type_show = selected_count > 1;
        break;
      case SELECTION_NOT_NONE:
        selection_type_show = selected_count > 0;
        break;
      case SELECTION_NONE:
        selection_type_show = selected_count == 0;
        break;
      case SELECTION_ANY:
        selection_type_show = TRUE;
        break;
      default:
        selection_type_show = selected_count == selection_type;
        break;
    }

    extensions = nautilus_action_get_extension_list (action);
    mimetypes = nautilus_action_get_mimetypes_list (action);

    ext_count = extensions != NULL ? g_strv_length (extensions) : 0;
    mime_count = mimetypes != NULL ? g_strv_length (mimetypes) : 0;

    if (!(ext_count == 1 && g_strcmp0 (extensions[0], "any") == 0)) {

      _Bool found_match = TRUE;

      for (iter = selection; iter != NULL && found_match; iter = iter->next) {
        found_match = FALSE;
        char *raw_fn = nautilus_file_get_name (NAUTILUS_FILE (iter->data));
        char *filename = g_ascii_strdown (raw_fn, -1);
        g_free (raw_fn);
        int i;
        _Bool is_dir = get_is_dir_hack (iter->data);
        if (ext_count > 0) {
          for (i = 0; i < ext_count; i++) {
            if (g_strcmp0 (extensions[i], "dir") == 0) {
              if (is_dir) {
                found_match = TRUE;
                break;
              }
            } else if (g_strcmp0 (extensions[i], "none") == 0) {
              if (g_strrstr (filename, ".") == NULL) {
                found_match = TRUE;
                break;
              }
            } else if (g_strcmp0 (extensions[i], "nodirs") == 0) {
              if (!is_dir) {
                found_match = TRUE;
                break;
              }
            } else {
              if (g_str_has_suffix (filename, g_ascii_strdown (extensions[i], -1))) {
                found_match = TRUE;
                break;
              }
            }
          }
          g_free (filename);
        }

        if (mime_count > 0) {
          for (i = 0; i < mime_count; i++) {
            if (nautilus_file_is_mime_type (NAUTILUS_FILE (iter->data), mimetypes[i])) {
              found_match = TRUE;
              break;
            }
          }
        }

        if (nautilus_file_is_mime_type (NAUTILUS_FILE (iter->data), "application/x-nautilus-link")) {
          found_match = FALSE;
        }
      }

      extension_type_show = found_match;
    }
  }

  out:

  return selection_type_show && extension_type_show && condition_type_show;
}