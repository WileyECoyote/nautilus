/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Nautilus
 *
 * Copyright (C) 2011, Red Hat, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, MA 02110-1335, USA.
 *
 * Author: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#include <config.h>
#include <glib/gi18n.h>

#include <eel/eel-canvas.h>
#include <eel/eel-icons.h>

#include <libnautilus-private/nautilus-icon-info.h>
#include "libnautilus-private/nautilus-directory.h"
#include <libnautilus-private/nautilus-global-preferences.h>
#include <libnautilus-private/nautilus-ui-utilities.h>

#include "nautilus-toolbar.h"

#include "nautilus-location-bar.h"
#include "nautilus-pathbar.h"
#include "nautilus-window-private.h"


struct _NautilusToolbarPriv {
    GtkToolbar     *toolbar;
    GtkToolbar     *secondary_toolbar;

    GtkActionGroup *action_group;
    GtkActionGroup *clibboard_actions;
    GtkUIManager   *ui_manager;

    GtkWidget      *path_bar;
    GtkWidget      *location_bar;
    GtkWidget      *search_bar;
    GtkWidget      *root_bar;

    _Bool    show_main_bar;
    _Bool    show_location_entry;
    _Bool    show_search_bar;
    _Bool    show_root_bar;
};

enum {
	PROP_ACTION_GROUP = 1,
	PROP_SHOW_LOCATION_ENTRY,
	PROP_SHOW_SEARCH_BAR,
	PROP_SHOW_MAIN_BAR,
	NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (NautilusToolbar, nautilus_toolbar, GTK_TYPE_BOX);

static void
setup_root_info_bar (NautilusToolbar *self) {

    GtkWidget *root_bar = gtk_info_bar_new ();
    gtk_info_bar_set_message_type (GTK_INFO_BAR (root_bar), GTK_MESSAGE_ERROR);
    GtkWidget *content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (root_bar));

    GtkWidget *label = gtk_label_new (_("Elevated Privileges"));
    gtk_widget_show (label);
    gtk_container_add (GTK_CONTAINER (content_area), label);

    self->priv->root_bar = root_bar;
    gtk_box_pack_start (GTK_BOX (self), self->priv->root_bar, TRUE, TRUE, 0);
}

void
nautilus_toolbar_update_root_state (NautilusToolbar *self)
{
    if (eel_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_WARN_ROOT) &&  (geteuid() == 0))
    {
        if (self->priv->show_root_bar != TRUE) {
            self->priv->show_root_bar = TRUE;
        }
    }
    else {
        self->priv->show_root_bar = FALSE;
    }

}

