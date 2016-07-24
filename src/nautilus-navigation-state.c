/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* Nautilus - Nautilus navigation state
 *
 * Copyright (C) 2011 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#include <config.h>

//#define DEBUG_NAVIGATION_STATE 1

#include "nautilus-navigation-state.h"

struct _NautilusNavigationStateDetails {
	GtkActionGroup *slave;
	GtkActionGroup *master;

	GList *groups;

	char **action_names;
	GList *active_bindings;
};

enum {
	PROP_SLAVE = 1,
	PROP_MASTER,
	PROP_ACTION_NAMES,
	NUM_PROPERTIES,
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (NautilusNavigationState, nautilus_navigation_state, G_TYPE_OBJECT);

static void
clear_bindings (NautilusNavigationState *self)
{
    g_list_free_full (self->priv->active_bindings, g_object_unref);
    self->priv->active_bindings = NULL;
}

static void
update_bindings (NautilusNavigationState *self)
{
  int length, idx;
  GBinding *binding;
  GtkAction *master_action, *slave_action;

  length = g_strv_length (self->priv->action_names);

  /* The master groups was already checked in nautilus_navigation_state_set_master */
  /* Make sure the slave action group is valid */
  if (GTK_IS_ACTION_GROUP(self->priv->slave)) {

#ifdef DEBUG_NAVIGATION_STATE
    const char *mgn = gtk_action_group_get_name (self->priv->master);
    const char *sgn = gtk_action_group_get_name (self->priv->slave);
    fprintf(stderr, "%s DEBUG: master group=%s, slave group=%s\n", __func__, mgn, sgn);
#endif
    for (idx = 0; idx < length; idx++) {

      master_action = gtk_action_group_get_action (self->priv->master,
                                                   self->priv->action_names[idx]);

      slave_action = gtk_action_group_get_action (self->priv->slave,
                                                  self->priv->action_names[idx]);

      if (GTK_IS_ACTION(slave_action)) {

        if (GTK_IS_ACTION(master_action)) {

          binding = g_object_bind_property (master_action, "sensitive",
                                            slave_action,  "sensitive",
                                            G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

#ifdef DEBUG_NAVIGATION_STATE
fprintf(stderr, "%s action=%s\n", __func__, self->priv->action_names[idx]);
#endif
          /* bind "active" for toggle actions */
          if (GTK_IS_TOGGLE_ACTION (master_action)) {
            binding = g_object_bind_property (master_action, "active",
                                              slave_action,  "active",
                                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
          }
#ifdef DEBUG_NAVIGATION_STATE
fprintf(stderr, "%s done\n", __func__);
#endif
          self->priv->active_bindings = g_list_prepend (self->priv->active_bindings, binding);
        }
        else {
          fprintf(stderr, "%s %s Bad slave action[%d]:<%s>\n", __FILE__,
                  __func__, idx, self->priv->action_names[idx]);
        }
      }
      else {
        fprintf(stderr, "%s %s Bad master action[%d]:<%s>\n", __FILE__,
                __func__, idx, self->priv->action_names[idx]);
      }
    }
  }
  else {
    fprintf(stderr, "%s %s slave is not an Action Group <%p>\n", __FILE__,
            __func__, self->priv->slave);
  }
}

static void
nautilus_navigation_state_init (NautilusNavigationState *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, NAUTILUS_TYPE_NAVIGATION_STATE,
						  NautilusNavigationStateDetails);
}

static void
nautilus_navigation_state_set_property (GObject *object,
					unsigned int property_id,
					const GValue *value,
					GParamSpec *pspec)
{
	NautilusNavigationState *self = NAUTILUS_NAVIGATION_STATE (object);

	switch (property_id) {
	case PROP_SLAVE:
		self->priv->slave = g_value_dup_object (value);
		nautilus_navigation_state_add_group (self, g_value_get_object (value));
		break;
	case PROP_MASTER:
		self->priv->master = g_value_dup_object (value);
		break;
	case PROP_ACTION_NAMES:
		self->priv->action_names = g_value_dup_boxed (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
nautilus_navigation_state_finalize (GObject *obj)
{
	NautilusNavigationState *self = NAUTILUS_NAVIGATION_STATE (obj);

	g_strfreev (self->priv->action_names);

	G_OBJECT_CLASS (nautilus_navigation_state_parent_class)->finalize (obj);
}

static void
nautilus_navigation_state_dispose (GObject *obj)
{
	NautilusNavigationState *self = NAUTILUS_NAVIGATION_STATE (obj);

	clear_bindings (self);

	g_clear_object (&self->priv->slave);
	g_clear_object (&self->priv->master);

	if (self->priv->groups != NULL) {
		g_list_free_full (self->priv->groups, g_object_unref);
		self->priv->groups = NULL;
	}

	G_OBJECT_CLASS (nautilus_navigation_state_parent_class)->dispose (obj);
}

static void
nautilus_navigation_state_class_init (NautilusNavigationStateClass *klass)
{
	GObjectClass *oclass = G_OBJECT_CLASS (klass);

	oclass->dispose = nautilus_navigation_state_dispose;
	oclass->finalize = nautilus_navigation_state_finalize;
	oclass->set_property = nautilus_navigation_state_set_property;

	properties[PROP_SLAVE] =
		g_param_spec_object ("slave",
				     "The slave GtkActionGroup",
				     "The GtkActionGroup that will sync with the current master",
				     GTK_TYPE_ACTION_GROUP,
				     G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
				     G_PARAM_STATIC_STRINGS);
	properties[PROP_MASTER] =
		g_param_spec_object ("master",
				     "The master GtkActionGroup",
				     "The GtkActionGroup that will be used to sync the slave",
				     GTK_TYPE_ACTION_GROUP,
				     G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);
	properties[PROP_ACTION_NAMES] =
		g_param_spec_boxed ("action-names",
				    "The action names to sync",
				    "The action names to sync",
				    G_TYPE_STRV,
				    G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (oclass, NUM_PROPERTIES, properties);
	g_type_class_add_private (klass, sizeof (NautilusNavigationStateDetails));
}

NautilusNavigationState *
nautilus_navigation_state_new (GtkActionGroup *slave,
                               const char **action_names)
{
    return g_object_new (NAUTILUS_TYPE_NAVIGATION_STATE,
                         "slave", slave,
                         "action-names", action_names,
                         NULL);
}

void
nautilus_navigation_state_set_master (NautilusNavigationState *self,
                                      GtkActionGroup          *master)
{
    if (NAUTILUS_IS_NAVIGATION_STATE(self)) {

        if (GTK_IS_ACTION_GROUP(master)) {

            if (self->priv->master != master) {

                if (self->priv->master != NULL) {
                   clear_bindings (self);
                }

                g_clear_object (&self->priv->master);

                self->priv->master = g_object_ref (master);

                update_bindings (self);

                g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MASTER]);
            }
        }
        else {
            fprintf(stderr, "%s Argument is not an Action Group <%p>\n", __func__, master);
        }
    }
    else {
        fprintf(stderr, "%s Bad pointer to instance <%p>\n", __func__, self);
    }
}

void
nautilus_navigation_state_add_group (NautilusNavigationState *self,
				     GtkActionGroup *group)
{
	self->priv->groups = g_list_prepend (self->priv->groups, g_object_ref (group));
}

void
nautilus_navigation_state_sync_all (NautilusNavigationState *self)
{
	GList *l;
	int length, idx;
	const char *action_name;
	GtkAction *action;
	_Bool master_value;
	GtkActionGroup *group;

	length = g_strv_length (self->priv->action_names);

	for (idx = 0; idx < length; idx++) {
		action_name = self->priv->action_names[idx];
		action = gtk_action_group_get_action (self->priv->master,
						      action_name);

		master_value = gtk_action_get_sensitive (action);

		for (l = self->priv->groups; l != NULL; l = l->next) {
			group = l->data;

			action = gtk_action_group_get_action (group, action_name);
			gtk_action_set_sensitive (action, master_value);
		}
	}
}

GtkActionGroup *
nautilus_navigation_state_get_master (NautilusNavigationState *self)
{
	return self->priv->master;
}

void
nautilus_navigation_state_set_boolean (NautilusNavigationState *self,
                                       const char              *action_name,
                                      _Bool                     value)
{
	GtkAction *action;

	action = gtk_action_group_get_action (self->priv->master,
					      action_name);

	if (action == NULL) {
		return;
	}

	gtk_action_set_sensitive (action, value);
}
