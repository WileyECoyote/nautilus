/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* fm-list-view.c - implementation of list view of directory.

   Copyright (C) 2000 Eazel, Inc.
   Copyright (C) 2001, 2002 Anders Carlsson <andersca@gnu.org>

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
   write to the Free Software Foundation, Inc., 51 Franklin Street - Suite 500,
   Boston, MA 02110-1335, USA.

   Authors: John Sullivan <sullivan@eazel.com>
            Anders Carlsson <andersca@gnu.org>
	    David Emory Watson <dwatson@cs.ucr.edu>
*/

#include <config.h>

#include <string.h>

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <libegg/eggtreemultidnd.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include <eel/eel.h>

#include <libnautilus-private/nautilus-file-attributes.h>
#include <libnautilus-private/nautilus-icon-info.h>
#include <libnautilus-private/nautilus-file.h>

#include <libnautilus-extension/nautilus-column-provider.h>
#include <libnautilus-private/nautilus-clipboard-monitor.h>
#include <libnautilus-private/nautilus-column-chooser.h>
#include <libnautilus-private/nautilus-column-utilities.h>
#include <libnautilus-private/nautilus-dnd.h>
#include <libnautilus-private/nautilus-file-dnd.h>
#include <libnautilus-private/nautilus-file-utilities.h>
#include <libnautilus-private/nautilus-global-preferences.h>
#include <libnautilus-private/nautilus-ui-utilities.h>
#include <libnautilus-private/nautilus-icon-dnd.h>
#include <libnautilus-private/nautilus-metadata.h>
#include <libnautilus-private/nautilus-module.h>
#include <libnautilus-private/nautilus-settings.h>
#include <libnautilus-private/nautilus-tree-view-drag-dest.h>
#include <libnautilus-private/nautilus-clipboard.h>

#include "nautilus-list-model.h"
#include "nautilus-error-reporting.h"
#include "nautilus-view-dnd.h"
#include "nautilus-view-factory.h"
#include "nautilus-window.h"

#include "nautilus-list-view.h"

#define DEBUG_FLAG NAUTILUS_DEBUG_LIST_VIEW
#include <libnautilus-private/nautilus-debug.h>

/* These are line-shorten'r and only apply with this file */
#define ClickPolicy view->details->click_policy
#define ListModel   list_view->details->model
#define ListTree    list_view->details->tree_view
#define TreeView    view->details->tree_view
#define ZoomLevel   list_view->details->zoom_level


struct NautilusListViewDetails {
    GtkTreeView              *tree_view;
    NautilusListModel        *model;
    GtkActionGroup           *list_action_group;
    unsigned int              list_merge_id;

    GtkTreeViewColumn        *file_name_column;
    int                       file_name_column_num;

    GtkCellRendererPixbuf    *pixbuf_cell;
    GtkCellRendererText      *file_name_cell;
    GList *cells;
    GtkCellEditable          *editable_widget;

    NautilusZoomLevel         zoom_level;

    NautilusTreeViewDragDest *drag_dest;

    GtkTreePath              *double_click_path[2]; /* Both clicks in a double click need to be on the same row */

    GtkTreePath              *new_selection_path;   /* Path of the new selection after removing a file */

    GtkTreePath              *hover_path;

    int                       click_policy;

    unsigned int              drag_button;
    int                       drag_x;
    int                       drag_y;

    _Bool                     drag_started;
    _Bool                     ignore_button_release;
    _Bool           row_selected_on_button_down;
    _Bool           menus_ready;
    _Bool           active;

    GHashTable     *columns;
    GtkWidget      *column_editor;

    char           *original_name;

    NautilusFile   *renaming_file;
    _Bool           rename_done;
    unsigned int    renaming_file_activate_timeout;

    unsigned long   clipboard_handler_id;

    unsigned int    last_sort_attr;

    EelTooltip     *tooltip;            /* provides custom window and timeout */
   _Bool            show_tooltips;      /* Enable or disable tooltips in this module */
    unsigned long   tooltip_handler_id; /* used to disable query callback */
    int             tooltip_flags;      /* Really a NautilusFileTooltipFlags */
};

struct SelectionForeachData {
	GList *list;
	GtkTreeSelection *selection;
};

/*
 * The row height should be large enough to not clip emblems.
 * Computing this would be costly, so we just choose a number
 * that works well with the set of emblems we've designed.
 */
#define LIST_VIEW_MINIMUM_ROW_HEIGHT	28

/* We wait two seconds after row is collapsed to unload the subdirectory */
#define COLLAPSE_TO_UNLOAD_DELAY 2

/* Wait for the rename to end when activating a file being renamed */
#define WAIT_FOR_RENAME_ON_ACTIVATE 200

static GdkCursor                *hand_cursor = NULL;

static GtkTargetList            *source_target_list = NULL;

static GList *list_view_get_selection                   (NautilusView     *view);
static GList *list_view_get_selection_for_transfer      (NautilusView     *view);
static void   nautilus_list_view_set_zoom_level                  (NautilusListView *view,
								  NautilusZoomLevel  new_level,
								  _Bool              always_set_level);
static void   nautilus_list_view_scale_font_size                 (NautilusListView *view,
								  NautilusZoomLevel  new_level);
static void   nautilus_list_view_scroll_to_file                  (NautilusListView *view,
								  NautilusFile      *file);
static void   nautilus_list_view_rename_callback                 (NautilusFile     *file,
								  GFile             *result_location,
								  GError            *error,
								  void              *callback_data);

static void   apply_columns_settings                             (NautilusListView *list_view,
                                                                  char            **column_order,
                                                                  char            **visible_columns);
static char **get_visible_columns                                (NautilusListView *list_view);
static char **get_default_visible_columns                        (NautilusListView *list_view);
static char **get_column_order                                   (NautilusListView *list_view);
static char **get_default_column_order                           (NautilusListView *list_view);

static void   set_columns_settings_from_metadata_and_preferences (NautilusListView *list_view);

G_DEFINE_TYPE (NautilusListView, nautilus_list_view, NAUTILUS_TYPE_VIEW);

static const char * default_search_visible_columns[] = {
   "name", "size", "type", "where", NULL
};

static const char * default_search_columns_order[] = {
   "name", "size", "type", "where", NULL
};

static const char * default_trash_visible_columns[] = {
	"name", "size", "type", "trashed_on", "trash_orig_path", NULL
};

static const char * default_trash_columns_order[] = {
	"name", "size", "type", "trashed_on", "trash_orig_path", NULL
};

static const char * default_recent_visible_columns[] = {
    "name", "size", "type", "date_accessed", NULL
};

static const char * default_recent_columns_order[] = {
    "name", "size", "type", "date_accessed", NULL
};

static const GtkActionEntry list_view_entries[] = {
  /* name, stock id, label */  { "Sort Column", NULL, N_("Sort Column") },
};

static const GtkRadioActionEntry sort_order_radio_entries[] = {
  { "Sort by Name", NULL,
    N_("By _Name"), NULL,
    N_("Keep icons sorted by name in rows"),
    NAUTILUS_FILE_SORT_BY_DISPLAY_NAME },
  { "Sort by Size", NULL,
    N_("By _Size"), NULL,
    N_("Keep icons sorted by size in rows"),
    NAUTILUS_FILE_SORT_BY_SIZE },
  { "Sort by Type", NULL,
    N_("By _Type"), NULL,
    N_("Keep icons sorted by type in rows"),
    NAUTILUS_FILE_SORT_BY_TYPE },
  { "Sort by Detailed Type", NULL,
    N_("By _Detailed Type"), NULL,
    N_("Keep icons sorted by detailed type in rows"),
    NAUTILUS_FILE_SORT_BY_DETAILED_TYPE },
  { "Sort by Modification Date", NULL,
    N_("By Modification _Date"), NULL,
    N_("Keep icons sorted by modification date in rows"),
    NAUTILUS_FILE_SORT_BY_MTIME },
  { "Sort by Trash Time", NULL,
    N_("By T_rash Time"), NULL,
    N_("Keep icons sorted by trash time in rows"),
    NAUTILUS_FILE_SORT_BY_TRASHED_TIME },
};
static const char*
get_default_sort_order (NautilusFile *file, _Bool *reversed)
{
	NautilusFileSortType default_sort_order;
	_Bool default_sort_reversed;
	const char *retval;
	const char *attributes[] = {
		"name", /* is really "manually" which doesn't apply to lists */
		"name",
		"size",
		"type",
		"date_modified",
		"date_accessed",
		"trashed_on",
		NULL
	};

	retval = nautilus_file_get_default_sort_attribute (file, reversed);

	if (retval == NULL) {

		default_sort_order = nautilus_settings_get_sort_order();
		default_sort_reversed = nautilus_settings_get_sort_reversed();

		retval = attributes[default_sort_order];
		*reversed = default_sort_reversed;
	}

	return retval;
}

static void
list_selection_changed_callback (GtkTreeSelection *selection, void *user_data)
{
	NautilusView *view;

	view = NAUTILUS_VIEW (user_data);

	nautilus_view_notify_selection_changed (view);
}

/* Move these to eel? */

static void
tree_selection_foreach_set_boolean (GtkTreeModel *model,
				    GtkTreePath  *path,
				    GtkTreeIter  *iter,
				    void         *callback_data)
{
	* (_Bool *) callback_data = TRUE;
}

static _Bool
tree_selection_not_empty (GtkTreeSelection *selection)
{
	_Bool not_empty;

	not_empty = FALSE;
	gtk_tree_selection_selected_foreach (selection,
					     tree_selection_foreach_set_boolean,
					     &not_empty);
	return not_empty;
}

static _Bool
tree_view_has_selection (GtkTreeView *view)
{
	return tree_selection_not_empty (gtk_tree_view_get_selection (view));
}

static void
preview_selected_items (NautilusListView *view)
{
	GList *file_list;

	file_list = list_view_get_selection (NAUTILUS_VIEW (view));

	if (file_list != NULL) {
		nautilus_view_preview_files (NAUTILUS_VIEW (view),
					     file_list, NULL);
		nautilus_file_list_free (file_list);
	}
}

static int
activate_selected_items (void *user_data)
{
  GList *file_list;

  NautilusListView *view = NAUTILUS_LIST_VIEW(user_data);

  file_list = list_view_get_selection (NAUTILUS_VIEW (view));

  if (view->details->renaming_file) {
    /* We're currently renaming a file, wait until the rename is
     *	   finished, or the activation uri will be wrong */
    if (view->details->renaming_file_activate_timeout == 0) {
      view->details->renaming_file_activate_timeout =
      g_timeout_add (WAIT_FOR_RENAME_ON_ACTIVATE, activate_selected_items, view);
    }
    return SOURCE_REMOVE;
  }

  EEL_SOURCE_REMOVE_IF_THEN_ZERO (view->details->renaming_file_activate_timeout);

  nautilus_view_activate_files (NAUTILUS_VIEW (view), file_list, 0, TRUE);

  nautilus_file_list_free (file_list);

  return SOURCE_REMOVE;
}

static void
activate_selected_items_alternate (NautilusListView *view,
				   NautilusFile *file,
				   _Bool open_in_tab)
{
	GList *file_list;
	NautilusWindowOpenFlags flags;

	flags = 0;

	if (nautilus_settings_get_always_use_browser()) {
		if (open_in_tab) {
			flags |= NAUTILUS_WINDOW_OPEN_FLAG_NEW_TAB;
		}
		else {
			flags |= NAUTILUS_WINDOW_OPEN_FLAG_NEW_WINDOW;
		}
	}
	else {
		flags |= NAUTILUS_WINDOW_OPEN_FLAG_CLOSE_BEHIND;
	}

	if (file != NULL) {
		nautilus_file_ref (file);
		file_list = g_list_prepend (NULL, file);
	}
	else {
		file_list = list_view_get_selection (NAUTILUS_VIEW (view));
	}
	nautilus_view_activate_files (NAUTILUS_VIEW (view),
				      file_list,
				      flags,
				      TRUE);
	nautilus_file_list_free (file_list);

}

static _Bool
button_event_modifies_selection (GdkEventButton *event)
{
	return (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) != 0;
}

static void
nautilus_list_view_did_not_drag (NautilusListView *view, GdkEventButton *event)
{
	GtkTreeView      *tree_view;
	GtkTreeSelection *selection;
	GtkTreePath      *path;

	tree_view = TreeView;
	selection = gtk_tree_view_get_selection (tree_view);

	if (gtk_tree_view_get_path_at_pos (tree_view, event->x, event->y,
					   &path, NULL, NULL, NULL))
        {
		if ((event->button == 1 || event->button == 2)
                    && ((event->state & GDK_CONTROL_MASK) != 0 || (event->state & GDK_SHIFT_MASK) == 0)
		    && view->details->row_selected_on_button_down)
                {
			if (!button_event_modifies_selection (event)) {
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_path (selection, path);
			} else {
				gtk_tree_selection_unselect_path (selection, path);
			}
		}

		if ((ClickPolicy == NAUTILUS_CLICK_POLICY_SINGLE) &&
                    !button_event_modifies_selection(event))
                {
			if (event->button == 1) {
				activate_selected_items (view);
			}
			else if (event->button == 2) {
				activate_selected_items_alternate (view, NULL, TRUE);
			}
		}
		gtk_tree_path_free (path);
	}

}

static void
drag_data_get_callback (GtkWidget *widget,
			GdkDragContext *context,
			GtkSelectionData *selection_data,
			unsigned int info,
			unsigned int time)
{
	GtkTreeView *tree_view;
	GtkTreeModel *model;
	GList *ref_list;

	tree_view = GTK_TREE_VIEW (widget);

	model = gtk_tree_view_get_model (tree_view);

	if (model == NULL) {
		return;
	}

	ref_list = g_object_get_data (G_OBJECT (context), "drag-info");

	if (ref_list == NULL) {
		return;
	}

	if (EGG_IS_TREE_MULTI_DRAG_SOURCE (model)) {
		egg_tree_multi_drag_source_drag_data_get (EGG_TREE_MULTI_DRAG_SOURCE (model),
							  ref_list,
							  selection_data);
	}
}

static void
filtered_selection_foreach (GtkTreeModel *model,
                            GtkTreePath  *path,
                            GtkTreeIter  *iter,
                            void         *data)
{
	struct SelectionForeachData *selection_data;
	GtkTreeIter parent;
	GtkTreeIter child;

	selection_data = data;

	/* If the parent folder is also selected, don't include this file in the
	 * file operation, since that would copy it to the toplevel target instead
	 * of keeping it as a child of the copied folder
	 */
	child = *iter;
	while (gtk_tree_model_iter_parent (model, &parent, &child)) {
		if (gtk_tree_selection_iter_is_selected (selection_data->selection,
							 &parent)) {
			return;
		}
		child = parent;
	}

	selection_data->list = g_list_prepend (selection_data->list,
					       gtk_tree_row_reference_new (model, path));
}

static GList *
get_filtered_selection_refs (GtkTreeView *tree_view)
{
	struct SelectionForeachData selection_data;

	selection_data.list = NULL;
	selection_data.selection = gtk_tree_view_get_selection (tree_view);

	gtk_tree_selection_selected_foreach (selection_data.selection,
					     filtered_selection_foreach,
					     &selection_data);
	return g_list_reverse (selection_data.list);
}

static void
ref_list_free (GList *ref_list)
{
	g_list_foreach (ref_list, (GFunc) gtk_tree_row_reference_free, NULL);
	g_list_free (ref_list);
}

static void
stop_drag_check (NautilusListView *view)
{
	view->details->drag_button = 0;
}

