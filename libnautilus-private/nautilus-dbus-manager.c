/*
 * nautilus-dbus-manager: nautilus DBus interface
 *
 * Copyright (C) 2010, Red Hat, Inc.
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

#include "nautilus-dbus-manager.h"
#include "nautilus-generated.h"

#include "nautilus-file-operations.h"

#define DEBUG_FLAG NAUTILUS_DEBUG_DBUS
#include "nautilus-debug.h"

#include <gio/gio.h>

#define SERVICE_TIMEOUT 5

typedef struct _NautilusDBusManager NautilusDBusManager;
typedef struct _NautilusDBusManagerClass NautilusDBusManagerClass;

struct _NautilusDBusManager {
  GObject parent;

  GDBusConnection            *connection;
  GApplication               *application;

  GDBusObjectManagerServer   *object_manager;
  NautilusDBusFileOperations *file_operations;

           int time_out;  /* Service Time Out Settings*/
  unsigned int owner_id;
};

struct _NautilusDBusManagerClass {
  GObjectClass parent_class;
};

enum {
  PROP_APPLICATION = 1,
  PROP_TIMEOUT,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

GType nautilus_dbus_manager_get_type (void) G_GNUC_CONST;
G_DEFINE_TYPE (NautilusDBusManager, nautilus_dbus_manager, G_TYPE_OBJECT);

//static NautilusDBusManager *singleton = NULL;

static void
nautilus_dbus_manager_dispose (GObject *object)
{
  NautilusDBusManager *self = (NautilusDBusManager *) object;

  /* Unown before unregistering so we're not registred in a partial state */
  if (self->owner_id != 0)
    {
      g_bus_unown_name (self->owner_id);
      self->owner_id = 0;
    }

  if (self->file_operations) {
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (self->file_operations));
    g_object_unref (self->file_operations);
    self->file_operations = NULL;
  }

  if (self->object_manager) {
    g_object_unref (self->object_manager);
    self->object_manager = NULL;
  }

  g_clear_object (&self->connection);

  G_OBJECT_CLASS (nautilus_dbus_manager_parent_class)->dispose (object);
}

/* Is really _Bool but glib errently defines gboolean as int */
static int
service_timeout_handler (void *user_data)
{
  NautilusDBusManager *self = user_data;

  DEBUG ("Reached the DBus service timeout");

  /* just unconditionally release here, as if an operation has been
   * called, its progress handler will hold it alive for all the task duration.
   */
  g_application_release (self->application);

  return FALSE;
}

static _Bool
handle_copy_file (NautilusDBusFileOperations *object,
                  GDBusMethodInvocation      *invocation,
                  const char                 *source_uri,
                  const char                 *source_display_name,
                  const char                 *dest_dir_uri,
                  const char                 *dest_name)
{
  GFile *source_file;
  GFile *target_dir;

  const char *target_name = NULL;
  const char *source_name = NULL;

  source_file = g_file_new_for_uri (source_uri);
  target_dir  = g_file_new_for_uri (dest_dir_uri);

  if (dest_name != NULL && dest_name[0] != '\0')
    target_name = dest_name;

  if (source_display_name != NULL && source_display_name[0] != '\0')
    source_name = source_display_name;

  nautilus_file_operations_copy_file (source_file, target_dir, source_name,
                                      target_name, NULL, NULL, NULL);

  g_object_unref (source_file);
  g_object_unref (target_dir);

  nautilus_dbus_file_operations_complete_copy_file (object, invocation);

  return TRUE; /* invocation was handled */
}

static _Bool
handle_copy_uris (NautilusDBusFileOperations *object,
                  GDBusMethodInvocation      *invocation,
                  const char                **sources,
                  const char                 *destination)
{
  GList *source_files = NULL;
  GFile *dest_dir;
  int idx;

  dest_dir = g_file_new_for_uri (destination);

  for (idx = 0; sources[idx] != NULL; idx++)
    source_files = g_list_prepend (source_files,
                                   g_file_new_for_uri (sources[idx]));

  nautilus_file_operations_copy (source_files, NULL,
                                 dest_dir,
                                 NULL, NULL, NULL);

  g_list_free_full (source_files, g_object_unref);
  g_object_unref (dest_dir);

  nautilus_dbus_file_operations_complete_copy_uris (object, invocation);
  return TRUE; /* invocation was handled */
}

