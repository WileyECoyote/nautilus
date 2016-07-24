/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Nautilus
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Nautilus is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Nautilus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Maciej Stachowiak <mjs@eazel.com>
 *         Ettore Perazzoli <ettore@gnu.org>
 *         Michael Meeks <michael@nuclecu.unam.mx>
 *	   Andy Hertzfeld <andy@eazel.com>
 *
 */

/* nautilus-location-bar.c - Location bar for Nautilus
 */

#include <config.h>

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <eel/eel-icons.h>
#include <eel/eel-canvas.h>
#include <eel/eel-debug.h>
#include <eel/eel-glib-macros.h>

#include <libnautilus-private/nautilus-file-utilities.h>
#include <libnautilus-private/nautilus-entry.h>
#include <libnautilus-private/nautilus-icon-info.h>
#include <libnautilus-private/nautilus-icon-dnd.h>
#include <libnautilus-private/nautilus-clipboard.h>
#include <libnautilus-private/nautilus-directory.h>

#include "nautilus-window-private.h"
#include "nautilus-window.h"

#include "nautilus-location-entry.h"

struct NautilusLocationEntryDetails {
	GtkLabel *label;

	char *current_directory;
	GFilenameCompleter *completer;

	unsigned int idle_id;

	_Bool has_special_text;
	_Bool setting_special_text;
	char  *special_text;
	NautilusLocationEntryAction secondary_action;
};

G_DEFINE_TYPE (NautilusLocationEntry, nautilus_location_entry, NAUTILUS_TYPE_ENTRY);

/* routine that performs the tab expansion.  Extract the directory name and
   incomplete basename, then iterate through the directory trying to complete it.  If we
   find something, add it to the entry */

/* Is really _Bool but glib errently defines gboolean as int */
static int
try_to_expand_path (void *callback_data)
{
	NautilusLocationEntry *entry;
	GtkEditable           *editable;
	char *suffix, *user_location, *absolute_location, *uri_scheme;
	int user_location_length, pos;

	entry = NAUTILUS_LOCATION_ENTRY (callback_data);
	editable = GTK_EDITABLE (entry);
	user_location = gtk_editable_get_chars (editable, 0, -1);
	user_location_length = g_utf8_strlen (user_location, -1);
	entry->details->idle_id = 0;

	uri_scheme = g_uri_parse_scheme (user_location);

	if (!g_path_is_absolute (user_location) && uri_scheme == NULL && user_location[0] != '~') {
		absolute_location = g_build_filename (entry->details->current_directory, user_location, NULL);
		suffix = g_filename_completer_get_completion_suffix (entry->details->completer,
							     absolute_location);
		g_free (absolute_location);
	} else {
		suffix = g_filename_completer_get_completion_suffix (entry->details->completer,
							     user_location);
	}

	g_free (user_location);
	g_free (uri_scheme);

	/* if we've got something, add it to the entry */
	if (suffix != NULL) {
		pos = user_location_length;
		gtk_editable_insert_text (editable,
					  suffix, -1,  &pos);
		pos = user_location_length;
		gtk_editable_select_region (editable, pos, -1);

		g_free (suffix);
	}

	return FALSE;
}

/* Until we have a more elegant solution, this is how we figure out if
 * the GtkEntry inserted characters, assuming that the return value is
 * TRUE indicating that the GtkEntry consumed the key event for some
 * reason. This is a clone of code from GtkEntry.
 */
static _Bool
entry_would_have_inserted_characters (const GdkEventKey *event)
{
	switch (event->keyval) {
	case GDK_KEY_BackSpace:
	case GDK_KEY_Clear:
	case GDK_KEY_Insert:
	case GDK_KEY_Delete:
	case GDK_KEY_Home:
	case GDK_KEY_End:
	case GDK_KEY_KP_Home:
	case GDK_KEY_KP_End:
	case GDK_KEY_Left:
	case GDK_KEY_Right:
	case GDK_KEY_KP_Left:
	case GDK_KEY_KP_Right:
	case GDK_KEY_Return:
		return FALSE;
	default:
		if (event->keyval >= 0x20 && event->keyval <= 0xFF) {
			if ((event->state & GDK_CONTROL_MASK) != 0) {
				return FALSE;
			}
			if ((event->state & GDK_MOD1_MASK) != 0) {
				return FALSE;
			}
		}
		return event->length > 0;
	}
}

static int
get_editable_number_of_chars (GtkEditable *editable)
{
	char *text;
	int length;

	text = gtk_editable_get_chars (editable, 0, -1);
	length = g_utf8_strlen (text, -1);
	g_free (text);
	return length;
}

