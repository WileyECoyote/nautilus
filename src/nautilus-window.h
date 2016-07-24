/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/*
 *  Nautilus
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 1999, 2000, 2001 Eazel, Inc.
 *
 *  Nautilus is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Nautilus is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Suite 500, MA 02110-1335, USA.
 *
 *  Authors: Elliot Lee <sopwith@redhat.com>
 *           Darin Adler <darin@bentspoon.com>
 *
 */
/* nautilus-window.h: Interface of the main window object */

#ifndef NAUTILUS_WINDOW_H
#define NAUTILUS_WINDOW_H

#include <gtk/gtk.h>
#include <eel/eel-glib-extensions.h>
#include <libnautilus-private/nautilus-bookmark.h>
#include <libnautilus-private/nautilus-query.h>
#include <libnautilus-private/nautilus-search-directory.h>

#include "nautilus-navigation-state.h"
#include "nautilus-view.h"
#include "nautilus-window-types.h"

#define NAUTILUS_TYPE_WINDOW nautilus_window_get_type()
#define NAUTILUS_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NAUTILUS_TYPE_WINDOW, NautilusWindow))
#define NAUTILUS_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NAUTILUS_TYPE_WINDOW, NautilusWindowClass))
#define NAUTILUS_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NAUTILUS_TYPE_WINDOW))
#define NAUTILUS_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NAUTILUS_TYPE_WINDOW))
#define NAUTILUS_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NAUTILUS_TYPE_WINDOW, NautilusWindowClass))

typedef enum {
        NAUTILUS_WINDOW_SHOW_HIDDEN_FILES_ENABLE,
        NAUTILUS_WINDOW_SHOW_HIDDEN_FILES_DISABLE
} NautilusWindowShowHiddenFilesMode;

typedef enum {
        NAUTILUS_WINDOW_NOT_SHOWN,
        NAUTILUS_WINDOW_POSITION_SET,
        NAUTILUS_WINDOW_SHOULD_SHOW
} NautilusWindowShowState;

typedef enum {
	NAUTILUS_WINDOW_OPEN_SLOT_NONE = 0,
	NAUTILUS_WINDOW_OPEN_SLOT_APPEND = 1
}  NautilusWindowOpenSlotFlags;

enum {
    SORT_NULL = -1,
    SORT_ASCENDING = 0,
    SORT_DESCENDING = 1
};

enum {
    TIGHTER_NULL = -1,
    TIGHTER_NO = 0,
    TIGHTER_YES = 1
};

#define NAUTILUS_WINDOW_SIDEBAR_PLACES "places"
#define NAUTILUS_WINDOW_SIDEBAR_TREE "tree"

typedef struct NautilusWindowDetails NautilusWindowDetails;

struct NautilusWindow {
        GtkWindow parent_object;

        NautilusWindowDetails *details;
};

typedef struct {
        GtkWindowClass parent_spot;

	/* Function pointers for overriding, without corresponding signals */

        void   (* sync_title) (NautilusWindow *window,
			       NautilusWindowSlot *slot);
        NautilusIconInfo * (* get_icon) (NautilusWindow *window,
                                         NautilusWindowSlot *slot);

        void   (* prompt_for_location) (NautilusWindow *window, const char *initial);
        void   (* close) (NautilusWindow *window);

        /* Signals used only for keybindings */
        _Bool    (* go_up)  (NautilusWindow *window, _Bool close);
	void     (* reload) (NautilusWindow *window);
} NautilusWindowClass;


GType         nautilus_window_get_type                     (void);
void          nautilus_window_close                        (NautilusWindow    *window);

void          nautilus_window_connect_content_view         (NautilusWindow    *window,
                                                            NautilusView      *view);
void          nautilus_window_disconnect_content_view      (NautilusWindow    *window,
                                                            NautilusView      *view);

void          nautilus_window_go_to                        (NautilusWindow    *window,
                                                            GFile             *location);
void          nautilus_window_go_to_full                   (NautilusWindow    *window,
                                                            GFile             *location,
                                                            NautilusWindowGoToCallback callback,
                                                            void              *user_data);
void          nautilus_window_new_tab                      (NautilusWindow    *window);

GtkUIManager *nautilus_window_get_ui_manager               (NautilusWindow    *window);
GtkActionGroup
             *nautilus_window_get_main_action_group        (NautilusWindow   *window);

NautilusNavigationState
             *nautilus_window_get_navigation_state         (NautilusWindow    *window);

void          nautilus_window_report_load_complete         (NautilusWindow *window,
                                                            NautilusView *view);

NautilusWindowSlot
             *nautilus_window_get_extra_slot               (NautilusWindow *window);

NautilusWindowShowHiddenFilesMode
              nautilus_window_get_hidden_files_mode        (NautilusWindow *window);
void          nautilus_window_set_hidden_files_mode        (NautilusWindow *window,
                                                            NautilusWindowShowHiddenFilesMode  mode);
void          nautilus_window_report_load_underway         (NautilusWindow *window,
                                                            NautilusView *view);
void          nautilus_window_view_visible                 (NautilusWindow *window,
                                                            NautilusView *view);
NautilusWindowSlot
             *nautilus_window_get_active_slot              (NautilusWindow *window);
void          nautilus_window_push_status                  (NautilusWindow *window,
                                                            const char *text);

void          nautilus_window_hide_sidebar                 (NautilusWindow *window);
void          nautilus_window_show_sidebar                 (NautilusWindow *window);

void          nautilus_window_split_view_on                (NautilusWindow *window);
void          nautilus_window_split_view_off               (NautilusWindow *window);
_Bool         nautilus_window_split_view_showing           (NautilusWindow *window);

int           nautilus_window_disable_chrome_mapping       (GValue       *value,
                                                            GVariant     *variant,
                                                            void         *user_data);

const char   *nautilus_window_get_sidebar_id                    (NautilusWindow *window);
void          nautilus_window_set_sidebar_id                    (NautilusWindow *window, const char *id);


_Bool         nautilus_window_get_show_sidebar                  (NautilusWindow *window);
void          nautilus_window_set_show_sidebar                  (NautilusWindow *window, _Bool show);


int           nautilus_window_get_sidebar_width                 (NautilusWindow *window);
void          nautilus_window_set_sidebar_width                 (NautilusWindow *window, int width);


const char   *nautilus_window_get_ignore_meta_view_id           (NautilusWindow *window);
void          nautilus_window_set_ignore_meta_view_id           (NautilusWindow *window, const char *id);
int           nautilus_window_get_ignore_meta_zoom_level        (NautilusWindow *window);
void          nautilus_window_set_ignore_meta_zoom_level        (NautilusWindow *window, int level);
GList        *nautilus_window_get_ignore_meta_visible_columns   (NautilusWindow *window);
void          nautilus_window_set_ignore_meta_visible_columns   (NautilusWindow *window, GList *list);
GList        *nautilus_window_get_ignore_meta_column_order      (NautilusWindow *window);
void          nautilus_window_set_ignore_meta_column_order      (NautilusWindow *window, GList *list);
const char   *nautilus_window_get_ignore_meta_sort_column       (NautilusWindow *window);
void          nautilus_window_set_ignore_meta_sort_column       (NautilusWindow *window, const char *column);
int           nautilus_window_get_ignore_meta_sort_direction    (NautilusWindow *window);
void          nautilus_window_set_ignore_meta_sort_direction    (NautilusWindow *window, int direction);
int           nautilus_window_get_ignore_meta_tighter_layout    (NautilusWindow *window);
void          nautilus_window_set_ignore_meta_tighter_layout    (NautilusWindow *window, int tighter);

#endif