static _Bool
handle_empty_trash (NautilusDBusFileOperations *object,
                    GDBusMethodInvocation      *invocation)
{
  nautilus_file_operations_empty_trash (NULL);

  nautilus_dbus_file_operations_complete_empty_trash (object, invocation);
  return TRUE; /* invocation was handled */
}

static void
bus_acquired_handler_cb (GDBusConnection *conn, const char *name, void *user_data)
{
  NautilusDBusManager *self = user_data;

  DEBUG ("Bus acquired at %s", name);

  self->connection = g_object_ref (conn);

  self->object_manager = g_dbus_object_manager_server_new ("/apps/Nautilus");

  self->file_operations = nautilus_dbus_file_operations_skeleton_new ();

  g_signal_connect (self->file_operations,
                    "handle-copy-uris",
                    G_CALLBACK (handle_copy_uris),
                    self);
  g_signal_connect (self->file_operations,
                    "handle-copy-file",
                    G_CALLBACK (handle_copy_file),
                    self);
  g_signal_connect (self->file_operations,
                    "handle-empty-trash",
                    G_CALLBACK (handle_empty_trash),
                    self);

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self->file_operations), self->connection,
                                    "/apps/Nautilus", NULL);

  g_dbus_object_manager_server_set_connection (self->object_manager, self->connection);

  g_timeout_add_seconds (self->time_out, service_timeout_handler, self);
}

static void
on_name_lost (GDBusConnection *connection, const char *name, void *user_data)
{
  DEBUG ("Lost (or failed to acquire) the name %s on the session message bus\n", name);
}

static void
on_name_acquired (GDBusConnection *connection, const char *name, void *user_data)
{
  DEBUG ("Acquired the name %s on the session message bus\n", name);
}

static void
nautilus_dbus_manager_init (NautilusDBusManager *self)
{
  /* do nothing */
}

static void
nautilus_dbus_manager_get_property (GObject     *object,
                                    unsigned int property_id,
                                    GValue      *value,
                                    GParamSpec  *pspec)
{
  NautilusDBusManager *self = (NautilusDBusManager *) (object);

  switch (property_id)
    {
    case PROP_TIMEOUT:
      g_value_set_int (value, self->time_out);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
static void
nautilus_dbus_manager_set_property (GObject *object,
                                    unsigned int property_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
  NautilusDBusManager *self = (NautilusDBusManager *) (object);

  switch (property_id)
    {
    case PROP_APPLICATION:
      self->application = g_value_get_object (value);
      break;
    case PROP_TIMEOUT:
      self->time_out = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
nautilus_dbus_manager_constructed (GObject *object)
{
  NautilusDBusManager *self = (NautilusDBusManager *) (object);

  G_OBJECT_CLASS (nautilus_dbus_manager_parent_class)->constructed (object);

  g_application_hold (self->application);

  self->owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
								   "apps.Nautilus",
								   G_BUS_NAME_OWNER_FLAGS_NONE,
								   bus_acquired_handler_cb,
								   on_name_acquired,
								   on_name_lost,
								   self,
								   NULL);
}

static void
nautilus_dbus_manager_class_init (NautilusDBusManagerClass *klass)
{
    GObjectClass *oclass = G_OBJECT_CLASS (klass);

    oclass->dispose       = nautilus_dbus_manager_dispose;
    oclass->constructed   = nautilus_dbus_manager_constructed;
    oclass->get_property  = nautilus_dbus_manager_get_property;
    oclass->set_property  = nautilus_dbus_manager_set_property;

    properties[PROP_APPLICATION] = g_param_spec_object ("application",
                                                        "GApplication instance",
                                                        "The owning GApplication instance",
                                                        G_TYPE_APPLICATION,
                                                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS);

    properties[PROP_TIMEOUT] = g_param_spec_int ("service-timeout",
                                                 "Service Timeout",
                                                 "Service Timeout",
                                                  1, 100, 5,
                                                  G_PARAM_READWRITE );

    g_object_class_install_properties (oclass, NUM_PROPERTIES, properties);
}

NautilusDBusManager *
nautilus_dbus_manager_new (GApplication *application)
{
  return g_object_new (nautilus_dbus_manager_get_type (),
                            "application", application,
                            NULL);
}
