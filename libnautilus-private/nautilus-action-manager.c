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
#include <gio/gio.h>
#include <gtk/gtk.h>

#include <eel/eel-icons.h>

#include <nautilus-file-attributes.h>
#include <nautilus-icon-info.h>
#include "nautilus-file.h"

#include "nautilus-action-manager.h"
#include "nautilus-directory.h"
#include "nautilus-action.h"
#include "nautilus-file-utilities.h"


G_DEFINE_TYPE (NautilusActionManager, nautilus_action_manager, G_TYPE_OBJECT);

static void     set_up_actions                 (NautilusActionManager *action_manager);

static void     nautilus_action_manager_init       (NautilusActionManager      *action_manager);

static void     nautilus_action_manager_class_init (NautilusActionManagerClass *klass);

static void     nautilus_action_manager_constructed (GObject *object);

static void     nautilus_action_manager_dispose (GObject *gobject);

static void     nautilus_action_manager_finalize (GObject *gobject);

static void *parent_class;

enum
{
  PROP_0,
  PROP_CONDITIONS
};

enum {
    CHANGED,
    LAST_SIGNAL
};

static unsigned int signals[LAST_SIGNAL];

static void
actions_added_or_changed_callback (NautilusDirectory *directory,
                                   GList             *files,
                                   void              *callback_data)
{
    NautilusActionManager *action_manager;

    action_manager = NAUTILUS_ACTION_MANAGER (callback_data);

    action_manager->action_list_dirty = TRUE;

    set_up_actions (action_manager);
}

static void
add_directory_to_directory_list (NautilusActionManager *action_manager,
                                 NautilusDirectory     *directory,
                                 GList                **directory_list,
                                 GCallback          changed_callback)
{
    NautilusFileAttributes attributes;

    if (g_list_find (*directory_list, directory) == NULL) {
        nautilus_directory_ref (directory);

        attributes =
            NAUTILUS_FILE_ATTRIBUTES_FOR_ICON |
            NAUTILUS_FILE_ATTRIBUTE_INFO |
            NAUTILUS_FILE_ATTRIBUTE_DIRECTORY_ITEM_COUNT;

        nautilus_directory_file_monitor_add (directory, directory_list,
                             FALSE, attributes,
                             (NautilusDirectoryCallback)changed_callback, action_manager);

        g_signal_connect_object (directory, "files_added",
                     G_CALLBACK (changed_callback), action_manager, 0);
        g_signal_connect_object (directory, "files_changed",
                     G_CALLBACK (changed_callback), action_manager, 0);

        *directory_list = g_list_append (*directory_list, directory);
    }
}

static void
remove_directory_from_directory_list (NautilusActionManager *action_manager,
                                          NautilusDirectory *directory,
                                                 GList **directory_list,
                                               GCallback changed_callback)
{
    *directory_list = g_list_remove (*directory_list, directory);

    g_signal_handlers_disconnect_by_func (directory,
                          G_CALLBACK (changed_callback),
                          action_manager);

    nautilus_directory_file_monitor_remove (directory, directory_list);

    nautilus_directory_unref (directory);
}

static void
add_directory_to_actions_directory_list (NautilusActionManager *action_manager,
                                             NautilusDirectory *directory)
{
    add_directory_to_directory_list (action_manager, directory,
                                     &action_manager->actions_directory_list,
                                     G_CALLBACK (actions_added_or_changed_callback));
}

static void
remove_directory_from_actions_directory_list (NautilusActionManager *action_manager,
                                                  NautilusDirectory *directory)
{
    remove_directory_from_directory_list (action_manager, directory,
                                          &action_manager->actions_directory_list,
                                          G_CALLBACK (actions_added_or_changed_callback));
}

static void
set_up_actions_directories (NautilusActionManager *action_manager)
{

    char *sys_path = g_build_filename (NAUTILUS_DATADIR, "actions", NULL);
    char *sys_uri = g_filename_to_uri (sys_path, NULL, NULL);

    char *user_path = g_build_filename (g_get_user_data_dir (), "nautilus", "actions", NULL);

    if (!g_file_test (user_path, G_FILE_TEST_EXISTS)) {
        g_mkdir_with_parents (user_path, DEFAULT_NAUTILUS_DIRECTORY_MODE);
    }

    char *user_uri = g_filename_to_uri (user_path, NULL, NULL);

    if (action_manager->actions_directory_list != NULL) {
        nautilus_directory_list_free (action_manager->actions_directory_list);
    }

    NautilusDirectory *dir;

    dir = nautilus_directory_get_by_uri (user_uri);
    add_directory_to_actions_directory_list (action_manager, dir);
    nautilus_directory_unref (dir);

    dir = nautilus_directory_get_by_uri (sys_uri);
    add_directory_to_actions_directory_list (action_manager, dir);
    nautilus_directory_unref (dir);

    g_free (sys_path);
    g_free (sys_uri);
    g_free (user_path);
    g_free (user_uri);
}