static cairo_surface_t*
get_drag_surface (NautilusListView *view)
{
    GtkTreeModel    *model;
    GtkTreePath     *path;
    GtkTreeIter      iter;
    cairo_surface_t *ret;
    GdkRectangle     cell_area;
    int              column;

    ret = NULL;

    if (gtk_tree_view_get_path_at_pos (TreeView,
                                       view->details->drag_x,
                                       view->details->drag_y,
                                      &path, NULL, NULL, NULL))
    {
        model = gtk_tree_view_get_model (TreeView);
        gtk_tree_model_get_iter (model, &iter, path);
        column = nautilus_list_model_get_column_id_from_zoom_level (view->details->zoom_level);
        gtk_tree_model_get (model, &iter, column, &ret, -1);

        gtk_tree_view_get_cell_area (TreeView,
                                     path,
                                     view->details->file_name_column,
                                     &cell_area);

        gtk_tree_path_free (path);
    }

    return ret;
}

static void
drag_begin_callback (GtkWidget        *widget,
                     GdkDragContext   *context,
                     NautilusListView *view)
{
    GList *ref_list;

    cairo_surface_t *surface;

    surface = get_drag_surface (view);

    if (surface && G_IS_OBJECT(surface)) {
        gtk_drag_set_icon_surface (context, surface);
        //cairo_surface_destroy (surface);
        g_object_unref (surface);
    }
    else {
        gtk_drag_set_icon_default (context);
    }

    stop_drag_check (view);
    view->details->drag_started = TRUE;

    ref_list = get_filtered_selection_refs (GTK_TREE_VIEW (widget));
    g_object_set_data_full (G_OBJECT (context),
                            "drag-info",
                            ref_list,
                            (GDestroyNotify)ref_list_free);
}

static int
motion_notify_callback (GtkWidget *widget,
			GdkEventMotion *event,
			void *callback_data)
{
    NautilusListView *view;

    view = NAUTILUS_LIST_VIEW (callback_data);

    if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (widget))) {
        return FALSE;
    }

    if (ClickPolicy == NAUTILUS_CLICK_POLICY_SINGLE) {
        GtkTreePath *old_hover_path;

        old_hover_path = view->details->hover_path;
        gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
                                       event->x, event->y,
                                       &view->details->hover_path,
                                       NULL, NULL, NULL);

        if ((old_hover_path != NULL) != (view->details->hover_path != NULL)) {
            if (view->details->hover_path != NULL) {
                gdk_window_set_cursor (gtk_widget_get_window (widget), hand_cursor);
            } else {
                gdk_window_set_cursor (gtk_widget_get_window (widget), NULL);
            }
        }

        if (old_hover_path != NULL) {
            gtk_tree_path_free (old_hover_path);
        }
    }

    if (view->details->drag_button != 0) {

        if (!source_target_list) {
            source_target_list = nautilus_list_model_get_drag_target_list ();
        }

        if (gtk_drag_check_threshold (widget,
                                      view->details->drag_x,
                                      view->details->drag_y,
                                      event->x,
                                      event->y))
        {
            gtk_drag_begin (widget,
                            source_target_list,
                            GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK | GDK_ACTION_ASK,
                            view->details->drag_button,
                            (GdkEvent*)event);
            }
            return TRUE;
    }

    return FALSE;
}

static _Bool
query_tooltip_callback (GtkWidget  *widget, int x, int y, _Bool kb_mode,
                        GtkTooltip *tooltip,
                        void       *user_data)
{
    NautilusListView *list_view;
    _Bool             ret_val;

    ret_val = FALSE;

    list_view = NAUTILUS_LIST_VIEW (user_data);

    /* First check tooltips are turned on */
    if (list_view->details->show_tooltips) {

        GtkTreeIter   iter;
        NautilusFile *file;
        GtkTreePath  *path = NULL;
        GtkTreeModel *model = GTK_TREE_MODEL (ListModel);

        if (gtk_tree_view_get_tooltip_context (GTK_TREE_VIEW (widget), &x, &y,
                                               kb_mode,
                                               &model, &path, &iter)) {

            if (!gtk_tree_view_is_blank_at_pos (GTK_TREE_VIEW (widget), x, y, NULL, NULL, NULL, NULL)) {
                gtk_tree_model_get (GTK_TREE_MODEL (ListModel),
                                    &iter,
                                    NAUTILUS_LIST_MODEL_FILE_COLUMN, &file,
                                    -1);
                if (file) {

                    char *tooltip_text;
                    tooltip_text = nautilus_file_construct_tooltip (file, list_view->details->tooltip_flags);
                    gtk_label_set_text (list_view->details->tooltip->label, tooltip_text);
                    gtk_tree_view_set_tooltip_cell (GTK_TREE_VIEW (widget), tooltip, path, NULL, NULL);
                    g_free (tooltip_text);

                    ret_val = TRUE;
                }
            }
        }
        gtk_tree_path_free (path);
    }

    return ret_val;
}

static _Bool
leave_notify_callback (GtkWidget *widget,
		       GdkEventCrossing *event,
		       void *callback_data)
{
	NautilusListView *view;

	view = NAUTILUS_LIST_VIEW (callback_data);

	if (ClickPolicy == NAUTILUS_CLICK_POLICY_SINGLE &&
	    view->details->hover_path != NULL) {
		gtk_tree_path_free (view->details->hover_path);
		view->details->hover_path = NULL;
	}

	return FALSE;
}

static _Bool
enter_notify_callback (GtkWidget *widget, GdkEventCrossing *event, void *callback_data)
{
    NautilusListView *view;

    view = NAUTILUS_LIST_VIEW (callback_data);

    if (ClickPolicy == NAUTILUS_CLICK_POLICY_SINGLE) {

        if (view->details->hover_path != NULL) {
            gtk_tree_path_free (view->details->hover_path);
        }

        gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
                                       event->x, event->y,
                                       &view->details->hover_path,
                                       NULL, NULL, NULL);

        if (view->details->hover_path != NULL) {
            gdk_window_set_cursor (gtk_widget_get_window (widget), hand_cursor);
        }
    }

    return FALSE;
}

static void
do_popup_menu (GtkWidget *widget, NautilusListView *view, GdkEventButton *event)
{

    if (tree_view_has_selection (GTK_TREE_VIEW (widget))) {
        nautilus_view_pop_up_selection_context_menu (NAUTILUS_VIEW (view), event);
    }
    else {
        nautilus_view_pop_up_background_context_menu (NAUTILUS_VIEW (view), event);
    }
}

static void
row_activated_callback (GtkTreeView *treeview, GtkTreePath *path,
			GtkTreeViewColumn *column, NautilusListView *view)
{
	activate_selected_items (view);
}

static void
columns_reordered_callback (AtkObject *atk,
                            void    *user_data)
{

    NautilusListView *view = NAUTILUS_LIST_VIEW (user_data);

    char **columns;
    GList *vis_columns = NULL;
    int i;
    NautilusFile *file = nautilus_view_get_directory_as_file (NAUTILUS_VIEW (view));

    columns = get_visible_columns (view);

    for (i = 0; columns[i] != NULL; ++i) {
        vis_columns = g_list_prepend (vis_columns, columns[i]);
    }

    vis_columns = g_list_reverse (vis_columns);

    GList *tv_list, *iter, *l;
    GList *list = NULL;

    tv_list = gtk_tree_view_get_columns (TreeView);

    for (iter = tv_list; iter != NULL; iter = iter->next) {
        for (l = vis_columns; l != NULL; l = l->next) {
            if (iter->data == g_hash_table_lookup (view->details->columns, l->data))
                list = g_list_prepend (list, (char *)l->data);
        }
    }
    list = g_list_reverse (list);

    if (nautilus_settings_get_ignore_view_metadata ())
        nautilus_window_set_ignore_meta_column_order (nautilus_view_get_nautilus_window (NAUTILUS_VIEW (view)), list);
    else
        nautilus_file_set_metadata_list (file,
                                     NAUTILUS_METADATA_KEY_LIST_VIEW_COLUMN_ORDER,
                                     list);
    g_list_free_full (list, g_free);
    g_free (columns);
    g_list_free (vis_columns);
    g_list_free (tv_list);
}

static _Bool
button_press_callback (GtkWidget *widget, GdkEventButton *event, void *callback_data)
{
    NautilusListView *view;
    GtkTreeView *tree_view;
    GtkTreePath *path;
    _Bool call_parent;
    GtkTreeSelection *selection;
    GtkWidgetClass *tree_view_class;
    signed long current_time;
    static signed long last_click_time = 0;
    static int click_count = 0;
    int double_click_time;
    int expander_size, horizontal_separator;
    _Bool on_expander;
    _Bool blank_click;

    view = NAUTILUS_LIST_VIEW (callback_data);
    tree_view = GTK_TREE_VIEW (widget);
    tree_view_class = GTK_WIDGET_GET_CLASS (tree_view);
    selection = gtk_tree_view_get_selection (tree_view);
    blank_click = FALSE;

    /* Don't handle extra mouse buttons here */
    if (event->button > 5) {
        return FALSE;
    }

    if (event->window != gtk_tree_view_get_bin_window (tree_view)) {
        return FALSE;
    }

    if (!nautilus_view_get_active (NAUTILUS_VIEW (view))) {
        NautilusWindowSlot *slot = nautilus_view_get_nautilus_window_slot (NAUTILUS_VIEW (view));
        nautilus_window_slot_make_hosting_pane_active (slot);
        return TRUE;
    }

    nautilus_list_model_set_drag_view
    (NAUTILUS_LIST_MODEL (gtk_tree_view_get_model (tree_view)),
     tree_view,
     event->x, event->y);

    g_object_get (G_OBJECT (gtk_widget_get_settings (widget)),
                  "gtk-double-click-time", &double_click_time,
                  NULL);

    /* Determine click count */
    current_time = eel_get_system_time ();
    if (current_time - last_click_time < double_click_time * 1000) {
        click_count++;
    }
    else {
        click_count = 0;
    }

    /* Stash time for next compare */
    last_click_time = current_time;

    /* Ignore double click if we are in single click mode */
    if (ClickPolicy == NAUTILUS_CLICK_POLICY_SINGLE && click_count >= 2) {
        return TRUE;
    }

    view->details->ignore_button_release = FALSE;

    call_parent = TRUE;
    if (gtk_tree_view_get_path_at_pos (tree_view,
        event->x, event->y, &path, NULL, NULL, NULL))
    {
        gtk_widget_style_get (widget,
                              "expander-size", &expander_size,
                              "horizontal-separator", &horizontal_separator,
                              NULL);
        /* TODO we should not hardcode this extra padding. It is
         * EXPANDER_EXTRA_PADDING from GtkTreeView.
         */
        expander_size += 4;
        on_expander = (event->x <= horizontal_separator / 2 +
        gtk_tree_path_get_depth (path) * expander_size);

        /* Keep track of path of last click so double clicks only happen
         * on the same item */
        if (event->type == GDK_BUTTON_PRESS && (event->button == 1 || event->button == 2))
        {
            if (view->details->double_click_path[1]) {
                gtk_tree_path_free (view->details->double_click_path[1]);
            }
            view->details->double_click_path[1] = view->details->double_click_path[0];
            view->details->double_click_path[0] = gtk_tree_path_copy (path);
        }

        if (event->type == GDK_2BUTTON_PRESS) {
            /* Double clicking does not trigger a D&D action. */
            view->details->drag_button = 0;
            if (view->details->double_click_path[1] && !on_expander &&
                gtk_tree_path_compare (view->details->double_click_path[0],
                                       view->details->double_click_path[1]) == 0)
            {
                /* NOTE: Activation can actually destroy the view if we're switching */
                if (!button_event_modifies_selection (event)) {
                    if ((event->button == 1 || event->button == 3)) {
                        activate_selected_items (view);
                    } else if (event->button == 2) {
                        activate_selected_items_alternate (view, NULL, TRUE);
                    }
                }
                else if (event->button == 1 && (event->state & GDK_SHIFT_MASK) != 0) {
                    NautilusFile *file;
                    file = nautilus_list_model_file_for_path (view->details->model, path);
                    if (file != NULL) {
                        activate_selected_items_alternate (view, file, TRUE);
                        nautilus_file_unref (file);
                    }
                }
            }
            else {
                tree_view_class->button_press_event (widget, event);
            }
        }
        else {

            /* We're going to filter out some situations where
             * we can't let the default code run because all
             * but one row would be would be deselected. We don't
             * want that; we want the right click menu or single
             * click to apply to everything that's currently selected. */

            if (event->button == 3) {
                blank_click =
                (!gtk_tree_selection_path_is_selected (selection, path) &&
                gtk_tree_view_is_blank_at_pos (tree_view, event->x, event->y, NULL, NULL, NULL, NULL));
            }

            if (event->button == 3 &&
                (blank_click || gtk_tree_selection_path_is_selected (selection, path)))
            {
                call_parent = FALSE;
            }

            if ((event->button == 1 || event->button == 2) &&
                ((event->state & GDK_CONTROL_MASK) != 0 ||
                (event->state & GDK_SHIFT_MASK) == 0))
            {

                view->details->row_selected_on_button_down = gtk_tree_selection_path_is_selected (selection, path);

                if (view->details->row_selected_on_button_down) {
                    call_parent = on_expander;
                    view->details->ignore_button_release = call_parent;
                }
                else if ((event->state & GDK_CONTROL_MASK) != 0) {
                    GList *selected_rows;
                    GList *l;

                    call_parent = FALSE;
                    if ((event->state & GDK_SHIFT_MASK) != 0) {
                        GtkTreePath *cursor;
                        gtk_tree_view_get_cursor (tree_view, &cursor, NULL);
                        if (cursor != NULL) {
                            gtk_tree_selection_select_range (selection, cursor, path);
                        } else {
                            gtk_tree_selection_select_path (selection, path);
                        }
                    } else {
                        gtk_tree_selection_select_path (selection, path);
                    }
                    selected_rows = gtk_tree_selection_get_selected_rows (selection, NULL);

                    /* This unselects everything */
                    gtk_tree_view_set_cursor (tree_view, path, NULL, FALSE);

                    /* So select it again */
                    l = selected_rows;
                    while (l != NULL) {
                        GtkTreePath *p = l->data;
                        l = l->next;
                        gtk_tree_selection_select_path (selection, p);
                        gtk_tree_path_free (p);
                    }
                    g_list_free (selected_rows);
                }
                else {
                    view->details->ignore_button_release = on_expander;
                }
            }

            if (call_parent) {
                g_signal_handlers_block_by_func (tree_view,
                                                 row_activated_callback,
                                                 view);

                tree_view_class->button_press_event (widget, event);

                g_signal_handlers_unblock_by_func (tree_view,
                                                   row_activated_callback,
                                                   view);
            }
            else if (gtk_tree_selection_path_is_selected (selection, path)) {
                gtk_widget_grab_focus (widget);
            }

            if ((event->button == 1 || event->button == 2) &&
                event->type == GDK_BUTTON_PRESS) {
                view->details->drag_started = FALSE;
            view->details->drag_button = event->button;
            view->details->drag_x = event->x;
            view->details->drag_y = event->y;
                }

                if (event->button == 3) {
                    if (blank_click) {
                        gtk_tree_selection_unselect_all (selection);
                    }
                    do_popup_menu (widget, view, event);
                }
        }

        gtk_tree_path_free (path);
    }
    else {
        if ((event->button == 1 || event->button == 2)  &&
            event->type == GDK_BUTTON_PRESS)
        {
            if (view->details->double_click_path[1]) {
                gtk_tree_path_free (view->details->double_click_path[1]);
            }
            view->details->double_click_path[1] = view->details->double_click_path[0];
            view->details->double_click_path[0] = NULL;
        }
        /* Deselect if people click outside any row. It's OK to
         *	   let default code run; it won't reselect anything. */
        gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (tree_view));
        tree_view_class->button_press_event (widget, event);

        if (event->button == 3) {
            do_popup_menu (widget, view, event);
        }
    }

    /* We chained to the default handler in this method, so never
     * let the default handler run */
    return TRUE;
}