static void
set_position_and_selection_to_end (GtkEditable *editable)
{
	int end;

	end = get_editable_number_of_chars (editable);
	gtk_editable_select_region (editable, end, end);
	gtk_editable_set_position (editable, end);
}

static _Bool
position_and_selection_are_at_end (GtkEditable *editable)
{
	int end;
	int start_sel, end_sel;

	end = get_editable_number_of_chars (editable);
	if (gtk_editable_get_selection_bounds (editable, &start_sel, &end_sel)) {
		if (start_sel != end || end_sel != end) {
			return FALSE;
		}
	}
	return gtk_editable_get_position (editable) == end;
}

static void
got_completion_data_callback (GFilenameCompleter *completer,
			      NautilusLocationEntry *entry)
{
	EEL_SOURCE_REMOVE_IF_THEN_ZERO (entry->details->idle_id);

	try_to_expand_path (entry);
}

static void
editable_event_after_callback (GtkEntry *entry,
                               GdkEvent *event,
                               NautilusLocationEntry *location_entry)
{
  GtkEditable *editable;
  GdkEventKey *keyevent;

  if (event->type != GDK_KEY_PRESS) {
    return;
  }

  editable = GTK_EDITABLE (entry);
  keyevent = (GdkEventKey *)event;

  /* After typing the right arrow key we move the selection to
   * the end, if we have a valid selection - since this is most
   * likely an auto-completion. We ignore shift / control since
   * they can validly be used to extend the selection.
   */
  if ((keyevent->keyval == GDK_KEY_Right || keyevent->keyval == GDK_KEY_End) &&
    !(keyevent->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) &&
    gtk_editable_get_selection_bounds (editable, NULL, NULL)) {
    set_position_and_selection_to_end (editable);
    }

    /* Only do expanding when we are typing at the end of the
     * text. Do the expand at idle time to avoid slowing down
     * typing when the directory is large. Only trigger the expand
     * when we type a key that would have inserted characters.
     */
    if (position_and_selection_are_at_end (editable)) {
      if (entry_would_have_inserted_characters (keyevent)) {
        if (location_entry->details->idle_id == 0) {
          location_entry->details->idle_id = g_idle_add (try_to_expand_path, location_entry);
        }
      }
    }
    else {
      /* FIXME: Also might be good to do this when you click
       * to change the position or selection.
       */
      EEL_SOURCE_REMOVE_IF_THEN_ZERO (location_entry->details->idle_id);

    }
}

static void
finalize (GObject *object)
{
	NautilusLocationEntry *entry;

	entry = NAUTILUS_LOCATION_ENTRY (object);

	g_object_unref (entry->details->completer);
	g_free (entry->details->special_text);

	G_OBJECT_CLASS (nautilus_location_entry_parent_class)->finalize (object);
}

static void
destroy (GtkWidget *object)
{
	NautilusLocationEntry *entry;

	entry = NAUTILUS_LOCATION_ENTRY (object);

	/* cancel the pending idle call, if any */
	EEL_SOURCE_REMOVE_IF_THEN_ZERO (entry->details->idle_id);

	g_free (entry->details->current_directory);
	entry->details->current_directory = NULL;

	GTK_WIDGET_CLASS (nautilus_location_entry_parent_class)->destroy (object);
}

static void
nautilus_location_entry_text_changed (NautilusLocationEntry *entry,
				      GParamSpec            *pspec)
{
	if (entry->details->setting_special_text) {
		return;
	}

	entry->details->has_special_text = FALSE;
}

static void
nautilus_location_entry_icon_release (GtkEntry *entry,
                                      GtkEntryIconPosition position,
                                      GdkEvent  *event,
                                      void      *unused)
{
  int action;
  action = NAUTILUS_LOCATION_ENTRY (entry)->details->secondary_action;

  switch (action)  {
    case NAUTILUS_LOCATION_ENTRY_ACTION_GOTO:
      g_signal_emit_by_name (entry, "activate", entry);
      break;
    case NAUTILUS_LOCATION_ENTRY_ACTION_CLEAR:
      gtk_entry_set_text (entry, "");
      break;
    default:
      fprintf(stderr, "%s Bad case %d\n", __func__, action);
  }
}

/* Is really _Bool but glib errently defines gboolean as int */
static int
nautilus_location_entry_focus_in (GtkWidget *widget, GdkEventFocus *event)
{
	NautilusLocationEntry *entry = NAUTILUS_LOCATION_ENTRY (widget);

	if (entry->details->has_special_text) {
		entry->details->setting_special_text = TRUE;
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		entry->details->setting_special_text = FALSE;
	}

	return GTK_WIDGET_CLASS (nautilus_location_entry_parent_class)->focus_in_event (widget, event);
}