static void
toolbar_update_appearance (NautilusToolbar *self)
{
  _Bool show_widget;

  nautilus_toolbar_update_root_state (self);

  show_widget = self->priv->show_location_entry; /* it's called "pre-re-using" a variable */

  gtk_widget_set_visible (GTK_WIDGET(self->priv->toolbar), self->priv->show_main_bar);

  gtk_widget_set_visible (self->priv->location_bar, show_widget);

  gtk_widget_set_visible (self->priv->path_bar, !show_widget);

  gtk_widget_set_visible (self->priv->search_bar, self->priv->show_search_bar);

  gtk_widget_set_visible (self->priv->root_bar, self->priv->show_root_bar);

  if (self->up_butt == NULL) {
    self->up_butt = gtk_ui_manager_get_widget (self->priv->ui_manager, "/Toolbar/Up");
  }
  if (self->up_butt != NULL) {
    show_widget = eel_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_UP_ICON_TOOLBAR);
    gtk_widget_set_visible(self->up_butt, show_widget);
  }

  if (self->reload_butt == NULL) {
    self->reload_butt = gtk_ui_manager_get_widget (self->priv->ui_manager, "/Toolbar/Reload");
  }
  if (self->reload_butt != NULL) {
    show_widget = eel_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_RELOAD_ICON_TOOLBAR);
    gtk_widget_set_visible(self->reload_butt, show_widget);
  }

  if (self->edit_butt == NULL) {
    self->edit_butt = gtk_ui_manager_get_widget (self->priv->ui_manager, "/SecondaryToolbar/Edit Location");
  }
  if (self->edit_butt != NULL) {
    show_widget = eel_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_EDIT_ICON_TOOLBAR);
    gtk_widget_set_visible(self->edit_butt, show_widget);
  }

  if (self->home_butt == NULL) {
    self->home_butt = gtk_ui_manager_get_widget (self->priv->ui_manager, "/Toolbar/Home");
  }
  if (self->home_butt != NULL) {
    show_widget = eel_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_HOME_ICON_TOOLBAR);
    gtk_widget_set_visible(self->home_butt, show_widget);
  }

  if (self->computer_butt == NULL) {
    self->computer_butt = gtk_ui_manager_get_widget (self->priv->ui_manager, "/Toolbar/Computer");
  }
  if (self->computer_butt != NULL) {
    show_widget = eel_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_COMPUTER_ICON_TOOLBAR);
    gtk_widget_set_visible(self->computer_butt, show_widget);
  }

  if (self->cut_butt == NULL) {
    self->cut_butt = gtk_ui_manager_get_widget (self->priv->ui_manager, "/Toolbar/Cut");
  }
  if (self->cut_butt != NULL) {
    show_widget = eel_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_CUT_TOOLBAR);
    gtk_widget_set_visible(self->cut_butt, show_widget);
  }

  if (self->copy_butt == NULL) {
    self->copy_butt = gtk_ui_manager_get_widget (self->priv->ui_manager, "/Toolbar/Copy");
  }
  if (self->copy_butt != NULL) {
    show_widget = eel_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_COPY_TOOLBAR);
    gtk_widget_set_visible(self->copy_butt, show_widget);
  }

  if (self->paste_butt == NULL) {
    self->paste_butt = gtk_ui_manager_get_widget (self->priv->ui_manager, "/Toolbar/Paste");
  }
  if (self->paste_butt != NULL) {
    show_widget = eel_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_PASTE_TOOLBAR);
    gtk_widget_set_visible(self->paste_butt, show_widget);
  }

  if (self->search_butt == NULL) {
    self->search_butt = gtk_ui_manager_get_widget (self->priv->ui_manager, "/SecondaryToolbar/Search");
  }
  if (self->search_butt != NULL) {
    show_widget = eel_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_SEARCH_ICON_TOOLBAR);
    gtk_widget_set_visible(self->search_butt, show_widget);
  }

  if (self->new_butt == NULL) {
    self->new_butt = gtk_ui_manager_get_widget (self->priv->ui_manager, "/SecondaryToolbar/New Folder");
  }
  if (self->new_butt != NULL) {
    show_widget = eel_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_NEW_FOLDER_ICON_TOOLBAR);
    gtk_widget_set_visible(self->new_butt, show_widget);
  }
}
/*
static void
toolbar_remap_layout (NautilusToolbar *self)
{

}
*/
static void
nautilus_toolbar_constructed (GObject *obj)
{
    NautilusToolbar *self = NAUTILUS_TOOLBAR (obj);
    GtkToolItem     *item;
    GtkBox          *hbox;
    GtkToolbar      *toolbar, *secondary_toolbar;
    GtkStyleContext *context;
    GError          *error;

    G_OBJECT_CLASS (nautilus_toolbar_parent_class)->constructed (obj);

    gtk_style_context_set_junction_sides (gtk_widget_get_style_context (GTK_WIDGET (self)),
                                          GTK_JUNCTION_BOTTOM);

    self->priv->show_location_entry = eel_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_LOCATION_ENTRY);

    /* add the UI */
    self->priv->ui_manager = gtk_ui_manager_new ();

    error = NULL;
    if (!gtk_ui_manager_add_ui_from_resource (self->priv->ui_manager, "/apps/nautilus/nautilus-toolbar-ui.xml", &error))
    {
      eel_show_error_dialog (_("<b>Unable to load Toolbar resouces!</b>"),error->message, "Nautilus", NULL);
      g_clear_error (&error);
      self->priv->toolbar = NULL;
    }

    gtk_ui_manager_insert_action_group (self->priv->ui_manager, self->priv->action_group, 0);

    toolbar = GTK_TOOLBAR (gtk_ui_manager_get_widget (self->priv->ui_manager, "/Toolbar"));
    self->priv->toolbar = toolbar;

    secondary_toolbar = GTK_TOOLBAR (gtk_ui_manager_get_widget (self->priv->ui_manager, "/SecondaryToolbar"));
    self->priv->secondary_toolbar = secondary_toolbar;

    if (GTK_IS_TOOLBAR(toolbar)) {
       gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar), STD_ICON_SIZE_SMALL_TOOLBAR);
       context = gtk_widget_get_style_context (GTK_WIDGET(toolbar));
       gtk_style_context_add_class (context, GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
    }

    if (GTK_IS_TOOLBAR(secondary_toolbar)) {
       gtk_toolbar_set_icon_size (GTK_TOOLBAR (secondary_toolbar), STD_ICON_SIZE_SMALL_TOOLBAR);
       context = gtk_widget_get_style_context (GTK_WIDGET(secondary_toolbar));
       gtk_style_context_add_class (context, GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
    }

    //search = gtk_ui_manager_get_widget (self->priv->ui_manager, "/Toolbar/Search");
    //gtk_style_context_add_class (gtk_widget_get_style_context (search), GTK_STYLE_CLASS_RAISED);
    //gtk_widget_set_name (search, "nautilus-search-button");

    hbox = GTK_BOX(gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));

    gtk_box_pack_start (hbox, GTK_WIDGET(self->priv->toolbar), TRUE, TRUE, 0);
    gtk_widget_show_all (GTK_WIDGET(self->priv->toolbar));

    gtk_toolbar_set_show_arrow (self->priv->secondary_toolbar, FALSE);
    gtk_box_pack_start (hbox, GTK_WIDGET(self->priv->secondary_toolbar), FALSE, TRUE, 0);
    gtk_widget_show_all (GTK_WIDGET(self->priv->secondary_toolbar));

    gtk_box_pack_start (GTK_BOX (self), GTK_WIDGET(hbox), TRUE, TRUE, 0);
    gtk_widget_show_all (GTK_WIDGET(hbox));

    hbox = GTK_BOX(gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_widget_show (GTK_WIDGET(hbox));

    GtkToolItem *gtk_sucks = gtk_separator_tool_item_new ();
    gtk_separator_tool_item_set_draw ((GtkSeparatorToolItem*)gtk_sucks, TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_sucks, -1);
    gtk_widget_show (GTK_WIDGET(gtk_sucks));

    /* regular path bar */
    self->priv->path_bar = g_object_new (NAUTILUS_TYPE_PATH_BAR, NULL);

    /* entry-like location bar */
    self->priv->location_bar = nautilus_location_bar_new ();

    if (!eel_settings_get_boolean (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_PATH_IN_PANE))
    {
      gtk_box_pack_start (GTK_BOX (hbox), self->priv->location_bar, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), self->priv->path_bar, TRUE, TRUE, 0);
    }

    item = gtk_tool_item_new ();
    gtk_tool_item_set_expand (item, TRUE);
    gtk_container_add (GTK_CONTAINER (item), GTK_WIDGET(hbox));
    /* append to the end of the toolbar so navigation buttons are at the beginning */
    gtk_toolbar_insert (GTK_TOOLBAR (self->priv->toolbar), item, -1);
    gtk_widget_show (GTK_WIDGET (item));

    setup_root_info_bar (self);

    /* search bar */
    self->priv->search_bar = nautilus_search_bar_new ();
    gtk_box_pack_start (GTK_BOX (self), self->priv->search_bar, TRUE, TRUE, 0);

    toolbar_update_appearance (self);

    /* nautilus patch */
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_SHOW_UP_ICON_TOOLBAR,
                              G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_SHOW_EDIT_ICON_TOOLBAR,
                              G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_SHOW_RELOAD_ICON_TOOLBAR,
                              G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_SHOW_HOME_ICON_TOOLBAR,
                              G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_SHOW_COMPUTER_ICON_TOOLBAR,
                              G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_SHOW_CUT_TOOLBAR,
                              G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_SHOW_COPY_TOOLBAR,
                              G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_SHOW_PASTE_TOOLBAR,
                              G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_SHOW_SEARCH_ICON_TOOLBAR,
                              G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_SHOW_LABEL_SEARCH_ICON_TOOLBAR,
                              G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_SHOW_NEW_FOLDER_ICON_TOOLBAR,
                              G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_SHOW_WARN_ROOT,
                              G_CALLBACK (toolbar_update_appearance), self);