static _Bool
button_release_callback (GtkWidget *widget,
			 GdkEventButton *event,
			 void *callback_data)
{
	NautilusListView *view;

	view = NAUTILUS_LIST_VIEW (callback_data);

	if (event->button == view->details->drag_button) {
		stop_drag_check (view);
		if (!view->details->drag_started &&
		    !view->details->ignore_button_release) {
			nautilus_list_view_did_not_drag (view, event);
		}
	}
	return FALSE;
}

static _Bool
popup_menu_callback (GtkWidget *widget, void *callback_data)
{
 	NautilusListView *view;

	view = NAUTILUS_LIST_VIEW (callback_data);

	do_popup_menu (widget, view, NULL);

	return TRUE;
}

static void
subdirectory_done_loading_callback (NautilusDirectory *directory, NautilusListView *view)
{
	nautilus_list_model_subdirectory_done_loading (view->details->model, directory);
}

static void
row_expanded_callback (GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *path, void *callback_data)
{
  NautilusListView *view;
  NautilusDirectory *directory;

  view = NAUTILUS_LIST_VIEW (callback_data);

  if (nautilus_list_model_load_subdirectory (view->details->model, path, &directory)) {
    char *uri;

    uri = nautilus_directory_get_uri (directory);
    DEBUG ("Row expaded callback for uri %s", uri);
    g_free (uri);

    nautilus_view_add_subdirectory (NAUTILUS_VIEW (view), directory);

    if (nautilus_directory_are_all_files_seen (directory)) {
      nautilus_list_model_subdirectory_done_loading (view->details->model, directory);
    }
    else {
      g_signal_connect_object (directory, "done_loading",
                               G_CALLBACK (subdirectory_done_loading_callback),
                               view, 0);
    }

    nautilus_directory_unref (directory);
  }
}

struct UnloadDelayData {
	NautilusFile *file;
	NautilusDirectory *directory;
	NautilusListView *view;
};

/* Is really _Bool but glib errently defines gboolean as int */
static int
unload_file_timeout (void *data)
{
	struct UnloadDelayData *unload_data = data;
	GtkTreeIter             iter;
	NautilusListModel      *model;
	GtkTreePath            *path;

	if (unload_data->view != NULL) {
		model = unload_data->view->details->model;

		if (nautilus_list_model_get_tree_iter_from_file (model,
							   unload_data->file,
							   unload_data->directory,
							   &iter)) {
			path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter);
			if (!gtk_tree_view_row_expanded (unload_data->TreeView,
							 path)) {
				nautilus_list_model_unload_subdirectory (model, &iter);
			}
			gtk_tree_path_free (path);
		}
	}

	eel_remove_weak_pointer (&unload_data->view);

	if (unload_data->directory) {
		nautilus_directory_unref (unload_data->directory);
	}
	nautilus_file_unref (unload_data->file);
	g_free (unload_data);
	return SOURCE_REMOVE;
}

static void
row_collapsed_callback (GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *path, void *callback_data)
{
 	NautilusListView *view;
 	NautilusFile *file;
	NautilusDirectory *directory;
	GtkTreeIter parent;
	struct UnloadDelayData *unload_data;
	GtkTreeModel *model;
	char *uri;

	view = NAUTILUS_LIST_VIEW (callback_data);

	model = GTK_TREE_MODEL (view->details->model);

	gtk_tree_model_get (model, iter,
			    NAUTILUS_LIST_MODEL_FILE_COLUMN, &file,
			    -1);

	directory = NULL;
	if (gtk_tree_model_iter_parent (model, &parent, iter)) {
		gtk_tree_model_get (model, &parent,
				    NAUTILUS_LIST_MODEL_SUBDIRECTORY_COLUMN, &directory,
				    -1);
	}


	uri = nautilus_file_get_uri (file);
	DEBUG ("Row collapsed callback for uri %s", uri);
	g_free (uri);

	unload_data = g_new (struct UnloadDelayData, 1);
	unload_data->view = view;
	unload_data->file = file;
	unload_data->directory = directory;

	eel_add_weak_pointer (&unload_data->view);

	g_timeout_add_seconds (COLLAPSE_TO_UNLOAD_DELAY,
			       unload_file_timeout,
			       unload_data);
}

static void
subdirectory_unloaded_callback (NautilusListModel *model,
				NautilusDirectory *directory,
				void *callback_data)
{
	NautilusListView *view;

	g_return_if_fail (NAUTILUS_IS_LIST_MODEL (model));
	g_return_if_fail (NAUTILUS_IS_DIRECTORY (directory));

	view = NAUTILUS_LIST_VIEW(callback_data);

	g_signal_handlers_disconnect_by_func (directory,
					      G_CALLBACK (subdirectory_done_loading_callback),
					      view);
	nautilus_view_remove_subdirectory (NAUTILUS_VIEW (view), directory);
}

static _Bool
key_press_callback (GtkWidget *widget, GdkEventKey *event, void *callback_data)
{
	NautilusView *view;
	GdkEventButton button_event = { 0 };
	_Bool handled;
	GtkTreeView *tree_view;
	GtkTreePath *path;

	tree_view = GTK_TREE_VIEW (widget);

	view = NAUTILUS_VIEW (callback_data);
	handled = FALSE;

	switch (event->keyval) {
	case GDK_KEY_F10:
		if (event->state & GDK_CONTROL_MASK) {
			nautilus_view_pop_up_background_context_menu (view, &button_event);
			handled = TRUE;
		}
		break;
	case GDK_KEY_Right:
		gtk_tree_view_get_cursor (tree_view, &path, NULL);
		if (path) {
			gtk_tree_view_expand_row (tree_view, path, FALSE);
			gtk_tree_path_free (path);
		}
		handled = TRUE;
		break;
	case GDK_KEY_Left:
		gtk_tree_view_get_cursor (tree_view, &path, NULL);
		if (path) {
			if (!gtk_tree_view_collapse_row (tree_view, path)) {
				/* if the row is already collapsed or doesn't have any children,
				 * jump to the parent row instead.
				 */
				if ((gtk_tree_path_get_depth (path) > 1) && gtk_tree_path_up (path)) {
					gtk_tree_view_set_cursor (tree_view, path, NULL, FALSE);
				}
			}

			gtk_tree_path_free (path);
		}
		handled = TRUE;
		break;
	case GDK_KEY_space:
		if (event->state & GDK_CONTROL_MASK) {
			handled = FALSE;
			break;
		}
		if (!gtk_widget_has_focus (GTK_WIDGET (NAUTILUS_LIST_VIEW (view)->details->tree_view))) {
			handled = FALSE;
			break;
		}
		if ((event->state & GDK_SHIFT_MASK) != 0) {
			activate_selected_items_alternate (NAUTILUS_LIST_VIEW (view), NULL, TRUE);
		} else {
			preview_selected_items (NAUTILUS_LIST_VIEW (view));
		}
		handled = TRUE;
		break;
	case GDK_KEY_Return:
	case GDK_KEY_KP_Enter:
		if ((event->state & GDK_SHIFT_MASK) != 0) {
			activate_selected_items_alternate (NAUTILUS_LIST_VIEW (view), NULL, TRUE);
		} else {
			activate_selected_items (NAUTILUS_LIST_VIEW (view));
		}
		handled = TRUE;
		break;
	case GDK_KEY_v:
		/* Eat Control + v to not enable type ahead */
		if ((event->state & GDK_CONTROL_MASK) != 0) {
			handled = TRUE;
		}
		break;

	default:
		handled = FALSE;
	}

	return handled;
}

static void
list_view_reveal_selection (NautilusView *view)
{
  GList *selection;

  g_return_if_fail (NAUTILUS_IS_LIST_VIEW (view));

  selection = nautilus_view_get_selection (view);

  /* Make sure at least one of the selected items is scrolled into view */
  if (selection != NULL) {

    NautilusListView *list_view;
    NautilusFile     *file;
    GtkTreePath      *path;
    GtkTreeIter       iter;

    list_view = NAUTILUS_LIST_VIEW (view);
    file = selection->data;
    if (nautilus_list_model_get_first_iter_for_file (ListModel, file, &iter)) {
      path = gtk_tree_model_get_path (GTK_TREE_MODEL (ListModel), &iter);

      gtk_tree_view_scroll_to_cell (ListTree, path, NULL, FALSE, 0.0, 0.0);

      gtk_tree_path_free (path);
    }
  }

  nautilus_file_list_free (selection);
}

static _Bool
sort_criterion_changes_due_to_user (GtkTreeView *tree_view)
{
	GList *columns, *p;
	GtkTreeViewColumn *column;
	GSignalInvocationHint *ihint;
	_Bool ret_val;

	ret_val = FALSE;

	columns = gtk_tree_view_get_columns (tree_view);
	for (p = columns; p != NULL; p = p->next) {
		column = p->data;
		ihint = g_signal_get_invocation_hint (column);
		if (ihint != NULL) {
			ret_val = TRUE;
			break;
		}
	}
	g_list_free (columns);

	return ret_val;
}

static void
sort_column_changed_callback (GtkTreeSortable *sortable,
                              NautilusListView *view)
{
	NautilusFile *file;
	int sort_column_id, default_sort_column_id;
	GtkSortType reversed;
	unsigned int sort_attr, default_sort_attr;
	char *reversed_attr, *default_reversed_attr;
	_Bool default_sort_reversed;

	file = nautilus_view_get_directory_as_file (NAUTILUS_VIEW (view));

	gtk_tree_sortable_get_sort_column_id (sortable, &sort_column_id, &reversed);
	sort_attr = nautilus_list_model_get_attribute_from_sort_column_id (view->details->model, sort_column_id);

	default_sort_column_id = nautilus_list_model_get_sort_column_id_from_attribute (view->details->model,
										  g_quark_from_string (get_default_sort_order (file, &default_sort_reversed)));
	default_sort_attr = nautilus_list_model_get_attribute_from_sort_column_id (view->details->model, default_sort_column_id);

    if (nautilus_settings_get_ignore_view_metadata ())
        nautilus_window_set_ignore_meta_sort_column (nautilus_view_get_nautilus_window (NAUTILUS_VIEW (view)),
                                                 g_quark_to_string (sort_attr));
    else
        nautilus_file_set_metadata (file, NAUTILUS_METADATA_KEY_LIST_VIEW_SORT_COLUMN,
                                g_quark_to_string (default_sort_attr), g_quark_to_string (sort_attr));

	default_reversed_attr = (default_sort_reversed ? "true" : "false");

	if (view->details->last_sort_attr != sort_attr &&
	    sort_criterion_changes_due_to_user (TreeView)) {
		/* at this point, the sort order is always GTK_SORT_ASCENDING, if the sort column ID
		 * switched. Invert the sort order, if it's the default criterion with a reversed preference,
		 * or if it makes sense for the attribute (i.e. date). */
		if (sort_attr == default_sort_attr) {
			/* use value from preferences */
			reversed = eel_settings_get_boolean (nautilus_preferences,
							   NAUTILUS_PREFERENCES_DEFAULT_SORT_IN_REVERSE_ORDER);
		} else {
			reversed = nautilus_file_is_date_sort_attribute_q (sort_attr);
		}

		if (reversed) {
			g_signal_handlers_block_by_func (sortable, sort_column_changed_callback, view);
			gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (view->details->model),
							      sort_column_id,
							      GTK_SORT_DESCENDING);
			g_signal_handlers_unblock_by_func (sortable, sort_column_changed_callback, view);
		}
	}

    if (nautilus_settings_get_ignore_view_metadata ()) {
        nautilus_window_set_ignore_meta_sort_direction (nautilus_view_get_nautilus_window (NAUTILUS_VIEW (view)),
                                                    reversed ? SORT_DESCENDING : SORT_ASCENDING);
    } else {
        reversed_attr = (reversed ? "true" : "false");
        nautilus_file_set_metadata (file, NAUTILUS_METADATA_KEY_LIST_VIEW_SORT_REVERSED,
                                default_reversed_attr, reversed_attr);
    }

	/* Make sure selected item(s) is visible after sort */
	list_view_reveal_selection (NAUTILUS_VIEW (view));

	view->details->last_sort_attr = sort_attr;
}

static void
editable_focus_out_cb (GtkWidget *widget, GdkEvent *event, void *user_data)
{
	NautilusListView *view = user_data;

	view->details->editable_widget = NULL;

	nautilus_view_set_is_renaming (NAUTILUS_VIEW (view), FALSE);
	nautilus_view_unfreeze_updates (NAUTILUS_VIEW (view));
}

static void
cell_renderer_editing_started_cb (GtkCellRenderer *renderer,
				  GtkCellEditable *editable,
				  const char *path_str,
				  NautilusListView *list_view)
{
	GtkEntry *entry;

	entry = GTK_ENTRY (editable);
	list_view->details->editable_widget = editable;

	/* Free a previously allocated original_name */
	g_free (list_view->details->original_name);

	list_view->details->original_name = g_strdup (gtk_entry_get_text (entry));

	g_signal_connect (entry, "focus-out-event",
			  G_CALLBACK (editable_focus_out_cb), list_view);

	nautilus_clipboard_set_up_editable
		(GTK_EDITABLE (entry),
		 nautilus_view_get_ui_manager (NAUTILUS_VIEW (list_view)),
		 FALSE);
}

static void
cell_renderer_editing_canceled (GtkCellRendererText *cell,
				NautilusListView    *view)
{
	view->details->editable_widget = NULL;

	nautilus_view_set_is_renaming (NAUTILUS_VIEW (view), FALSE);
	nautilus_view_unfreeze_updates (NAUTILUS_VIEW (view));
}

static void
cell_renderer_edited (GtkCellRendererText *cell,
		      const char          *path_str,
		      const char          *new_text,
		      NautilusListView    *view)
{
    GtkTreePath *path;
    NautilusFile *file;
    GtkTreeIter iter;

    view->details->editable_widget = NULL;
    nautilus_view_set_is_renaming (NAUTILUS_VIEW (view), FALSE);

    /* Don't allow a rename with an empty string. Revert to original
     * without notifying the user.
     */
    if (new_text[0] == '\0') {
        g_object_set (G_OBJECT (view->details->file_name_cell),
                      "editable", FALSE,
                      NULL);
        nautilus_view_unfreeze_updates (NAUTILUS_VIEW (view));
        return;
    }

    path = gtk_tree_path_new_from_string (path_str);

    gtk_tree_model_get_iter (GTK_TREE_MODEL (view->details->model), &iter, path);

    gtk_tree_path_free (path);

    gtk_tree_model_get (GTK_TREE_MODEL (view->details->model),
                        &iter,
                        NAUTILUS_LIST_MODEL_FILE_COLUMN, &file,
                        -1);

    /* Only rename if name actually changed */
    if (strcmp (new_text, view->details->original_name) != 0) {
        view->details->renaming_file = nautilus_file_ref (file);
        view->details->rename_done = FALSE;
        nautilus_rename_file (file, new_text, nautilus_list_view_rename_callback, g_object_ref (view));
        g_free (view->details->original_name);
        view->details->original_name = g_strdup (new_text);
    }

    nautilus_file_unref (file);

    /*We're done editing - make the filename-cells readonly again.*/
    g_object_set (G_OBJECT (view->details->file_name_cell),
                  "editable", FALSE,
                  NULL);

    nautilus_view_unfreeze_updates (NAUTILUS_VIEW (view));
}

