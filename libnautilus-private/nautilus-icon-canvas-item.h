/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* Nautilus - Icon canvas item class for icon container.
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Author: Andy Hertzfeld <andy@eazel.com>
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
 */

#ifndef NAUTILUS_ICON_CANVAS_ITEM_H
#define NAUTILUS_ICON_CANVAS_ITEM_H

#include <eel/eel-canvas.h>
#include <eel/eel-art-extensions.h>

G_BEGIN_DECLS

#define NAUTILUS_TYPE_ICON_CANVAS_ITEM nautilus_icon_canvas_item_get_type()
#define NAUTILUS_ICON_CANVAS_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NAUTILUS_TYPE_ICON_CANVAS_ITEM, NautilusIconCanvasItem))
#define NAUTILUS_ICON_CANVAS_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NAUTILUS_TYPE_ICON_CANVAS_ITEM, NautilusIconCanvasItemClass))
#define NAUTILUS_IS_ICON_CANVAS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NAUTILUS_TYPE_ICON_CANVAS_ITEM))
#define NAUTILUS_IS_ICON_CANVAS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NAUTILUS_TYPE_ICON_CANVAS_ITEM))
#define NAUTILUS_ICON_CANVAS_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NAUTILUS_TYPE_ICON_CANVAS_ITEM, NautilusIconCanvasItemClass))

typedef struct NautilusIconCanvasItem NautilusIconCanvasItem;
typedef struct NautilusIconCanvasItemClass NautilusIconCanvasItemClass;
typedef struct NautilusIconCanvasItemDetails NautilusIconCanvasItemDetails;

struct NautilusIconCanvasItem {
	EelCanvasItem item;
	NautilusIconCanvasItemDetails *details;
	void *user_data;
        char *tooltip;
        int   click_policy;
};

struct NautilusIconCanvasItemClass {
	EelCanvasItemClass parent_class;
};

/* not namespaced due to their length */
typedef enum {
	BOUNDS_USAGE_FOR_LAYOUT,
	BOUNDS_USAGE_FOR_ENTIRE_ITEM,
	BOUNDS_USAGE_FOR_DISPLAY
} NautilusIconCanvasItemBoundsUsage;

/* GObject */
GType       nautilus_icon_canvas_item_get_type                 (void);

/* attributes */
void        nautilus_icon_canvas_item_set_image                (NautilusIconCanvasItem   *item,
                                                                GdkPixbuf                *image);
cairo_surface_t* nautilus_icon_canvas_item_get_drag_surface    (NautilusIconCanvasItem   *item);
void        nautilus_icon_canvas_item_set_emblems              (NautilusIconCanvasItem   *item,
                                                               GList                     *emblem_pixbufs);
void        nautilus_icon_canvas_item_set_show_stretch_handles (NautilusIconCanvasItem   *item,
                                                               _Bool                      show_stretch_handles);
void        nautilus_icon_canvas_item_set_attach_points        (NautilusIconCanvasItem   *item,
                                                               GdkPoint                  *attach_points,
                                                               int                        n_attach_points);
void        nautilus_icon_canvas_item_set_embedded_text_rect   (NautilusIconCanvasItem   *item,
                                                               const GdkRectangle        *text_rect);
void        nautilus_icon_canvas_item_set_embedded_text        (NautilusIconCanvasItem   *item,
                                                               const char                *text);
double      nautilus_icon_canvas_item_get_max_text_width       (NautilusIconCanvasItem   *item);
const char *nautilus_icon_canvas_item_get_editable_text        (NautilusIconCanvasItem   *icon_item);
void        nautilus_icon_canvas_item_set_renaming             (NautilusIconCanvasItem   *icon_item,
                                                               _Bool                      state);

/* geometry and hit testing */
_Bool       nautilus_icon_canvas_item_hit_test_rectangle      (NautilusIconCanvasItem        *item,
                                                               EelIRect                       canvas_rect);
_Bool       nautilus_icon_canvas_item_hit_test_stretch_handles (NautilusIconCanvasItem       *item,
                                                               double                         world_x,
                                                               double                         world_y,
                                                               GtkCornerType                 *corner);
void        nautilus_icon_canvas_item_invalidate_label         (NautilusIconCanvasItem       *item);
void        nautilus_icon_canvas_item_invalidate_label_size    (NautilusIconCanvasItem       *item);
EelDRect    nautilus_icon_canvas_item_get_icon_rectangle       (const NautilusIconCanvasItem *item);
EelDRect    nautilus_icon_canvas_item_get_text_rectangle       (NautilusIconCanvasItem       *item,
                                                               _Bool                          for_layout);
void        nautilus_icon_canvas_item_get_bounds_for_layout    (NautilusIconCanvasItem       *item,
                                                                double *x1, double *y1, double *x2, double *y2);
void        nautilus_icon_canvas_item_get_bounds_for_entire_item (NautilusIconCanvasItem     *item,
                                                                 double *x1, double *y1, double *x2, double *y2);
void        nautilus_icon_canvas_item_update_bounds            (NautilusIconCanvasItem       *item,
                                                               double i2w_dx, double i2w_dy);
void        nautilus_icon_canvas_item_set_is_visible           (NautilusIconCanvasItem       *item,
                                                               _Bool                      visible);
/* whether the entire label text must be visible at all times */
void        nautilus_icon_canvas_item_set_entire_text          (NautilusIconCanvasItem       *icon_item,
                                                               _Bool                      entire_text);
void        nautilus_icon_canvas_item_set_tooltip_text         (NautilusIconCanvasItem *item, const gchar *text);

G_END_DECLS

#endif /* NAUTILUS_ICON_CANVAS_ITEM_H */