static void
nautilus_location_entry_activate (GtkEntry *entry)
{
	NautilusLocationEntry *loc_entry;
	const char *entry_text;
	char *full_path, *uri_scheme = NULL;

	loc_entry = NAUTILUS_LOCATION_ENTRY (entry);
	entry_text = gtk_entry_get_text (entry);

	if (entry_text != NULL && *entry_text != '\0') {
		uri_scheme = g_uri_parse_scheme (entry_text);

		if (!g_path_is_absolute (entry_text) && uri_scheme == NULL && entry_text[0] != '~') {
			/* Fix non absolute paths */
			full_path = g_build_filename (loc_entry->details->current_directory, entry_text, NULL);
			gtk_entry_set_text (entry, full_path);
			g_free (full_path);
		}

		g_free (uri_scheme);
	}

	GTK_ENTRY_CLASS (nautilus_location_entry_parent_class)->activate (entry);
}

static void
nautilus_location_entry_class_init (NautilusLocationEntryClass *class)
{
	GtkWidgetClass *widget_class;
	GObjectClass *gobject_class;
	GtkEntryClass *entry_class;

	widget_class = GTK_WIDGET_CLASS (class);
	widget_class->focus_in_event = nautilus_location_entry_focus_in;
	widget_class->destroy = destroy;

	gobject_class = G_OBJECT_CLASS (class);
	gobject_class->finalize = finalize;

	entry_class = GTK_ENTRY_CLASS (class);
	entry_class->activate = nautilus_location_entry_activate;

	g_type_class_add_private (class, sizeof (NautilusLocationEntryDetails));
}

void
nautilus_location_entry_update_current_location (NautilusLocationEntry *entry,
						 const char *location)
{
	g_free (entry->details->current_directory);
	entry->details->current_directory = g_strdup (location);

	nautilus_entry_set_text (NAUTILUS_ENTRY (entry), location);
	set_position_and_selection_to_end (GTK_EDITABLE (entry));
}

void
nautilus_location_entry_set_secondary_action (NautilusLocationEntry *entry,
					      NautilusLocationEntryAction secondary_action)
{
	if (entry->details->secondary_action == secondary_action) {
		return;
	}
	switch (secondary_action) {
	case NAUTILUS_LOCATION_ENTRY_ACTION_CLEAR:
		gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
						   GTK_ENTRY_ICON_SECONDARY,
						   "edit-clear-symbolic");
		break;
	case NAUTILUS_LOCATION_ENTRY_ACTION_GOTO:
		gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
						   GTK_ENTRY_ICON_SECONDARY,
						   "go-next-symbolic");
		break;
	default:
		BUG_IMSG("Invalid secondary_action", secondary_action);
	}
	entry->details->secondary_action = secondary_action;
}

static void
nautilus_location_entry_init (NautilusLocationEntry *entry)
{
	entry->details = G_TYPE_INSTANCE_GET_PRIVATE (entry, NAUTILUS_TYPE_LOCATION_ENTRY,
						      NautilusLocationEntryDetails);

	entry->details->completer = g_filename_completer_new ();
	g_filename_completer_set_dirs_only (entry->details->completer, TRUE);

	nautilus_location_entry_set_secondary_action (entry,
						      NAUTILUS_LOCATION_ENTRY_ACTION_CLEAR);

	nautilus_entry_set_special_tab_handling (NAUTILUS_ENTRY (entry), TRUE);

	g_signal_connect (entry, "event_after",
		          G_CALLBACK (editable_event_after_callback), entry);

	g_signal_connect (entry, "notify::text",
			  G_CALLBACK (nautilus_location_entry_text_changed), NULL);

	g_signal_connect (entry, "icon-release",
			  G_CALLBACK (nautilus_location_entry_icon_release), NULL);

	g_signal_connect (entry->details->completer, "got_completion_data",
		          G_CALLBACK (got_completion_data_callback), entry);
}

GtkWidget *
nautilus_location_entry_new (void)
{
	GtkWidget *entry;

	entry = gtk_widget_new (NAUTILUS_TYPE_LOCATION_ENTRY, NULL);

	return entry;
}

void
nautilus_location_entry_set_special_text (NautilusLocationEntry *entry,
					  const char            *special_text)
{
	entry->details->has_special_text = TRUE;

	g_free (entry->details->special_text);
	entry->details->special_text = g_strdup (special_text);

	entry->details->setting_special_text = TRUE;
	gtk_entry_set_text (GTK_ENTRY (entry), special_text);
	entry->details->setting_special_text = FALSE;
}