static char *
get_root_uri_callback (NautilusTreeViewDragDest *dest,
		       void    *user_data)
{
	NautilusListView *view;

	view = NAUTILUS_LIST_VIEW (user_data);

	return nautilus_view_get_uri (NAUTILUS_VIEW (view));
}

static NautilusFile *
get_file_for_path_callback (NautilusTreeViewDragDest *dest,
			    GtkTreePath *path,
			    void    *user_data)
{
	NautilusListView *view;

	view = NAUTILUS_LIST_VIEW (user_data);

	return nautilus_list_model_file_for_path (view->details->model, path);
}

/* Handles an URL received from Mozilla */
static void
list_view_handle_netscape_url (NautilusTreeViewDragDest *dest, const char *encoded_url,
			       const char *target_uri, GdkDragAction action, int x, int y, NautilusListView *view)
{
	nautilus_view_handle_netscape_url_drop (NAUTILUS_VIEW (view),
						encoded_url, target_uri, action, x, y);
}

static void
list_view_handle_uri_list (NautilusTreeViewDragDest *dest, const char *item_uris,
			   const char *target_uri,
			   GdkDragAction action, int x, int y, NautilusListView *view)
{
	nautilus_view_handle_uri_list_drop (NAUTILUS_VIEW (view),
					    item_uris, target_uri, action, x, y);
}

static void
list_view_handle_text (NautilusTreeViewDragDest *dest, const char *text,
		       const char *target_uri,
		       GdkDragAction action, int x, int y, NautilusListView *view)
{
	nautilus_view_handle_text_drop (NAUTILUS_VIEW (view),
					text, target_uri, action, x, y);
}

static void
list_view_handle_raw (NautilusTreeViewDragDest *dest, const char *raw_data,
		      int length, const char *target_uri, const char *direct_save_uri,
		      GdkDragAction action, int x, int y, NautilusListView *view)
{
	nautilus_view_handle_raw_drop (NAUTILUS_VIEW (view),
				       raw_data, length, target_uri, direct_save_uri,
				       action, x, y);
}

static void
move_copy_items_callback (NautilusTreeViewDragDest *dest,
			  const GList *item_uris,
			  const char *target_uri,
			  unsigned int action,
			  int x,
			  int y,
			  void    *user_data)

{
	NautilusView *view = user_data;

	nautilus_clipboard_clear_if_colliding_uris (GTK_WIDGET (view),
						    item_uris,
						    nautilus_view_get_copied_files_atom (view));
	nautilus_view_move_copy_items (view,
				       item_uris,
				       NULL,
				       target_uri,
				       action,
				       x, y);
}

static void
column_header_menu_toggled (GtkCheckMenuItem *menu_item,
                            NautilusListView *list_view)
{
	NautilusFile *file;
	char **visible_columns;
	char **column_order;
	const char *column;
	GList *list = NULL;
	GList *l;
	int i;

	file = nautilus_view_get_directory_as_file (NAUTILUS_VIEW (list_view));
	visible_columns = get_visible_columns (list_view);
	column_order = get_column_order (list_view);
	column = g_object_get_data (G_OBJECT (menu_item), "column-name");

	for (i = 0; visible_columns[i] != NULL; ++i) {
		list = g_list_prepend (list, visible_columns[i]);
	}

	if (gtk_check_menu_item_get_active (menu_item)) {
		list = g_list_prepend (list, g_strdup (column));
	} else {
		l = g_list_find_custom (list, column, (GCompareFunc) g_strcmp0);
		list = g_list_delete_link (list, l);
	}

	list = g_list_reverse (list);

    if (nautilus_settings_get_ignore_view_metadata ())
        nautilus_window_set_ignore_meta_visible_columns (nautilus_view_get_nautilus_window (NAUTILUS_VIEW (list_view)), list);
    else
        nautilus_file_set_metadata_list (file,
                                     NAUTILUS_METADATA_KEY_LIST_VIEW_VISIBLE_COLUMNS,
                                     list);

	g_free (visible_columns);

	visible_columns = g_new0 (char *, g_list_length (list) + 1);
	for (i = 0, l = list; l != NULL; ++i, l = l->next) {
		visible_columns[i] = l->data;
	}

	/* set view values ourselves, as new metadata could not have been
	 * updated yet.
	 */
	apply_columns_settings (list_view, column_order, visible_columns);

	g_list_free (list);
	g_strfreev (column_order);
	g_strfreev (visible_columns);
}

static void
column_header_menu_use_default (GtkMenuItem *menu_item,
                                NautilusListView *list_view)
{
	NautilusFile *file;
	char **default_columns;
	char **default_order;

	file = nautilus_view_get_directory_as_file (NAUTILUS_VIEW (list_view));

    g_signal_handlers_block_by_func (ListTree,
                                     columns_reordered_callback,
                                     list_view);

    if (nautilus_settings_get_ignore_view_metadata ()) {
        NautilusWindow *window = nautilus_view_get_nautilus_window (NAUTILUS_VIEW (list_view));
        nautilus_window_set_ignore_meta_visible_columns (window, NULL);
        nautilus_window_set_ignore_meta_column_order (window, NULL);
    } else {
        nautilus_file_set_metadata_list (file, NAUTILUS_METADATA_KEY_LIST_VIEW_COLUMN_ORDER, NULL);
        nautilus_file_set_metadata_list (file, NAUTILUS_METADATA_KEY_LIST_VIEW_VISIBLE_COLUMNS, NULL);
    }

    default_columns = get_default_visible_columns (list_view);

    default_order = get_default_column_order (list_view);

	/* set view values ourselves, as new metadata could not have been
	 * updated yet.
	 */
	apply_columns_settings (list_view, default_order, default_columns);

    g_signal_handlers_unblock_by_func (ListTree,
                                       columns_reordered_callback,
                                       list_view);

	g_strfreev (default_columns);
	g_strfreev (default_order);
}

static void
column_header_menu_disable_sort (GtkMenuItem *menu_item,
                                 NautilusListView *list_view)
{
    _Bool active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menu_item));

    nautilus_list_model_set_temporarily_disable_sort (ListModel, active);
}

static _Bool
column_header_clicked (GtkWidget        *column_button,
                       GdkEventButton   *event,
                       NautilusListView *list_view)
{
	NautilusFile *file;
	char **visible_columns;
	char **column_order;
	GList *all_columns;
	GHashTable *visible_columns_hash;
	int i;
	GList *l;
	GtkWidget *menu;
	GtkWidget *menu_item;

	if (event->button != GDK_BUTTON_SECONDARY) {
		return FALSE;
	}

	file = nautilus_view_get_directory_as_file (NAUTILUS_VIEW (list_view));

	visible_columns = get_visible_columns (list_view);
	column_order    = get_column_order (list_view);

	all_columns = nautilus_get_columns_for_file (file);
	all_columns = nautilus_sort_columns (all_columns, column_order);

	/* hash table to lookup if a given column should be visible */
	visible_columns_hash = g_hash_table_new_full (g_str_hash,
	                                              g_str_equal,
	                                              (GDestroyNotify) g_free,
	                                              (GDestroyNotify) g_free);
	if (visible_columns != NULL) {
		for (i = 0; visible_columns[i] != NULL; ++i) {
			g_hash_table_insert (visible_columns_hash,
			                     g_ascii_strdown (visible_columns[i], -1),
			                     g_ascii_strdown (visible_columns[i], -1));
		}
	}

	menu = gtk_menu_new ();

	for (l = all_columns; l != NULL; l = l->next) {
		char *name;
		char *label;
		char *lowercase;

		g_object_get (G_OBJECT (l->data),
		              "name", &name,
		              "label", &label,
		              NULL);
		lowercase = g_ascii_strdown (name, -1);

		menu_item = gtk_check_menu_item_new_with_label (label);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

		g_object_set_data_full (G_OBJECT (menu_item),
		                        "column-name", name, g_free);

		if (g_hash_table_lookup (visible_columns_hash, lowercase) != NULL) {
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item),
			                                TRUE);
        /*  Don't allow hiding the only visible column */
            if (g_hash_table_size (visible_columns_hash) == 1)
                gtk_widget_set_sensitive (GTK_WIDGET (menu_item), FALSE);
		}

		g_signal_connect (menu_item,
		                  "toggled",
		                  G_CALLBACK (column_header_menu_toggled),
		                  list_view);

		g_free (lowercase);
		g_free (label);
	}

	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	menu_item = gtk_menu_item_new_with_label (_("Use Default"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	g_signal_connect (menu_item,
	                  "activate",
	                  G_CALLBACK (column_header_menu_use_default),
	                  list_view);

    menu_item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    menu_item = gtk_check_menu_item_new_with_label (_("Temporarily disable auto-sort"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item),
                                    nautilus_list_model_get_temporarily_disable_sort (ListModel));

    g_signal_connect (menu_item,
                      "activate",
                      G_CALLBACK (column_header_menu_disable_sort),
                      list_view);

	gtk_widget_show_all (menu);
	gtk_menu_popup_for_device (GTK_MENU (menu),
	                           gdk_event_get_device ((GdkEvent *) event),
	                           NULL, NULL, NULL, NULL, NULL,
	                           event->button, event->time);

	g_hash_table_destroy (visible_columns_hash);
	nautilus_column_list_free (all_columns);
	g_strfreev (column_order);
	g_strfreev (visible_columns);

	return TRUE;
}

static void
apply_columns_settings (NautilusListView *list_view,
                        char **column_order,
                        char **visible_columns)
{
        GList *all_columns;
        NautilusFile *file;
        GList *old_view_columns, *view_columns;
        GHashTable *visible_columns_hash;
        GtkTreeViewColumn *prev_view_column;
        GList *l;
        int i;

        file = nautilus_view_get_directory_as_file (NAUTILUS_VIEW (list_view));

        /* prepare ordered list of view columns using column_order and visible_columns */
        view_columns = NULL;

        all_columns = nautilus_get_columns_for_file (file);
        all_columns = nautilus_sort_columns (all_columns, column_order);

        /* hash table to lookup if a given column should be visible */
        visible_columns_hash = g_hash_table_new_full (g_str_hash,
                                                      g_str_equal,
                                                      (GDestroyNotify) g_free,
                                                      (GDestroyNotify) g_free);
        for (i = 0; visible_columns[i] != NULL; ++i) {
                g_hash_table_insert (visible_columns_hash,
                                     g_ascii_strdown (visible_columns[i], -1),
                                     g_ascii_strdown (visible_columns[i], -1));
        }

        for (l = all_columns; l != NULL; l = l->next) {
                char *name;
                char *lowercase;

                g_object_get (G_OBJECT (l->data), "name", &name, NULL);
                lowercase = g_ascii_strdown (name, -1);

                if (g_hash_table_lookup (visible_columns_hash, lowercase) != NULL) {
                        GtkTreeViewColumn *view_column;

                        view_column = g_hash_table_lookup (list_view->details->columns, name);
                        if (view_column != NULL) {
                                view_columns = g_list_prepend (view_columns, view_column);
                        }
                }

                g_free (name);
                g_free (lowercase);
        }

        g_hash_table_destroy (visible_columns_hash);
        nautilus_column_list_free (all_columns);

        view_columns = g_list_reverse (view_columns);

        /* remove columns that are not present in the configuration */
        old_view_columns = gtk_tree_view_get_columns (ListTree);
        for (l = old_view_columns; l != NULL; l = l->next) {
                if (g_list_find (view_columns, l->data) == NULL) {
                        gtk_tree_view_remove_column (ListTree, l->data);
                }
        }
        g_list_free (old_view_columns);

        /* append new columns from the configuration */
        old_view_columns = gtk_tree_view_get_columns (ListTree);
        for (l = view_columns; l != NULL; l = l->next) {
                if (g_list_find (old_view_columns, l->data) == NULL) {
                        gtk_tree_view_append_column (ListTree, l->data);
                }
        }
        g_list_free (old_view_columns);

        /* place columns in the correct order */
        prev_view_column = NULL;
        for (l = view_columns; l != NULL; l = l->next) {
                gtk_tree_view_move_column_after (ListTree, l->data, prev_view_column);
                prev_view_column = l->data;
        }
        g_list_free (view_columns);
}

static void
filename_cell_data_func (GtkTreeViewColumn  *column,
                         GtkCellRenderer    *renderer,
                         GtkTreeModel       *model,
                         GtkTreeIter        *iter,
                         NautilusListView   *view)
{
	char          *text;
	GtkTreePath   *path;
	PangoUnderline underline;

	gtk_tree_model_get (model, iter,
			    view->details->file_name_column_num, &text,
			    -1);

	if (ClickPolicy == NAUTILUS_CLICK_POLICY_SINGLE) {
		path = gtk_tree_model_get_path (model, iter);

		if (view->details->hover_path == NULL ||
		    gtk_tree_path_compare (path, view->details->hover_path)) {
			underline = PANGO_UNDERLINE_NONE;
		}
		else {
			underline = PANGO_UNDERLINE_SINGLE;
		}

		gtk_tree_path_free (path);
	}
	else {
		underline = PANGO_UNDERLINE_NONE;
	}

	g_object_set (G_OBJECT (renderer),
		      "text", text,
		      "underline", underline,
		      NULL);
}

static _Bool
focus_in_event_callback (GtkWidget *widget, GdkEventFocus *event, void    *user_data)
{
	NautilusWindowSlot *slot;
	NautilusListView *list_view = NAUTILUS_LIST_VIEW (user_data);

	/* make the corresponding slot (and the pane that contains it) active */
	slot = nautilus_view_get_nautilus_window_slot (NAUTILUS_VIEW (list_view));
	nautilus_window_slot_make_hosting_pane_active (slot);

	return FALSE;
}


static void
create_and_setup_tree_view (NautilusListView *view)
{
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;
    GtkBindingSet *binding_set;
    AtkObject *atk_object;
    GList *nautilus_columns;
    GList *l;
    char **default_column_order, **default_visible_columns;

    TreeView = GTK_TREE_VIEW (gtk_tree_view_new ());
    view->details->columns = g_hash_table_new_full (g_str_hash,
                                                    g_str_equal,
                                                    (GDestroyNotify)g_free,
                                                    (GDestroyNotify) g_object_unref);
    gtk_tree_view_set_enable_search (TreeView, TRUE);

    /* Don't handle backspace key. It's used to open the parent folder. */
    binding_set = gtk_binding_set_by_class (GTK_WIDGET_GET_CLASS (TreeView));
    gtk_binding_entry_remove (binding_set, GDK_KEY_BackSpace, 0);

    view->details->drag_dest =
    nautilus_tree_view_drag_dest_new (TreeView);

    g_signal_connect_object (view->details->drag_dest, "get_root_uri",
                             G_CALLBACK (get_root_uri_callback), view, 0);
    g_signal_connect_object (view->details->drag_dest, "get_file_for_path",
                             G_CALLBACK (get_file_for_path_callback), view, 0);
    g_signal_connect_object (view->details->drag_dest, "move_copy_items",
                             G_CALLBACK (move_copy_items_callback), view, 0);
    g_signal_connect_object (view->details->drag_dest, "handle_netscape_url",
                             G_CALLBACK (list_view_handle_netscape_url), view, 0);
    g_signal_connect_object (view->details->drag_dest, "handle_uri_list",
                             G_CALLBACK (list_view_handle_uri_list), view, 0);
    g_signal_connect_object (view->details->drag_dest, "handle_text",
                             G_CALLBACK (list_view_handle_text), view, 0);
    g_signal_connect_object (view->details->drag_dest, "handle_raw",
                             G_CALLBACK (list_view_handle_raw), view, 0);

    g_signal_connect_object (gtk_tree_view_get_selection (TreeView), "changed",
                             G_CALLBACK (list_selection_changed_callback), view, 0);
/*
    g_signal_connect_object (GTK_WIDGET (TreeView), "query-tooltip",
                             G_CALLBACK (query_tooltip_callback), view, 0);
*/
    g_signal_connect_object (TreeView, "drag_begin",
                             G_CALLBACK (drag_begin_callback), view, 0);
    g_signal_connect_object (TreeView, "drag_data_get",
                             G_CALLBACK (drag_data_get_callback), view, 0);
    g_signal_connect_object (TreeView, "motion_notify_event",
                             G_CALLBACK (motion_notify_callback), view, 0);
    g_signal_connect_object (TreeView, "enter_notify_event",
                             G_CALLBACK (enter_notify_callback), view, 0);
    g_signal_connect_object (TreeView, "leave_notify_event",
                             G_CALLBACK (leave_notify_callback), view, 0);
    g_signal_connect_object (TreeView, "button_press_event",
                             G_CALLBACK (button_press_callback), view, 0);
    g_signal_connect_object (TreeView, "button_release_event",
                             G_CALLBACK (button_release_callback), view, 0);
    g_signal_connect_object (TreeView, "key_press_event",
                             G_CALLBACK (key_press_callback), view, 0);
    g_signal_connect_object (TreeView, "popup_menu",
                             G_CALLBACK (popup_menu_callback), view, 0);
    g_signal_connect_object (TreeView, "row_expanded",
                             G_CALLBACK (row_expanded_callback), view, 0);
    g_signal_connect_object (TreeView, "row_collapsed",
                             G_CALLBACK (row_collapsed_callback), view, 0);
    g_signal_connect_object (TreeView, "row-activated",
                             G_CALLBACK (row_activated_callback), view, 0);

    g_signal_connect_object (TreeView, "focus_in_event",
                             G_CALLBACK(focus_in_event_callback), view, 0);

    view->details->model = g_object_new (NAUTILUS_TYPE_LIST_MODEL, NULL);
    gtk_tree_view_set_model (TreeView, GTK_TREE_MODEL (view->details->model));
    /* Need the model for the dnd drop icon "accept" change */
    nautilus_list_model_set_drag_view (NAUTILUS_LIST_MODEL (view->details->model),
                                       TreeView,  0, 0);

    g_signal_connect_object (view->details->model, "sort_column_changed",
                             G_CALLBACK (sort_column_changed_callback), view, 0);

    g_signal_connect_object (view->details->model, "subdirectory_unloaded",
                             G_CALLBACK (subdirectory_unloaded_callback), view, 0);

    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (TreeView), GTK_SELECTION_MULTIPLE);
    gtk_tree_view_set_rules_hint (TreeView, TRUE);

    nautilus_columns = nautilus_get_all_columns ();

    for (l = nautilus_columns; l != NULL; l = l->next)
    {
        NautilusColumn *nautilus_column;
        int column_num;
        int font_size;
        char *name;
        char *label;
        float xalign;

        nautilus_column = NAUTILUS_COLUMN (l->data);

        g_object_get (nautilus_column,
                      "name", &name,
                      "label", &label,
                      "xalign", &xalign, NULL);

        column_num = nautilus_list_model_add_column (view->details->model, nautilus_column);

        /* Created the name column specially, because it might have the icon in it.*/
        if (!strcmp (name, "name")) {
            /* Create the file name column */
            cell = gtk_cell_renderer_pixbuf_new ();
            view->details->pixbuf_cell = (GtkCellRendererPixbuf *)cell;

            view->details->file_name_column = gtk_tree_view_column_new ();

            gtk_tree_view_column_set_expand (view->details->file_name_column, TRUE);

            font_size = PANGO_PIXELS (pango_font_description_get_size (gtk_style_context_get_font(gtk_widget_get_style_context(GTK_WIDGET(view)), GTK_STATE_FLAG_NORMAL)));
            /* WEH: hated this*/
            //gtk_tree_view_column_set_min_width (view->details->file_name_column, 20*font_size);
            gtk_tree_view_column_set_min_width (view->details->file_name_column, 25);

            g_object_ref_sink (view->details->file_name_column);
            view->details->file_name_column_num = column_num;

            g_hash_table_insert (view->details->columns,  g_strdup ("name"),
                                 view->details->file_name_column);

            gtk_tree_view_set_search_column (TreeView, column_num);

            gtk_tree_view_column_set_sort_column_id (view->details->file_name_column, column_num);
            gtk_tree_view_column_set_title (view->details->file_name_column, _("Name"));
            gtk_tree_view_column_set_resizable (view->details->file_name_column, TRUE);

            gtk_tree_view_column_pack_start (view->details->file_name_column, cell, FALSE);
            gtk_tree_view_column_set_attributes (view->details->file_name_column,
                                                 cell,
                                                 "pixbuf", NAUTILUS_LIST_MODEL_SMALLEST_ICON_COLUMN,
                                                 NULL);

            /* cell = nautilus_cell_renderer_text_ellipsized_new (); */

            cell = gtk_cell_renderer_text_new ();
            g_object_set (cell,
                          "ellipsize", PANGO_ELLIPSIZE_END,
                          "ellipsize-set", TRUE,
                          NULL);

            view->details->file_name_cell = (GtkCellRendererText *)cell;

            g_signal_connect (cell, "edited", G_CALLBACK (cell_renderer_edited), view);
            g_signal_connect (cell, "editing-canceled", G_CALLBACK (cell_renderer_editing_canceled), view);
            g_signal_connect (cell, "editing-started", G_CALLBACK (cell_renderer_editing_started_cb), view);

            gtk_tree_view_column_pack_start (view->details->file_name_column, cell, TRUE);

            gtk_tree_view_column_set_cell_data_func (view->details->file_name_column, cell,
                                                     (GtkTreeCellDataFunc) filename_cell_data_func,
                                                     view, NULL);

        }
        else { /* name column */
            cell = gtk_cell_renderer_text_new ();
            g_object_set (cell, "xalign", xalign, NULL);
            view->details->cells = g_list_append (view->details->cells, cell);
            column = gtk_tree_view_column_new_with_attributes (label,
                                                               cell,
                                                               "text", column_num,
                                                               NULL);
            g_object_ref_sink (column);
            gtk_tree_view_column_set_sort_column_id (column, column_num);
            g_hash_table_insert (view->details->columns,
                                 g_strdup (name),
                                 column);

            gtk_tree_view_column_set_resizable (column, TRUE);
            gtk_tree_view_column_set_visible (column, TRUE);
        }
        g_free (name);
        g_free (label);
    }
    nautilus_column_list_free (nautilus_columns);

    default_visible_columns = nautilus_settings_default_visible_columns();
    default_column_order = nautilus_settings_default_column_order();

    /* Apply the default column order and visible columns, to get it
     * right most of the time. The metadata will be checked when a
     * folder is loaded */
    apply_columns_settings (view, default_column_order, default_visible_columns);

    gtk_container_add (GTK_CONTAINER (view), GTK_WIDGET (TreeView));
    gtk_widget_show (GTK_WIDGET (TreeView));

    atk_object = gtk_widget_get_accessible (GTK_WIDGET (TreeView));
    atk_object_set_name (atk_object, _("List View"));

    gtk_widget_set_has_tooltip (GTK_WIDGET (TreeView), TRUE);

    g_strfreev (default_visible_columns);
    g_strfreev (default_column_order);
}


