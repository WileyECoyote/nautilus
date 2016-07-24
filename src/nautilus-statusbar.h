/* nautilus-statusbar.h
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
 * Free Software Foundation, Inc., 51 Franklin Street - Suite 500,
 * Boston, MA 02110-1335, USA.
 *
 *
 */

#ifndef NAUTILUS_STATUSBAR_H
#define NAUTILUS_STATUSBAR_H

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "nautilus-window.h"
#include "nautilus-window-slot.h"
#include "nautilus-view.h"

typedef struct _NautilusStatusBar      NautilusStatusBar;
typedef struct _NautilusStatusBarClass NautilusStatusBarClass;


#define NAUTILUS_TYPE_STATUS_BAR                 (nautilus_status_bar_get_type ())
#define NAUTILUS_STATUS_BAR(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), NAUTILUS_TYPE_STATUS_BAR, NautilusStatusBar))
#define NAUTILUS_STATUS_BAR_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), NAUTILUS_TYPE_STATUS_BAR, NautilusStatusBarClass))
#define NAUTILUS_IS_STATUS_BAR(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NAUTILUS_TYPE_STATUS_BAR))
#define NAUTILUS_IS_STATUS_BAR_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), NAUTILUS_TYPE_STATUS_BAR))
#define NAUTILUS_STATUS_BAR_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), NAUTILUS_TYPE_STATUS_BAR, NautilusStatusBarClass))

#define NAUTILUS_STATUSBAR_ICON_SIZE_NAME "statusbar-icon"
#define NAUTILUS_STATUSBAR_ICON_SIZE 11

struct _NautilusStatusBar
{
    GtkBox parent;

    NautilusWindow *window;
    GtkWidget *real_statusbar;

    GtkWidget *zoom_slider;

    GtkWidget *tree_button;
    GtkWidget *places_button;
    GtkWidget *show_button;
    GtkWidget *hide_button;
    GtkWidget *separator;
};

struct _NautilusStatusBarClass
{
    GtkBoxClass parent_class;
};

unsigned int nautilus_status_bar_get_type (void) G_GNUC_CONST;

GtkWidget *nautilus_status_bar_new (NautilusWindow *window);

GtkWidget *nautilus_status_bar_get_real_statusbar (NautilusStatusBar *bar);

void       nautilus_status_bar_sync_button_states (NautilusStatusBar *bar);

void       nautilus_status_bar_sync_zoom_widgets (NautilusStatusBar *bar);

#endif /* NAUTILUS_STATUSBAR_H */
