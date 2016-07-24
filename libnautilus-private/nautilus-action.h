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

#ifndef NAUTILUS_ACTION_H
#define NAUTILUS_ACTION_H

#define NAUTILUS_TYPE_ACTION nautilus_action_get_type()
#define NAUTILUS_ACTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), NAUTILUS_TYPE_ACTION, NautilusAction))
#define NAUTILUS_ACTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), NAUTILUS_TYPE_ACTION, NautilusActionClass))
#define NAUTILUS_IS_ACTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NAUTILUS_TYPE_ACTION))
#define NAUTILUS_IS_ACTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), NAUTILUS_TYPE_ACTION))
#define NAUTILUS_ACTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), NAUTILUS_TYPE_ACTION, NautilusActionClass))

#define SELECTION_SINGLE_KEY "s"
#define SELECTION_MULTIPLE_KEY "m"
#define SELECTION_ANY_KEY "any"
#define SELECTION_NONE_KEY "none"
#define SELECTION_NOT_NONE_KEY "notnone"

#define TOKEN_EXEC_URI_LIST "%U"
#define TOKEN_EXEC_FILE_LIST "%F"
#define TOKEN_EXEC_PARENT "%P"
#define TOKEN_EXEC_FILE_NAME "%f"
#define TOKEN_EXEC_PARENT_NAME "%p"
#define TOKEN_EXEC_DEVICE "%D"

#define TOKEN_LABEL_FILE_NAME "%N" // Leave in for compatibility, same as TOKEN_EXEC_FILE_NAME

typedef struct _NautilusAction NautilusAction;
typedef struct _NautilusActionClass NautilusActionClass;

typedef enum {
    SELECTION_SINGLE = G_MAXINT - 10,
    SELECTION_MULTIPLE,
    SELECTION_NOT_NONE,
    SELECTION_ANY,
    SELECTION_NONE
} SelectionType;

typedef enum {
    QUOTE_TYPE_SINGLE = 0,
    QUOTE_TYPE_DOUBLE,
    QUOTE_TYPE_BACKTICK,
    QUOTE_TYPE_NONE
} QuoteType;

typedef enum {
    TOKEN_NONE = 0,
    TOKEN_PATH_LIST,
    TOKEN_URI_LIST,
    TOKEN_FILE_DISPLAY_NAME,
    TOKEN_PARENT_DISPLAY_NAME,
    TOKEN_PARENT_PATH,
    TOKEN_DEVICE
} TokenType;

struct _NautilusAction {
    GtkAction parent;
    char *key_file_path;
    SelectionType selection_type;
    char **extensions;
    char **mimetypes;
    char *exec;
    char *parent_dir;
    char **conditions;
    char *separator;
    QuoteType quote_type;
    char *orig_label;
    char *orig_tt;
    _Bool use_parent_dir;
    _Bool log_output;
    GList *dbus;
    _Bool dbus_satisfied;
    _Bool escape_underscores;
};

struct _NautilusActionClass {
	GtkActionClass parent_class;
};

GType         nautilus_action_get_type             (void);

NautilusAction   *nautilus_action_new              (const char *name, const char *path);
void          nautilus_action_activate             (NautilusAction *action, GList *selection, NautilusFile *parent);
SelectionType nautilus_action_get_selection_type   (NautilusAction *action);
char        **nautilus_action_get_extension_list   (NautilusAction *action);
char        **nautilus_action_get_mimetypes_list   (NautilusAction *action);
void          nautilus_action_set_key_file_path    (NautilusAction *action, const char *path);
void          nautilus_action_set_exec             (NautilusAction *action, const char *exec);
void          nautilus_action_set_parent_dir       (NautilusAction *action, const char *parent_dir);
void          nautilus_action_set_separator        (NautilusAction *action, const char *separator);
void          nautilus_action_set_conditions       (NautilusAction *action, char **conditions);
void          nautilus_action_set_orig_label       (NautilusAction *action, const char *orig_label);
void          nautilus_action_set_orig_tt          (NautilusAction *action, const char *orig_tt);
const char   *nautilus_action_get_orig_label       (NautilusAction *action);
const char   *nautilus_action_get_orig_tt          (NautilusAction *action);
char        **nautilus_action_get_conditions       (NautilusAction *action);
char         *nautilus_action_get_label            (NautilusAction *action, GList *selection, NautilusFile *parent);
char         *nautilus_action_get_tt               (NautilusAction *action, GList *selection, NautilusFile *parent);
void          nautilus_action_set_extensions       (NautilusAction *action, char **extensions);
void          nautilus_action_set_mimetypes        (NautilusAction *action, char **mimetypes);
_Bool         nautilus_action_get_dbus_satisfied   (NautilusAction *action);
_Bool         nautilus_action_get_visibility       (NautilusAction *action, GList *selection, NautilusFile *parent);

#endif /* NAUTILUS_ACTION_H */