static void
list_view_add_file (NautilusView *view, NautilusFile *file, NautilusDirectory *directory)
{
	NautilusListModel *model;

	model = NAUTILUS_LIST_VIEW (view)->details->model;
	nautilus_list_model_add_file (model, file, directory);
}

static char **
get_default_visible_columns (NautilusListView *list_view)
{
    NautilusFile *file;
    NautilusDirectory *directory;

    file = nautilus_view_get_directory_as_file (NAUTILUS_VIEW (list_view));

    if (nautilus_file_is_in_trash (file)) {
        return g_strdupv ((char **) default_trash_visible_columns);
    }
/*
    if (nautilus_file_is_in_recent (file)) {
        return g_strdupv ((char **) default_recent_visible_columns);
    }
*/
    directory = nautilus_view_get_model (NAUTILUS_VIEW (list_view));
    if (NAUTILUS_IS_SEARCH_DIRECTORY (directory)) {
        return g_strdupv ((char **) default_search_visible_columns);
    }

    return eel_settings_get_strv (nautilus_list_view_preferences,
                                NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_VISIBLE_COLUMNS);
}

static char **
get_visible_columns (NautilusListView *list_view)
{
	NautilusFile *file;
	GList *visible_columns;
	char **ret_val;

	ret_val = NULL;

	file = nautilus_view_get_directory_as_file (NAUTILUS_VIEW (list_view));

    if (nautilus_settings_get_ignore_view_metadata ()) {
        visible_columns = nautilus_window_get_ignore_meta_visible_columns (nautilus_view_get_nautilus_window (NAUTILUS_VIEW (list_view)));
    } else {
        visible_columns = nautilus_file_get_metadata_list (file,
                                                   NAUTILUS_METADATA_KEY_LIST_VIEW_VISIBLE_COLUMNS);
    }

	if (visible_columns) {
		GPtrArray *res;
		GList *l;

		res = g_ptr_array_new ();
		for (l = visible_columns; l != NULL; l = l->next) {
			g_ptr_array_add (res, l->data);
		}
		g_ptr_array_add (res, NULL);

		ret_val = (char **) g_ptr_array_free (res, FALSE);
		g_list_free (visible_columns);
	}

	if (ret_val != NULL) {
		return ret_val;
	}

	return get_default_visible_columns (list_view);
}

static char**
get_default_column_order (NautilusListView *list_view)
{
    NautilusFile *file;
    NautilusDirectory *directory;

    file = nautilus_view_get_directory_as_file (NAUTILUS_VIEW (list_view));

    if (nautilus_file_is_in_trash (file)) {
        return g_strdupv ((char **) default_trash_columns_order);
    }
/*
    if (nautilus_file_is_in_recent (file)) {
        return g_strdupv ((char **) default_recent_columns_order);
    }
*/
    directory = nautilus_view_get_model (NAUTILUS_VIEW (list_view));
    if (NAUTILUS_IS_SEARCH_DIRECTORY (directory)) {
        return g_strdupv ((char **) default_search_columns_order);
    }

    return nautilus_settings_default_column_order();
}

static char **
get_column_order (NautilusListView *list_view)
{
    NautilusFile *file;
    GList *column_order;
    char **ret_val;

    ret_val = NULL;

    file = nautilus_view_get_directory_as_file (NAUTILUS_VIEW (list_view));

    if (nautilus_settings_get_ignore_view_metadata ()) {
        column_order = nautilus_window_get_ignore_meta_column_order (nautilus_view_get_nautilus_window (NAUTILUS_VIEW (list_view)));
    } else {
        column_order = nautilus_file_get_metadata_list (file,
                                                        NAUTILUS_METADATA_KEY_LIST_VIEW_COLUMN_ORDER);
    }

    if (column_order) {
        GPtrArray *res;
        GList *l;

        res = g_ptr_array_new ();
        for (l = column_order; l != NULL; l = l->next) {
            g_ptr_array_add (res, l->data);
        }
        g_ptr_array_add (res, NULL);

        ret_val = (char **) g_ptr_array_free (res, FALSE);
        g_list_free (column_order);
    }

    if (ret_val != NULL) {
        return ret_val;
    }

    return get_default_column_order (list_view);
}

static void
set_columns_settings_from_metadata_and_preferences (NautilusListView *list_view)
{
	char **column_order;
	char **visible_columns;
        /* TODO: should column widths be in the settings? */
	column_order = get_column_order (list_view);
	visible_columns = get_visible_columns (list_view);

	apply_columns_settings (list_view, column_order, visible_columns);

	g_strfreev (column_order);
	g_strfreev (visible_columns);
}

static void
set_sort_order_from_metadata_and_preferences (NautilusListView *list_view)
{
	char *sort_attribute;
	int sort_column_id;
	NautilusFile *file;
	_Bool sort_reversed, default_sort_reversed;
	const char *default_sort_order;

	file = nautilus_view_get_directory_as_file (NAUTILUS_VIEW (list_view));

    if (nautilus_settings_get_ignore_view_metadata ())
        sort_attribute = g_strdup (nautilus_window_get_ignore_meta_sort_column (nautilus_view_get_nautilus_window (NAUTILUS_VIEW (list_view))));
    else
        sort_attribute = nautilus_file_get_metadata (file,
                                                 NAUTILUS_METADATA_KEY_LIST_VIEW_SORT_COLUMN,
                                                 NULL);
	sort_column_id = nautilus_list_model_get_sort_column_id_from_attribute (ListModel,
									  g_quark_from_string (sort_attribute));
	g_free (sort_attribute);

	default_sort_order = get_default_sort_order (file, &default_sort_reversed);

	if (sort_column_id == -1) {
		sort_column_id =
			nautilus_list_model_get_sort_column_id_from_attribute (ListModel,
									 g_quark_from_string (default_sort_order));
	}

    if (nautilus_settings_get_ignore_view_metadata ()) {
        int dir = nautilus_window_get_ignore_meta_sort_direction (nautilus_view_get_nautilus_window (NAUTILUS_VIEW (list_view)));
        sort_reversed = dir > SORT_NULL ? dir == SORT_DESCENDING : default_sort_reversed;
    } else {
        sort_reversed = nautilus_file_get_boolean_metadata (file,
                                                        NAUTILUS_METADATA_KEY_LIST_VIEW_SORT_REVERSED,
                                                        default_sort_reversed);
    }
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (ListModel),
                                                             sort_column_id,
                                                             sort_reversed ? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING);
}

static _Bool
list_view_changed_foreach (GtkTreeModel *model,
              		   GtkTreePath  *path,
			   GtkTreeIter  *iter,
			   void    *     data)
{
	gtk_tree_model_row_changed (model, path, iter);
	return FALSE;
}

static NautilusZoomLevel
get_default_zoom_level (void) {
	NautilusZoomLevel default_zoom_level;

	default_zoom_level = nautilus_settings_get_list_zoom_level();

	if (default_zoom_level <  NAUTILUS_ZOOM_LEVEL_SMALLEST
	    || NAUTILUS_ZOOM_LEVEL_LARGEST < default_zoom_level) {
		default_zoom_level = NAUTILUS_ZOOM_LEVEL_SMALL;
	}

	return default_zoom_level;
}

static void
set_zoom_level_from_metadata_and_preferences (NautilusListView *list_view)
{
    NautilusFile *file;
    int level;

    if (nautilus_view_supports_zooming (NAUTILUS_VIEW (list_view))) {

        file = nautilus_view_get_directory_as_file (NAUTILUS_VIEW (list_view));
        if (nautilus_settings_get_ignore_view_metadata ()) {
            int ignore_level = nautilus_window_get_ignore_meta_zoom_level (nautilus_view_get_nautilus_window (NAUTILUS_VIEW (list_view)));
            level = ignore_level > -1 ? ignore_level : get_default_zoom_level ();
        }
        else {
            level = nautilus_file_get_integer_metadata (file,
                                                        NAUTILUS_METADATA_KEY_LIST_VIEW_ZOOM_LEVEL,
                                                        get_default_zoom_level ());
        }
        nautilus_list_view_set_zoom_level (list_view, level, TRUE);

        /* updated the rows after updating the font size */
        gtk_tree_model_foreach (GTK_TREE_MODEL (ListModel),
                                (GtkTreeModelForeachFunc)list_view_changed_foreach, NULL);
    }
}