static char *
escape_action_name (const char *action_name, const char *prefix)
{
    GString *s;

    if (action_name == NULL) {
        return NULL;
    }

    s = g_string_new (prefix);

    while (*action_name != 0) {
        switch (*action_name) {
        case '\\':
            g_string_append (s, "\\\\");
            break;
        case '/':
            g_string_append (s, "\\s");
            break;
        case '&':
            g_string_append (s, "\\a");
            break;
        case '"':
            g_string_append (s, "\\q");
            break;
        default:
            g_string_append_c (s, *action_name);
        }

        action_name ++;
    }
    return g_string_free (s, FALSE);
}

static void
add_action_to_action_list (NautilusActionManager *action_manager, NautilusFile *file)
{
    char *uri;
    char *action_name;
    NautilusAction *action;

    uri = nautilus_file_get_uri (file);

    action_name = escape_action_name (uri, "action_");
    char *path = g_filename_from_uri (uri, NULL, NULL);

    action = nautilus_action_new (action_name, path);

    g_free (path);

    if (action == NULL) {
        g_free (uri);
        g_free (action_name);
        return;
    }

    action_manager->actions = g_list_append (action_manager->actions, action);
}

static void
void_action_list (NautilusActionManager *action_manager)
{
    GList *tmp = action_manager->actions;

    action_manager->actions = NULL;

    g_list_free_full (tmp, g_object_unref);
}

static void
set_up_actions (NautilusActionManager *action_manager)
{
    GList *dir, *file_list, *node;
    NautilusFile *file;
    NautilusDirectory *directory;

    if (g_list_length (action_manager->actions) > 0)
        void_action_list (action_manager);

    for (dir = action_manager->actions_directory_list; dir != NULL; dir = dir->next) {
        directory = dir->data;
        file_list = nautilus_directory_get_file_list (directory);
        for (node = file_list; node != NULL; node = node->next) {
            file = node->data;
            if (!g_str_has_suffix (nautilus_file_get_name (file), ".nautilus_action"))
                continue;
            add_action_to_action_list (action_manager, file);
        }
        nautilus_file_list_free (file_list);
    }

    action_manager->action_list_dirty = FALSE;

    g_signal_emit (action_manager, signals[CHANGED], 0);
}

static void
nautilus_action_manager_init (NautilusActionManager *action_manager)
{
    action_manager->actions = NULL;
    action_manager->actions_directory_list = NULL;
    action_manager->action_list_dirty = TRUE;
}

static void
nautilus_action_manager_class_init (NautilusActionManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    parent_class               = g_type_class_peek_parent (klass);
    object_class->finalize     = nautilus_action_manager_finalize;
    object_class->dispose      = nautilus_action_manager_dispose;
    object_class->constructed  = nautilus_action_manager_constructed;

    signals[CHANGED] =
        g_signal_new ("changed",
                      G_TYPE_FROM_CLASS (object_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (NautilusActionManagerClass, changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
}

void
nautilus_action_manager_constructed (GObject *object)
{
    G_OBJECT_CLASS (parent_class)->constructed (object);

    NautilusActionManager *action_manager = NAUTILUS_ACTION_MANAGER (object);

    set_up_actions_directories (action_manager);

    set_up_actions (action_manager);

}

NautilusActionManager *
nautilus_action_manager_new (void)
{
    return g_object_new (NAUTILUS_TYPE_ACTION_MANAGER, NULL);
}

static void
nautilus_action_manager_dispose (GObject *object)
{
    NautilusActionManager *action_manager = NAUTILUS_ACTION_MANAGER (object);

    if (action_manager->actions_directory_list != NULL) {
        GList *node, *copy;
        copy = nautilus_directory_list_copy (action_manager->actions_directory_list);

        for (node = copy; node != NULL; node = node->next) {
            remove_directory_from_actions_directory_list (action_manager, node->data);
        }
        g_list_free (action_manager->actions_directory_list);
        action_manager->actions_directory_list = NULL;
        nautilus_directory_list_free (copy);
    }

    G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
nautilus_action_manager_finalize (GObject *object)
{
    NautilusActionManager *action_manager = NAUTILUS_ACTION_MANAGER (object);

    g_list_free_full (action_manager->actions, g_object_unref);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

GList *
nautilus_action_manager_list_actions (NautilusActionManager *action_manager)
{
    return action_manager->action_list_dirty ? NULL : action_manager->actions;
}