/*
    g_signal_connect_swapped (nautilus_preferences,
                              "changed::" NAUTILUS_PREFERENCES_SHOW_PATH_IN_PANE,
                              G_CALLBACK (toolbar_remap_layout), self);
*/

}

static void
nautilus_toolbar_init (NautilusToolbar *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, NAUTILUS_TYPE_TOOLBAR,
						  NautilusToolbarPriv);
	self->priv->show_main_bar = TRUE;

        self->up_butt             = NULL;
        self->reload_butt         = NULL;
        self->edit_butt           = NULL;
        self->home_butt           = NULL;
        self->computer_butt       = NULL;
        self->cut_butt            = NULL;
        self->copy_butt           = NULL;
        self->paste_butt          = NULL;
        self->search_butt         = NULL;
        self->new_butt            = NULL;
}

static void
nautilus_toolbar_get_property (GObject *object,
			       unsigned int property_id,
			       GValue *value,
			       GParamSpec *pspec)
{
	NautilusToolbar *self = NAUTILUS_TOOLBAR (object);

	switch (property_id) {
	case PROP_SHOW_LOCATION_ENTRY:
		g_value_set_boolean (value, self->priv->show_location_entry);
		break;
	case PROP_SHOW_SEARCH_BAR:
		g_value_set_boolean (value, self->priv->show_search_bar);
		break;
	case PROP_SHOW_MAIN_BAR:
		g_value_set_boolean (value, self->priv->show_main_bar);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
nautilus_toolbar_set_property (GObject *object,
			       unsigned int property_id,
			       const GValue *value,
			       GParamSpec *pspec)
{
	NautilusToolbar *self = NAUTILUS_TOOLBAR (object);

	switch (property_id) {
	case PROP_ACTION_GROUP:
		self->priv->action_group = g_value_dup_object (value);
		break;
	case PROP_SHOW_LOCATION_ENTRY:
		nautilus_toolbar_set_show_location_entry (self, g_value_get_boolean (value));
		break;
	case PROP_SHOW_SEARCH_BAR:
		nautilus_toolbar_set_show_search_bar (self, g_value_get_boolean (value));
		break;
	case PROP_SHOW_MAIN_BAR:
		nautilus_toolbar_set_show_main_bar (self, g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
nautilus_toolbar_dispose (GObject *obj)
{
	NautilusToolbar *self = NAUTILUS_TOOLBAR (obj);

	g_clear_object (&self->priv->ui_manager);
	g_clear_object (&self->priv->action_group);

	g_signal_handlers_disconnect_by_func (nautilus_preferences,
					      toolbar_update_appearance, self);

	G_OBJECT_CLASS (nautilus_toolbar_parent_class)->dispose (obj);
}

static void
nautilus_toolbar_class_init (NautilusToolbarClass *klass)
{
	GObjectClass *oclass;

	oclass               = G_OBJECT_CLASS (klass);

	oclass->get_property = nautilus_toolbar_get_property;
	oclass->set_property = nautilus_toolbar_set_property;
	oclass->constructed  = nautilus_toolbar_constructed;
	oclass->dispose      = nautilus_toolbar_dispose;

	properties[PROP_ACTION_GROUP] =
		g_param_spec_object ("action-group",
				     "The action group",
				     "The action group to get actions from",
				     GTK_TYPE_ACTION_GROUP,
				     G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY |
				     G_PARAM_STATIC_STRINGS);
	properties[PROP_SHOW_LOCATION_ENTRY] =
		g_param_spec_boolean ("show-location-entry",
				      "Whether to show the location entry",
				      "Whether to show the location entry instead of the pathbar",
				      FALSE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	properties[PROP_SHOW_SEARCH_BAR] =
		g_param_spec_boolean ("show-search-bar",
				      "Whether to show the search bar",
				      "Whether to show the search bar beside the toolbar",
				      FALSE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	properties[PROP_SHOW_MAIN_BAR] =
		g_param_spec_boolean ("show-main-bar",
				      "Whether to show the main bar",
				      "Whether to show the main toolbar",
				      TRUE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_type_class_add_private (klass, sizeof (NautilusToolbarClass));
	g_object_class_install_properties (oclass, NUM_PROPERTIES, properties);
}

NautilusToolbar *
nautilus_toolbar_new (GtkActionGroup *action_group)
{
	return g_object_new (NAUTILUS_TYPE_TOOLBAR,
			     "action-group", action_group,
			     "orientation", GTK_ORIENTATION_VERTICAL,
			     NULL);
}

GtkWidget *
nautilus_toolbar_get_path_bar (NautilusToolbar *self)
{
	return self->priv->path_bar;
}

GtkWidget *
nautilus_toolbar_get_location_bar (NautilusToolbar *self)
{
	return self->priv->location_bar;
}

GtkWidget *
nautilus_toolbar_get_search_bar (NautilusToolbar *self)
{
	return self->priv->search_bar;
}

_Bool
nautilus_toolbar_get_show_location_entry (NautilusToolbar *self)
{
	return self->priv->show_location_entry;
}

void
nautilus_toolbar_set_show_main_bar (NautilusToolbar *self,
				    _Bool    show_main_bar)
{
	if (show_main_bar != self->priv->show_main_bar) {
		self->priv->show_main_bar = show_main_bar;
		toolbar_update_appearance (self);

		g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_MAIN_BAR]);
	}
}

void
nautilus_toolbar_set_show_location_entry (NautilusToolbar *self,
					  _Bool    show_location_entry)
{
	if (show_location_entry != self->priv->show_location_entry) {
		self->priv->show_location_entry = show_location_entry;
		toolbar_update_appearance (self);

		g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_LOCATION_ENTRY]);
	}
}

void
nautilus_toolbar_set_show_search_bar (NautilusToolbar *self,
				      _Bool    show_search_bar)
{
	if (show_search_bar != self->priv->show_search_bar) {
		self->priv->show_search_bar = show_search_bar;
		toolbar_update_appearance (self);

		g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_SEARCH_BAR]);
	}
}