static void
list_view_begin_loading (NautilusView *view)
{
    AtkObject        *atk_objectect;
    NautilusListView *list_view;

    list_view = NAUTILUS_LIST_VIEW (view);

    set_sort_order_from_metadata_and_preferences (list_view);
    set_zoom_level_from_metadata_and_preferences (list_view);
    set_columns_settings_from_metadata_and_preferences (list_view);

  /* Accessibility */
    atk_objectect = gtk_widget_get_accessible (GTK_WIDGET (ListTree));

    g_signal_connect_object (atk_objectect, "column-reordered",
                             G_CALLBACK (columns_reordered_callback), view, 0);
}

static void
stop_cell_editing (NautilusListView *list_view)
{
	GtkTreeViewColumn *column;

	/* Stop an ongoing rename to commit the name changes when the user
	 * changes directories without exiting cell edit mode. It also prevents
	 * the edited handler from being called on the cleared list model.
	 */
	column = list_view->details->file_name_column;
	if (column != NULL && list_view->details->editable_widget != NULL &&
	    GTK_IS_CELL_EDITABLE (list_view->details->editable_widget)) {
		gtk_cell_editable_editing_done (list_view->details->editable_widget);
	}
}

static void
nautilus_list_view_rename_callback (NautilusFile *file,
				    GFile *result_location,
				    GError *error,
				    void *callback_data)
{
	NautilusListView *view;

	view = NAUTILUS_LIST_VIEW (callback_data);

	if (view->details->renaming_file) {
		view->details->rename_done = TRUE;

		if (error != NULL) {
			/* If the rename failed (or was cancelled), kill renaming_file.
			 * We won't get a change event for the rename, so otherwise
			 * it would stay around forever.
			 */
			nautilus_file_unref (view->details->renaming_file);
			view->details->renaming_file = NULL;
		}
	}

	g_object_unref (view);
}

static void
list_view_clear (NautilusView *view)
{
  NautilusListView *list_view;

  list_view = NAUTILUS_LIST_VIEW (view);

  if (ListModel != NULL) {
    stop_cell_editing (list_view);
    nautilus_list_model_clear (ListModel);
  }
}

static void
list_view_file_changed (NautilusView      *view,
                        NautilusFile      *file,
                        NautilusDirectory *directory)
{
    NautilusListView *list_view;
    GtkTreeIter iter;
    GtkTreePath *file_path;

    list_view = NAUTILUS_LIST_VIEW (view);

    nautilus_list_model_file_changed (ListModel, file, directory);

    if (list_view->details->renaming_file != NULL &&
        file == list_view->details->renaming_file &&
        list_view->details->rename_done) {
        /* This is (probably) the result of the rename operation, and
         * the tree-view changes above could have resorted the list, so
         * scroll to the new position
         */
        if (nautilus_list_model_get_tree_iter_from_file (ListModel, file, directory, &iter)) {
            file_path = gtk_tree_model_get_path (GTK_TREE_MODEL (ListModel), &iter);
            gtk_tree_view_scroll_to_cell (ListTree,
                                          file_path, NULL,
                                          FALSE, 0.0, 0.0);
            gtk_tree_path_free (file_path);
        }

        nautilus_file_unref (list_view->details->renaming_file);
        list_view->details->renaming_file = NULL;
    }
}

typedef struct {
	GtkTreePath *path;
	_Bool is_common;
	_Bool is_root;
} HasCommonParentData;

static void
tree_selection_has_common_parent_foreach_func (GtkTreeModel *model,
                                               GtkTreePath  *path,
                                               GtkTreeIter  *iter,
                                               void         *user_data)
{
	HasCommonParentData *data;
	GtkTreePath         *parent_path;
	_Bool                has_parent;

	data = (HasCommonParentData *) user_data;

	parent_path = gtk_tree_path_copy (path);
	gtk_tree_path_up (parent_path);

	has_parent = (gtk_tree_path_get_depth (parent_path) > 0) ? TRUE : FALSE;

	if (!has_parent) {
		data->is_root = TRUE;
	}

	if (data->is_common && !data->is_root) {
		if (data->path == NULL) {
			data->path = gtk_tree_path_copy (parent_path);
		}
		else if (gtk_tree_path_compare (data->path, parent_path) != 0) {
			data->is_common = FALSE;
		}
	}

	gtk_tree_path_free (parent_path);
}

static void
tree_selection_has_common_parent (GtkTreeSelection *selection,
				  _Bool *is_common,
				  _Bool *is_root)
{
	HasCommonParentData data;

	g_assert (is_common != NULL);
	g_assert (is_root != NULL);

	data.path      = NULL;
	data.is_common = *is_common = TRUE;
	data.is_root   = *is_root = FALSE;

	gtk_tree_selection_selected_foreach (selection,
					     tree_selection_has_common_parent_foreach_func,
					     &data);

	*is_common = data.is_common;
	*is_root   = data.is_root;

	if (data.path != NULL) {
		gtk_tree_path_free (data.path);
	}
}

static char *
list_view_get_backing_uri (NautilusView *view)
{
        NautilusFile      *file;
        NautilusListView  *list_view;
        NautilusListModel *list_model;

        GtkTreeView       *tree_view;
        GtkTreeSelection  *selection;
        GtkTreePath       *path;
        GList             *paths;
        unsigned int       length;
        char              *uri;

	g_return_val_if_fail (NAUTILUS_IS_LIST_VIEW (view), NULL);

	list_view = NAUTILUS_LIST_VIEW (view);
	list_model = ListModel;
	tree_view = ListTree;

	g_assert (list_model);

	/* We currently handle three common cases here:
	 * (a) if the selection contains non-filesystem items (i.e., the
	 *     "(Empty)" label), we return the uri of the parent.
	 * (b) if the selection consists of exactly one _expanded_ directory, we
	 *     return its URI.
	 * (c) if the selection consists of either exactly one item which is not
	 *     an expanded directory) or multiple items in the same directory,
	 *     we return the URI of the common parent.
	 */

	uri = NULL;

	selection = gtk_tree_view_get_selection (tree_view);
	length = gtk_tree_selection_count_selected_rows (selection);

	if (length == 1) {

		paths = gtk_tree_selection_get_selected_rows (selection, NULL);
		path = (GtkTreePath *) paths->data;

		file = nautilus_list_model_file_for_path (list_model, path);
		if (file == NULL) {
			/* The selected item is a label, not a file */
			gtk_tree_path_up (path);
			file = nautilus_list_model_file_for_path (list_model, path);
		}

		if (file != NULL) {
			if (nautilus_file_is_directory (file) &&
			    gtk_tree_view_row_expanded (tree_view, path)) {
				uri = nautilus_file_get_uri (file);
			}
			nautilus_file_unref (file);
		}

		gtk_tree_path_free (path);
		g_list_free (paths);
	}

	if (uri == NULL && length > 0) {

		_Bool is_common, is_root;

		/* Check that all the selected items belong to the same
		 * directory and that directory is not the root directory (which
		 * is handled by NautilusView::get_backing_directory.) */

		tree_selection_has_common_parent (selection, &is_common, &is_root);

		if (is_common && !is_root) {

			paths = gtk_tree_selection_get_selected_rows (selection, NULL);
			path = (GtkTreePath *) paths->data;

			file = nautilus_list_model_file_for_path (list_model, path);
			g_assert (file != NULL);
			uri = nautilus_file_get_parent_uri (file);
			nautilus_file_unref (file);

			g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
			g_list_free (paths);
		}
	}

	if (uri != NULL) {
		return uri;
	}

	return NAUTILUS_VIEW_CLASS (nautilus_list_view_parent_class)->get_backing_uri (view);
}

static void
list_view_get_selection_foreach_func (GtkTreeModel *model,
                                      GtkTreePath  *path,
                                      GtkTreeIter  *iter,
                                      void         *data)
{
	GList **list;
	NautilusFile *file;

	list = data;

	gtk_tree_model_get (model, iter,
			    NAUTILUS_LIST_MODEL_FILE_COLUMN, &file,
			    -1);

	if (file != NULL) {
		(* list) = g_list_prepend ((* list), file);
	}
}

static GList *
list_view_get_selection (NautilusView *view)
{
	GList *list;

	list = NULL;

	gtk_tree_selection_selected_foreach (gtk_tree_view_get_selection (NAUTILUS_LIST_VIEW (view)->details->tree_view),
					     list_view_get_selection_foreach_func, &list);

	return g_list_reverse (list);
}

static void
list_view_get_selection_for_transfer_foreach_func (GtkTreeModel *model,
                                                   GtkTreePath  *path,
                                                   GtkTreeIter  *iter,
                                                   void         *data)
{
	NautilusFile *file;
	struct SelectionForeachData *selection_data;
	GtkTreeIter parent, child;

	selection_data = data;

	gtk_tree_model_get (model, iter,
			    NAUTILUS_LIST_MODEL_FILE_COLUMN, &file,
			    -1);

	if (file != NULL) {
		/* If the parent folder is also selected, don't include this file in the
		 * file operation, since that would copy it to the toplevel target instead
		 * of keeping it as a child of the copied folder
		 */
		child = *iter;
		while (gtk_tree_model_iter_parent (model, &parent, &child)) {
			if (gtk_tree_selection_iter_is_selected (selection_data->selection,
								 &parent)) {
				return;
			}
			child = parent;
		}

		nautilus_file_ref (file);
		selection_data->list = g_list_prepend (selection_data->list, file);
	}
}


static GList *
list_view_get_selection_for_transfer (NautilusView *view)
{
	struct SelectionForeachData selection_data;

	selection_data.list = NULL;
	selection_data.selection = gtk_tree_view_get_selection (NAUTILUS_LIST_VIEW (view)->details->tree_view);

	gtk_tree_selection_selected_foreach (selection_data.selection,
					     list_view_get_selection_for_transfer_foreach_func, &selection_data);

	return g_list_reverse (selection_data.list);
}




static unsigned int
list_view_get_item_count (NautilusView *view)
{
	g_return_val_if_fail (NAUTILUS_IS_LIST_VIEW (view), 0);

	return nautilus_list_model_get_length (NAUTILUS_LIST_VIEW (view)->details->model);
}

static _Bool
list_view_is_empty (NautilusView *view)
{
	return nautilus_list_model_is_empty (NAUTILUS_LIST_VIEW (view)->details->model);
}

static void
list_view_end_file_changes (NautilusView *view)
{
	NautilusListView *list_view;

	list_view = NAUTILUS_LIST_VIEW (view);

	if (list_view->details->new_selection_path) {
		gtk_tree_view_set_cursor (ListTree,
					  list_view->details->new_selection_path,
					  NULL, FALSE);
		gtk_tree_path_free (list_view->details->new_selection_path);
		list_view->details->new_selection_path = NULL;
	}
}

static void
list_view_remove_file (NautilusView *view, NautilusFile *file, NautilusDirectory *directory)
{
	GtkTreePath *path;
	GtkTreePath *file_path;
	GtkTreeIter iter;
	GtkTreeIter temp_iter;
	GtkTreeRowReference* row_reference;
	NautilusListView *list_view;
	GtkTreeModel* tree_model;
	GtkTreeSelection *selection;

	path = NULL;
	row_reference = NULL;
	list_view = NAUTILUS_LIST_VIEW (view);
	tree_model = GTK_TREE_MODEL(ListModel);

	if (nautilus_list_model_get_tree_iter_from_file (ListModel, file, directory, &iter)) {
		selection = gtk_tree_view_get_selection (ListTree);
		file_path = gtk_tree_model_get_path (tree_model, &iter);

		if (gtk_tree_selection_path_is_selected (selection, file_path)) {
			/* get reference for next element in the list view. If the element to be deleted is the
			 * last one, get reference to previous element. If there is only one element in view
			 * no need to select anything.
			 */
			temp_iter = iter;

			if (gtk_tree_model_iter_next (tree_model, &iter)) {
				path = gtk_tree_model_get_path (tree_model, &iter);
				row_reference = gtk_tree_row_reference_new (tree_model, path);
			} else {
				path = gtk_tree_model_get_path (tree_model, &temp_iter);
				if (gtk_tree_path_prev (path)) {
					row_reference = gtk_tree_row_reference_new (tree_model, path);
				}
			}
			gtk_tree_path_free (path);
		}

		gtk_tree_path_free (file_path);

		nautilus_list_model_remove_file (ListModel, file, directory);

		if (gtk_tree_row_reference_valid (row_reference)) {
			if (list_view->details->new_selection_path) {
				gtk_tree_path_free (list_view->details->new_selection_path);
			}
			list_view->details->new_selection_path = gtk_tree_row_reference_get_path (row_reference);
		}

		if (row_reference) {
			gtk_tree_row_reference_free (row_reference);
		}
	}


}

static void
list_view_set_selection (NautilusView *view, GList *selection)
{
	NautilusListView *list_view;
	GtkTreeSelection *tree_selection;
	GList *node;
	GList *iters, *l;
	NautilusFile *file;

	list_view = NAUTILUS_LIST_VIEW (view);
	tree_selection = gtk_tree_view_get_selection (ListTree);

	g_signal_handlers_block_by_func (tree_selection, list_selection_changed_callback, view);

	gtk_tree_selection_unselect_all (tree_selection);
	for (node = selection; node != NULL; node = node->next) {
		file = node->data;
		iters = nautilus_list_model_get_all_iters_for_file (ListModel, file);

		for (l = iters; l != NULL; l = l->next) {
			gtk_tree_selection_select_iter (tree_selection,
							(GtkTreeIter *)l->data);
		}
		g_list_free_full (iters, g_free);
	}

	g_signal_handlers_unblock_by_func (tree_selection, list_selection_changed_callback, view);
	nautilus_view_notify_selection_changed (view);
}

static void
list_view_invert_selection (NautilusView *view)
{
	NautilusListView *list_view;
	GtkTreeSelection *tree_selection;
	GList *node;
	GList *iters, *l;
	NautilusFile *file;
	GList *selection = NULL;

	list_view = NAUTILUS_LIST_VIEW (view);
	tree_selection = gtk_tree_view_get_selection (ListTree);

	g_signal_handlers_block_by_func (tree_selection, list_selection_changed_callback, view);

	gtk_tree_selection_selected_foreach (tree_selection,
					     list_view_get_selection_foreach_func, &selection);

	gtk_tree_selection_select_all (tree_selection);

	for (node = selection; node != NULL; node = node->next) {
		file = node->data;
		iters = nautilus_list_model_get_all_iters_for_file (ListModel, file);

		for (l = iters; l != NULL; l = l->next) {
			gtk_tree_selection_unselect_iter (tree_selection,
							  (GtkTreeIter *)l->data);
		}
		g_list_free_full (iters, g_free);
	}

	g_list_free (selection);

	g_signal_handlers_unblock_by_func (tree_selection, list_selection_changed_callback, view);
	nautilus_view_notify_selection_changed (view);
}

static void
list_view_select_all (NautilusView *view)
{
	gtk_tree_selection_select_all (gtk_tree_view_get_selection (NAUTILUS_LIST_VIEW (view)->details->tree_view));
}

static void
action_sort_radio_callback (GtkAction        *action,
                            GtkRadioAction   *current,
                            NautilusListView *view)
{
        NautilusFileSortType sort_type;

        sort_type = gtk_radio_action_get_current_value (current);

        /* set_sort_criterion_by_sort_type (view, sort_type); **/
        /*sort_column_changed_callback*/

        fprintf(stderr, "%s sort to column=%d\n", __func__, sort_type);
}

