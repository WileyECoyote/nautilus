/*
 * nautilus-freedesktop-dbus: Implementation for the org.freedesktop DBus file-management interfaces
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
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1335, USA, <http://www.gnu.org/licenses/>
 *
 * Authors: Akshay Gupta <kitallis@gmail.com>
 *          Federico Mena Quintero <federico@gnome.org>
 */


#ifndef __NAUTILUS_FREEDESKTOP_DBUS_H__
#define __NAUTILUS_FREEDESKTOP_DBUS_H__

#include "nautilus-application.h"

#include <glib-object.h>

#define NAUTILUS_TYPE_FREEDESKTOP_DBUS            (nautilus_freedesktop_dbus_get_type())
#define NAUTILUS_FREEDESKTOP_DBUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NAUTILUS_TYPE_FREEDESKTOP_DBUS, NemoFreedesktopDBus))
#define NAUTILUS_FREEDESKTOP_DBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NAUTILUS_TYPE_FREEDESKTOP_DBUS, NemoFreedesktopDBusClass))
#define NAUTILUS_IS_FREEDESKTOP_DBUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NAUTILUS_TYPE_FREEDESKTOP_DBUS))
#define NAUTILUS_IS_FREEDESKTOP_DBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NAUTILUS_TYPE_FREEDESKTOP_DBUS))
#define NAUTILUS_FREEDESKTOP_DBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NAUTILUS_TYPE_FREEDESKTOP_DBUS, NemoFreedesktopDBusClass))

typedef struct _NautilusFreedesktopDBus NautilusFreedesktopDBus;
typedef struct _NautilusFreedesktopDBusClass NautilusFreedesktopDBusClass;

GType nautilus_freedesktop_dbus_get_type (void);

NautilusFreedesktopDBus *nautilus_freedesktop_dbus_new (GApplication *app);

void nautilus_freedesktop_dbus_set_open_locations (NautilusFreedesktopDBus *fdb, const char **locations);

#endif /* __NAUTILUS_FREEDESKTOP_DBUS_H__ */