static void
list_view_merge_menus (NautilusView *view)
{
  NautilusListView *list_view;
  GtkUIManager *ui_manager;
  GtkActionGroup *action_group;

  list_view = NAUTILUS_LIST_VIEW (view);

  NAUTILUS_VIEW_CLASS (nautilus_list_view_parent_class)->merge_menus (view);

  ui_manager = nautilus_view_get_ui_manager (view);

  action_group = gtk_action_group_new ("ListViewActions");
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  list_view->details->list_action_group = action_group;

  gtk_action_group_add_actions (action_group,
                                list_view_entries, G_N_ELEMENTS (list_view_entries),
                                list_view);

  gtk_action_group_add_radio_actions (action_group,
                                      sort_order_radio_entries,
                                      G_N_ELEMENTS (sort_order_radio_entries),
                                      -1,
                                      G_CALLBACK (action_sort_radio_callback),
                                      list_view);

  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  g_object_unref (action_group); /* owned by ui manager */

  list_view->details->list_merge_id =
  gtk_ui_manager_add_ui_from_resource (ui_manager, "/apps/nautilus/nautilus-list-view-ui.xml", NULL);

  list_view->details->menus_ready = TRUE;
}

static void
list_view_unmerge_menus (NautilusView *view)
{
  NautilusListView *list_view;
  GtkUIManager *ui_manager;

  list_view = NAUTILUS_LIST_VIEW (view);

  NAUTILUS_VIEW_CLASS (nautilus_list_view_parent_class)->unmerge_menus (view);

  ui_manager = nautilus_view_get_ui_manager (view);
  if (ui_manager != NULL) {
    nautilus_ui_unmerge_ui (ui_manager,
          &list_view->details->list_merge_id,
          &list_view->details->list_action_group);
  }
}

static void
list_view_update_menus (NautilusView *view)
{
	NautilusListView *list_view;

        list_view = NAUTILUS_LIST_VIEW (view);

	/* don't update if the menus aren't ready */
	if (!list_view->details->menus_ready) {
		return;
	}

	NAUTILUS_VIEW_CLASS (nautilus_list_view_parent_class)->update_menus (view);
}

/* Reset sort criteria and zoom level to match defaults */
static void
list_view_reset_to_defaults (NautilusView *view)
{
  NautilusFile *file;

  file = nautilus_view_get_directory_as_file (view);

  g_signal_handlers_block_by_func (NAUTILUS_LIST_VIEW (view)->details->tree_view,
                                   columns_reordered_callback,
                                   NAUTILUS_LIST_VIEW (view));

  if (nautilus_settings_get_ignore_view_metadata ()) {
    NautilusWindow *window = nautilus_view_get_nautilus_window (NAUTILUS_VIEW (view));
    nautilus_window_set_ignore_meta_sort_column (window, NULL);
    nautilus_window_set_ignore_meta_sort_direction (window, SORT_NULL);
    nautilus_window_set_ignore_meta_zoom_level (window, NAUTILUS_ZOOM_LEVEL_NULL);
    nautilus_window_set_ignore_meta_column_order (window, NULL);
    nautilus_window_set_ignore_meta_visible_columns (window, NULL);
  } else {
    nautilus_file_set_metadata (file, NAUTILUS_METADATA_KEY_LIST_VIEW_SORT_COLUMN, NULL, NULL);
    nautilus_file_set_metadata (file, NAUTILUS_METADATA_KEY_LIST_VIEW_SORT_REVERSED, NULL, NULL);
    nautilus_file_set_metadata (file, NAUTILUS_METADATA_KEY_LIST_VIEW_ZOOM_LEVEL, NULL, NULL);
    nautilus_file_set_metadata_list (file, NAUTILUS_METADATA_KEY_LIST_VIEW_COLUMN_ORDER, NULL);
    nautilus_file_set_metadata_list (file, NAUTILUS_METADATA_KEY_LIST_VIEW_VISIBLE_COLUMNS, NULL);
  }

  char **default_columns, **default_order;

  default_columns = get_default_visible_columns (NAUTILUS_LIST_VIEW (view));
  default_order = get_default_column_order (NAUTILUS_LIST_VIEW (view));
  apply_columns_settings (NAUTILUS_LIST_VIEW (view), default_order, default_columns);

  g_signal_handlers_unblock_by_func (NAUTILUS_LIST_VIEW (view)->details->tree_view,
                                     columns_reordered_callback,
                                     NAUTILUS_LIST_VIEW (view));
}

static void
nautilus_list_view_scale_font_size (NautilusListView *view,
				    NautilusZoomLevel new_level)
{
	GList *l;
	static _Bool first_time = TRUE;
	static double pango_scale[7];
	int medium;
	int i;

	g_return_if_fail (new_level >= NAUTILUS_ZOOM_LEVEL_SMALLEST &&
			  new_level <= NAUTILUS_ZOOM_LEVEL_LARGEST);

	if (first_time) {
		first_time = FALSE;
		medium = NAUTILUS_ZOOM_LEVEL_SMALLER;
		pango_scale[medium] = PANGO_SCALE_MEDIUM;
		for (i = medium; i > NAUTILUS_ZOOM_LEVEL_SMALLEST; i--) {
			pango_scale[i - 1] = (1 / 1.2) * pango_scale[i];
		}
		for (i = medium; i < NAUTILUS_ZOOM_LEVEL_LARGEST; i++) {
			pango_scale[i + 1] = 1.2 * pango_scale[i];
		}
	}

	g_object_set (G_OBJECT (view->details->file_name_cell),
		      "scale", pango_scale[new_level],
		      NULL);
	for (l = view->details->cells; l != NULL; l = l->next) {
		g_object_set (G_OBJECT (l->data),
			      "scale", pango_scale[new_level],
			      NULL);
	}
}

static void
autosize_columns_preference_changed_callback (void *data)
{
    NautilusListView *view = NAUTILUS_LIST_VIEW (data);
}

static void
nautilus_list_view_set_zoom_level (NautilusListView *view,
                                   NautilusZoomLevel new_level,
                                   _Bool always_emit)
{
    int icon_size;
    int column;

    g_return_if_fail (NAUTILUS_IS_LIST_VIEW (view));

    //g_return_if_fail (new_level >= NAUTILUS_ZOOM_LEVEL_SMALLEST &&
    //new_level <= NAUTILUS_ZOOM_LEVEL_LARGEST);

    if (view->details->zoom_level == new_level) {
        if (always_emit) {
            g_signal_emit_by_name (NAUTILUS_VIEW(view), "zoom-level-changed");
        }
        return;
    }

    view->details->zoom_level = new_level;
    g_signal_emit_by_name (NAUTILUS_VIEW(view), "zoom-level-changed");

    if (nautilus_settings_get_ignore_view_metadata ())
        nautilus_window_set_ignore_meta_zoom_level (nautilus_view_get_nautilus_window (NAUTILUS_VIEW (view)), new_level);
    else
        nautilus_file_set_integer_metadata
        (nautilus_view_get_directory_as_file (NAUTILUS_VIEW (view)),
         NAUTILUS_METADATA_KEY_LIST_VIEW_ZOOM_LEVEL, get_default_zoom_level (), new_level);

        /* Select correctly scaled icons. */
        column = nautilus_list_model_get_column_id_from_zoom_level (new_level);
    /*
     * gtk_tree_view_column_set_attributes (view->details->file_name_column,
     *				     GTK_CELL_RENDERER (view->details->pixbuf_cell),
     *				     "surface", column,
     *				     NULL);
     */
    /* Scale text. */
    nautilus_list_view_scale_font_size (view, new_level);

    /* Make all rows the same size. */
    icon_size = nautilus_get_list_icon_size_for_zoom_level (new_level);
    gtk_cell_renderer_set_fixed_size (GTK_CELL_RENDERER (view->details->pixbuf_cell),
                                      -1, icon_size);

    nautilus_view_update_menus (NAUTILUS_VIEW (view));

    if (eel_settings_get_boolean (nautilus_list_view_preferences,
        NAUTILUS_PREFERENCES_LIST_VIEW_AUTO_RESIZE_COLUMNS))
    {
      //fprintf(stderr,"%s auto resizing columns\n", __func__);
      // TODO Does not WORK
        gtk_tree_view_columns_autosize (TreeView);
    }
}

static void
list_view_bump_zoom_level (NautilusView *view, int zoom_increment)
{
	NautilusListView *list_view;
	int new_level;

	//g_return_if_fail (NAUTILUS_IS_LIST_VIEW (view));

	list_view = NAUTILUS_LIST_VIEW (view);
	new_level = ZoomLevel + zoom_increment;

	if (new_level >= NAUTILUS_ZOOM_LEVEL_SMALLEST &&
	    new_level <= NAUTILUS_ZOOM_LEVEL_LARGEST) {
		nautilus_list_view_set_zoom_level (list_view, new_level, FALSE);
	}
}

static NautilusZoomLevel
list_view_get_zoom_level (NautilusView *view)
{
	NautilusListView *list_view;

	g_return_val_if_fail (NAUTILUS_IS_LIST_VIEW (view), NAUTILUS_ZOOM_LEVEL_STANDARD);

	list_view = NAUTILUS_LIST_VIEW (view);

	return ZoomLevel;
}

static void
list_view_zoom_to_level (NautilusView *view,
				  NautilusZoomLevel zoom_level)
{
	NautilusListView *list_view;

	g_return_if_fail (NAUTILUS_IS_LIST_VIEW (view));

	list_view = NAUTILUS_LIST_VIEW (view);

	nautilus_list_view_set_zoom_level (list_view, zoom_level, FALSE);
}

static void
list_view_restore_default_zoom_level (NautilusView *view)
{
	NautilusListView *list_view;

	g_return_if_fail (NAUTILUS_IS_LIST_VIEW (view));

	list_view = NAUTILUS_LIST_VIEW (view);

	nautilus_list_view_set_zoom_level (list_view, get_default_zoom_level (), FALSE);
}

static NautilusZoomLevel
list_view_get_default_zoom_level (NautilusView *view)
{
    g_return_if_fail (NAUTILUS_IS_LIST_VIEW (view));

    return get_default_zoom_level();
}

static _Bool
list_view_can_zoom_in (NautilusView *view)
{
	g_return_val_if_fail (NAUTILUS_IS_LIST_VIEW (view), FALSE);

	return NAUTILUS_LIST_VIEW (view)->details->zoom_level	< NAUTILUS_ZOOM_LEVEL_LARGEST;
}

static _Bool
list_view_can_zoom_out (NautilusView *view)
{
	g_return_val_if_fail (NAUTILUS_IS_LIST_VIEW (view), FALSE);

	return NAUTILUS_LIST_VIEW (view)->details->zoom_level > NAUTILUS_ZOOM_LEVEL_SMALLEST;
}

static void
list_view_start_renaming_file (NautilusView *view,
                               NautilusFile *file,
                               _Bool select_all)
{
	NautilusListView *list_view;
	GtkTreeIter iter;
	GtkTreePath *path;

	list_view = NAUTILUS_LIST_VIEW (view);

	/* Select all if we are in renaming mode already */
	if (list_view->details->file_name_column && list_view->details->editable_widget) {
		gtk_editable_select_region (GTK_EDITABLE (list_view->details->editable_widget),
					    0,
					    -1);
		return;
	}

	if (!nautilus_list_model_get_first_iter_for_file (ListModel, file, &iter)) {
		return;
	}

	/* call parent class to make sure the right icon is selected */
	NAUTILUS_VIEW_CLASS (nautilus_list_view_parent_class)->start_renaming_file (view, file, select_all);

	/* Freeze updates to the view to prevent losing rename focus when the tree view updates */
	nautilus_view_freeze_updates (NAUTILUS_VIEW (view));

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (ListModel), &iter);

	/* Make filename-cells editable. */
	g_object_set (G_OBJECT (list_view->details->file_name_cell),
		      "editable", TRUE,
		      NULL);

	gtk_tree_view_scroll_to_cell (ListTree,
				      NULL,
				      list_view->details->file_name_column,
				      TRUE, 0.0, 0.0);
	gtk_tree_view_set_cursor_on_cell (ListTree,
					  path,
					  list_view->details->file_name_column,
					  GTK_CELL_RENDERER (list_view->details->file_name_cell),
					  TRUE);

	/* set cursor also triggers editing-started, where we save the editable widget */
	if (list_view->details->editable_widget != NULL) {
        int start_offset = 0;
        int end_offset = -1;

        if (!select_all) {
            eel_filename_get_rename_region (list_view->details->original_name,
                           &start_offset, &end_offset);
        }

		gtk_editable_select_region (GTK_EDITABLE (list_view->details->editable_widget),
					    start_offset, end_offset);
	}

	gtk_tree_path_free (path);
}

static void
list_view_click_policy_changed (NautilusView *directory_view)
{
    GdkWindow *win;
    GdkDisplay *display;
    NautilusListView *view;
    GtkTreeIter iter;
    GtkTreeView *tree;

    view = NAUTILUS_LIST_VIEW (directory_view);

    ClickPolicy  = nautilus_settings_get_click_policy();

    /* ensure that we unset the hand cursor and refresh underlined rows */
    if (ClickPolicy == NAUTILUS_CLICK_POLICY_DOUBLE) {
        if (view->details->hover_path != NULL) {
            if (gtk_tree_model_get_iter (GTK_TREE_MODEL (view->details->model),
                &iter, view->details->hover_path)) {
                gtk_tree_model_row_changed (GTK_TREE_MODEL (view->details->model),
                                            view->details->hover_path, &iter);
                }

                gtk_tree_path_free (view->details->hover_path);
            view->details->hover_path = NULL;
        }

        tree = TreeView;
        if (gtk_widget_get_realized (GTK_WIDGET (tree))) {
            win = gtk_widget_get_window (GTK_WIDGET (tree));
            gdk_window_set_cursor (win, NULL);

            display = gtk_widget_get_display (GTK_WIDGET (view));
            if (display != NULL) {
                gdk_display_flush (display);
            }
        }

        g_clear_object (&hand_cursor);
    }
    else if (ClickPolicy == NAUTILUS_CLICK_POLICY_SINGLE) {
        if (hand_cursor == NULL) {
            hand_cursor = gdk_cursor_new(GDK_HAND2);
        }
    }
}

/* ---------- Callback handler for Preference changes ---------- */

static void
default_sort_order_changed_callback (void *callback_data)
{
	NautilusListView *list_view;

	list_view = NAUTILUS_LIST_VIEW (callback_data);

	set_sort_order_from_metadata_and_preferences (list_view);
}

static void
default_zoom_level_changed_callback (void *callback_data)
{
	NautilusListView *list_view;

	list_view = NAUTILUS_LIST_VIEW (callback_data);

	set_zoom_level_from_metadata_and_preferences (list_view);
}

static void
default_visible_columns_changed_callback (void *callback_data)
{
	NautilusListView *list_view;

	list_view = NAUTILUS_LIST_VIEW (callback_data);

	set_columns_settings_from_metadata_and_preferences (list_view);
}

static void
default_column_order_changed_callback (void *callback_data)
{
	NautilusListView *list_view;

	list_view = NAUTILUS_LIST_VIEW (callback_data);

	set_columns_settings_from_metadata_and_preferences (list_view);
}

static void
tooltip_prefs_time_changed_callback (NautilusListView *view)
{
   int timeout;

   timeout = eel_settings_get_int (nautilus_tooltips_preferences,
                                   NAUTILUS_TOOLTIP_PREFERENCES_LIST_TIME_OUT);

   eel_tooltip_set_timeout (view->details->tooltip, timeout);
}

static void
tooltip_prefs_changed_callback (NautilusListView *view)
{
    int show_tooltips;

    show_tooltips = eel_settings_get_boolean (nautilus_tooltips_preferences,
                                              NAUTILUS_TOOLTIP_PREFERENCES_LIST_VIEW);
    if (show_tooltips) {

       view->details->tooltip_flags = nautilus_global_preferences_get_tooltip_flags ();

       view->details->show_tooltips = FALSE;

       view->details->tooltip_handler_id =
           g_signal_connect (TreeView, "query-tooltip",
                             G_CALLBACK (query_tooltip_callback), view);

       g_object_set (TreeView, "has-tooltip", TRUE, NULL);
    }
    else {
       g_object_set (TreeView, "has-tooltip", FALSE, NULL);
       if (view->details->tooltip_handler_id > 0) {
         eel_source_remove (view->details->tooltip_handler_id);
         view->details->tooltip_handler_id = 0;
       }
    }

    view->details->show_tooltips = show_tooltips;
}

/* End Preference changed callbacks */

static void
list_view_sort_directories_first_changed (NautilusView *view)
{
	NautilusListView *list_view;

	list_view = NAUTILUS_LIST_VIEW (view);

	nautilus_list_model_set_should_sort_directories_first (ListModel,
							 nautilus_view_should_sort_directories_first (view));
}

static int
list_view_compare_files (NautilusView *view, NautilusFile *file1, NautilusFile *file2)
{
	NautilusListView *list_view;

	list_view = NAUTILUS_LIST_VIEW (view);
	return nautilus_list_model_compare_func (ListModel, file1, file2);
}

static _Bool
list_view_using_manual_layout (NautilusView *view)
{
	g_return_val_if_fail (NAUTILUS_IS_LIST_VIEW (view), FALSE);

	return FALSE;
}

static void
list_view_dispose (GObject *object)
{
  NautilusListView *list_view;

  list_view = NAUTILUS_LIST_VIEW (object);

  if (ListModel) {
    stop_cell_editing (list_view);
    g_object_unref (ListModel);
    ListModel = NULL;
  }

  if (list_view->details->drag_dest) {
    g_object_unref (list_view->details->drag_dest);
    list_view->details->drag_dest = NULL;
  }

  EEL_SOURCE_REMOVE_IF_THEN_ZERO (list_view->details->renaming_file_activate_timeout);

  EEL_DISCONNECT_HANDLER_IF_THEN_ZERO (nautilus_clipboard_monitor_get (), list_view->details->clipboard_handler_id)

  G_OBJECT_CLASS (nautilus_list_view_parent_class)->dispose (object);
}

static void
list_view_finalize (GObject *object)
{
	NautilusListView *list_view;

	list_view = NAUTILUS_LIST_VIEW (object);

	g_free (list_view->details->original_name);
	list_view->details->original_name = NULL;

	if (list_view->details->double_click_path[0]) {
		gtk_tree_path_free (list_view->details->double_click_path[0]);
	}
	if (list_view->details->double_click_path[1]) {
		gtk_tree_path_free (list_view->details->double_click_path[1]);
	}
	if (list_view->details->new_selection_path) {
		gtk_tree_path_free (list_view->details->new_selection_path);
	}

	g_list_free (list_view->details->cells);
	g_hash_table_destroy (list_view->details->columns);

	if (list_view->details->hover_path != NULL) {
		gtk_tree_path_free (list_view->details->hover_path);
	}

	if (list_view->details->column_editor != NULL) {
		gtk_widget_destroy (list_view->details->column_editor);
	}

	g_free (list_view->details);

	g_signal_handlers_disconnect_by_func (nautilus_preferences,
					      default_sort_order_changed_callback,
					      list_view);
	g_signal_handlers_disconnect_by_func (nautilus_list_view_preferences,
					      default_zoom_level_changed_callback,
					      list_view);
	g_signal_handlers_disconnect_by_func (nautilus_list_view_preferences,
					      default_visible_columns_changed_callback,
					      list_view);
	g_signal_handlers_disconnect_by_func (nautilus_list_view_preferences,
					      default_column_order_changed_callback,
					      list_view);
/*
        g_signal_handlers_disconnect_by_func (nautilus_preferences,
                                              click_policy_prefs_changed_callback,
                                              list_view);
*/
        g_signal_handlers_disconnect_by_func (nautilus_preferences,
                                              tooltip_prefs_changed_callback,
                                              list_view);

	G_OBJECT_CLASS (nautilus_list_view_parent_class)->finalize (object);
}

static char *
list_view_get_first_visible_file (NautilusView *view)
{
	NautilusFile *file;
	GtkTreePath *path;
	GtkTreeIter iter;
	NautilusListView *list_view;

	list_view = NAUTILUS_LIST_VIEW (view);

	if (gtk_tree_view_get_path_at_pos (ListTree,
					   0, 0,
					   &path, NULL, NULL, NULL)) {
		gtk_tree_model_get_iter (GTK_TREE_MODEL (ListModel),
					 &iter, path);

		gtk_tree_path_free (path);

		gtk_tree_model_get (GTK_TREE_MODEL (ListModel),
				    &iter,
				    NAUTILUS_LIST_MODEL_FILE_COLUMN, &file,
				    -1);
		if (file) {
			char *uri;

			uri = nautilus_file_get_uri (file);

			nautilus_file_unref (file);

			return uri;
		}
	}

	return NULL;
}

static void
nautilus_list_view_scroll_to_file (NautilusListView *view,
				   NautilusFile *file)
{
	GtkTreePath *path;
	GtkTreeIter iter;

	if (!nautilus_list_model_get_first_iter_for_file (view->details->model, file, &iter)) {
		return;
	}

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (view->details->model), &iter);

	gtk_tree_view_scroll_to_cell (TreeView,
				      path, NULL,
				      TRUE, 0.0, 0.0);

	gtk_tree_path_free (path);
}

static void
list_view_scroll_to_file (NautilusView *view,
			  const char *uri)
{
	NautilusFile *file;

	if (uri != NULL) {
		/* Only if existing, since we don't want to add the file to
		   the directory if it has been removed since then */
		file = nautilus_file_get_existing_by_uri (uri);
		if (file != NULL) {
			nautilus_list_view_scroll_to_file (NAUTILUS_LIST_VIEW (view), file);
			nautilus_file_unref (file);
		}
	}
}

static void
list_view_notify_clipboard_info (NautilusClipboardMonitor *monitor,
                                 NautilusClipboardInfo *info,
                                 NautilusListView *view)
{
	/* this could be called as a result of _end_loading() being
	 * called after _dispose(), where the model is cleared.
	 */
	if (view->details->model == NULL) {
		return;
	}

	if (info != NULL && info->cut) {
		nautilus_list_model_set_highlight_for_files (view->details->model, info->files);
	} else {
		nautilus_list_model_set_highlight_for_files (view->details->model, NULL);
	}
}

static void
list_view_end_loading (NautilusView *view, _Bool all_files_seen)
{
	NautilusClipboardMonitor *monitor;
	NautilusClipboardInfo *info;

	monitor = nautilus_clipboard_monitor_get ();
	info = nautilus_clipboard_monitor_get_clipboard_info (monitor);

	list_view_notify_clipboard_info (monitor, info, NAUTILUS_LIST_VIEW (view));
}

static const char *
list_view_get_id (NautilusView *view)
{
	return NAUTILUS_LIST_VIEW_ID;
}

static void
nautilus_list_view_class_init (NautilusListViewClass *class)
{
    NautilusViewClass *view_class;  /* Parent Class is NautilusViewClass */

    view_class = NAUTILUS_VIEW_CLASS (class);

    G_OBJECT_CLASS (class)->dispose              = list_view_dispose;
    G_OBJECT_CLASS (class)->finalize             = list_view_finalize;

    view_class->add_file                         = list_view_add_file;
    view_class->begin_loading                    = list_view_begin_loading;
    view_class->end_loading                      = list_view_end_loading;
    view_class->bump_zoom_level                  = list_view_bump_zoom_level;
    view_class->can_zoom_in                      = list_view_can_zoom_in;
    view_class->can_zoom_out                     = list_view_can_zoom_out;
    view_class->click_policy_changed             = list_view_click_policy_changed;
    view_class->clear                            = list_view_clear;
    view_class->file_changed                     = list_view_file_changed;

    view_class->get_backing_uri                  = list_view_get_backing_uri;
    view_class->get_selection                    = list_view_get_selection;
    view_class->get_selection_for_transfer       = list_view_get_selection_for_transfer;
    view_class->get_item_count                   = list_view_get_item_count;

    view_class->is_empty                         = list_view_is_empty;
    view_class->remove_file                      = list_view_remove_file;
    view_class->reset_to_defaults                = list_view_reset_to_defaults;
    view_class->restore_default_zoom_level       = list_view_restore_default_zoom_level;

    view_class->merge_menus                      = list_view_merge_menus;
    view_class->unmerge_menus                    = list_view_unmerge_menus;
    view_class->update_menus                     = list_view_update_menus;


    view_class->get_default_zoom_level           = list_view_get_default_zoom_level;
    view_class->get_zoom_level                   = list_view_get_zoom_level;
    view_class->zoom_to_level                    = list_view_zoom_to_level;

    view_class->reveal_selection                 = list_view_reveal_selection;
    view_class->select_all                       = list_view_select_all;
    view_class->set_selection                    = list_view_set_selection;
    view_class->invert_selection                 = list_view_invert_selection;
    view_class->compare_files                    = list_view_compare_files;
    view_class->sort_directories_first_changed   = list_view_sort_directories_first_changed;
    view_class->start_renaming_file              = list_view_start_renaming_file;

    view_class->end_file_changes                 = list_view_end_file_changes;
    view_class->using_manual_layout              = list_view_using_manual_layout;
    view_class->get_view_id                      = list_view_get_id;
    view_class->get_first_visible_file           = list_view_get_first_visible_file;
    view_class->scroll_to_file                   = list_view_scroll_to_file;
}

static void
nautilus_list_view_init (NautilusListView *list_view)
{
    list_view->details = g_new0 (NautilusListViewDetails, 1);

    create_and_setup_tree_view (list_view);

    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_DEFAULT_SORT_ORDER,
                              G_CALLBACK (default_sort_order_changed_callback),
                              list_view);
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_DEFAULT_SORT_IN_REVERSE_ORDER,
                              G_CALLBACK (default_sort_order_changed_callback),
                              list_view);
    g_signal_connect_swapped (nautilus_list_view_preferences,
                              "changed::" NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_ZOOM_LEVEL,
                              G_CALLBACK (default_zoom_level_changed_callback),
                              list_view);
    g_signal_connect_swapped (nautilus_list_view_preferences,
                              "changed::" NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_VISIBLE_COLUMNS,
                              G_CALLBACK (default_visible_columns_changed_callback),
                              list_view);
    g_signal_connect_swapped (nautilus_list_view_preferences,
                              "changed::" NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_COLUMN_ORDER,
                              G_CALLBACK (default_column_order_changed_callback),
                              list_view);

    g_signal_connect_swapped (nautilus_tooltips_preferences,
                              "changed::" NAUTILUS_TOOLTIP_PREFERENCES_LIST_VIEW,
                              G_CALLBACK (tooltip_prefs_changed_callback),
                              list_view);

    g_signal_connect_swapped (nautilus_tooltips_preferences,
                              "changed::" NAUTILUS_TOOLTIP_PREFERENCES_LIST_TIME_OUT,
                              G_CALLBACK (tooltip_prefs_time_changed_callback),
                              list_view);

    g_signal_connect_swapped (nautilus_tooltips_preferences,
                              "changed::" NAUTILUS_TOOLTIP_PREFERENCES_FILE_TYPE,
                              G_CALLBACK (tooltip_prefs_changed_callback),
                              list_view);

    g_signal_connect_swapped (nautilus_tooltips_preferences,
                              "changed::" NAUTILUS_TOOLTIP_PREFERENCES_MOD_DATE,
                              G_CALLBACK (tooltip_prefs_changed_callback),
                              list_view);

    g_signal_connect_swapped (nautilus_tooltips_preferences,
                              "changed::" NAUTILUS_TOOLTIP_PREFERENCES_ACCESS_DATE,
                              G_CALLBACK (tooltip_prefs_changed_callback),
                              list_view);

    g_signal_connect_swapped (nautilus_tooltips_preferences,
                              "changed::" NAUTILUS_TOOLTIP_PREFERENCES_FULL_PATH,
                              G_CALLBACK (tooltip_prefs_changed_callback),
                              list_view);

    g_signal_connect_swapped (nautilus_tooltips_preferences,
                              "changed::" NAUTILUS_TOOLTIP_PREFERENCES_PERMISSIONS,
                              G_CALLBACK (tooltip_prefs_changed_callback),
                              list_view);

    g_signal_connect_swapped (nautilus_tooltips_preferences,
                              "changed::" NAUTILUS_TOOLTIP_PREFERENCES_OCTAL,
                              G_CALLBACK (tooltip_prefs_changed_callback),
                              list_view);

    list_view->details->tooltip = eel_tooltip_new ();

    tooltip_prefs_time_changed_callback (list_view);

    list_view->details->tooltip_handler_id    = 0;

    gtk_widget_set_tooltip_window (GTK_WIDGET(ListTree),
                                   GTK_WINDOW(list_view->details->tooltip->window));

    tooltip_prefs_changed_callback (list_view);

    list_view_click_policy_changed (NAUTILUS_VIEW (list_view));

    list_view_sort_directories_first_changed (NAUTILUS_VIEW (list_view));

    /* ensure that the zoom level is always set in begin_loading */
    ZoomLevel = NAUTILUS_ZOOM_LEVEL_SMALLEST - 1;

    list_view->details->hover_path = NULL;
    list_view->details->clipboard_handler_id =
    g_signal_connect (nautilus_clipboard_monitor_get (),
                      "clipboard_info",
                      G_CALLBACK (list_view_notify_clipboard_info), list_view);
}

static NautilusView *
nautilus_list_view_create (NautilusWindowSlot *slot)
{
    NautilusListView *view;

    view = g_object_new (NAUTILUS_TYPE_LIST_VIEW,
                         "window-slot", slot,
                         NULL);
    return NAUTILUS_VIEW (view);
}

static _Bool
nautilus_list_view_supports_uri (const char *uri,
				 GFileType file_type,
				 const char *mime_type)
{
	if (file_type == G_FILE_TYPE_DIRECTORY) {
		return TRUE;
	}
	if (strcmp (mime_type, NAUTILUS_SAVED_SEARCH_MIMETYPE) == 0){
		return TRUE;
	}
	if (g_str_has_prefix (uri, "trash:")) {
		return TRUE;
	}
    if (g_str_has_prefix (uri, "recent:")) {
        return TRUE;
    }
	if (g_str_has_prefix (uri, EEL_SEARCH_URI)) {
		return TRUE;
	}

	return FALSE;
}

static NautilusViewInfo nautilus_list_view = {
	NAUTILUS_LIST_VIEW_ID,
	/* translators: this is used in the view selection dropdown
	 * of navigation windows and in the preferences dialog */
	N_("List View"),
	/* translators: this is used in the view menu */
	N_("_List"),
	N_("The list view encountered an error."),
	N_("The list view encountered an error while starting up."),
	N_("Display this location with the list view."),
	nautilus_list_view_create,
	nautilus_list_view_supports_uri
};

void
nautilus_list_view_register (void)
{
    nautilus_list_view.view_combo_label              = _(nautilus_list_view.view_combo_label);
    nautilus_list_view.view_menu_label_with_mnemonic = _(nautilus_list_view.view_menu_label_with_mnemonic);
    nautilus_list_view.error_label            = _(nautilus_list_view.error_label);
    nautilus_list_view.startup_error_label    = _(nautilus_list_view.startup_error_label);
    nautilus_list_view.display_location_label = _(nautilus_list_view.display_location_label);

    nautilus_view_factory_register (&nautilus_list_view);
}

GtkTreeView*
nautilus_list_view_get_tree_view (NautilusListView *list_view)
{
	return ListTree;
}
#undef ClickPolicy
#undef ListTree
#undef TreeView
#undef ZoomLevel

