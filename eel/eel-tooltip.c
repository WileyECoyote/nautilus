/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-
 */
/*! \file eel-tooltip.c: enhance GTK tool tips.
 *
 * Copyright (C) 2014 Wiley Edward Hill
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the historicalied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Wiley Edward Hill <wileyhill@gmail.com>
 * Date:    05/14/2014
*/

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "eel-tooltip.h"

G_DEFINE_TYPE (EelTooltip, eel_tooltip, G_TYPE_OBJECT)

static int
hide_tooltip (void *data)
{
  GtkWindow  *window;

  window = data;

  if (GTK_IS_WINDOW(window)) {
     g_signal_emit_by_name (window, "hide", NULL);
  }
  return FALSE;
}

static void
eel_tooltip_dispose (GObject   *object)
{
  EelTooltip *tooltip = EEL_TOOLTIP (object);

  if (tooltip->window) {

      gtk_widget_destroy (GTK_WIDGET(tooltip->window));
      tooltip->window = NULL;
    }

  G_OBJECT_CLASS (eel_tooltip_parent_class)->dispose (object);
}

static void
eel_tooltip_finalize (GObject *object)
{
  EelTooltip *tooltip = EEL_TOOLTIP (object);

  G_OBJECT_CLASS (eel_tooltip_parent_class)->finalize (object);
}

static void
eel_tooltip_class_init (EelTooltipClass *class)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (class);

  object_class->dispose = eel_tooltip_dispose;

  object_class->finalize  = eel_tooltip_finalize;

}
static void
paint_background_and_frame (GtkWidget *window, cairo_t *cr)
{
  GtkStyleContext *context;
  int width, height;

  width = gtk_widget_get_allocated_width (window);
  height = gtk_widget_get_allocated_height (window);
  context = gtk_widget_get_style_context (window);

  gtk_render_background (context, cr, 0, 0, width, height);

  gtk_render_frame (context, cr, 0, 0, width, height);
}

static void
maybe_update_shape (GtkWidget *window)
{
  cairo_t         *cr;
  cairo_surface_t *surface;
  cairo_region_t  *region;

  /* fallback to XShape only for non-composited clients */
  if (gtk_widget_is_composited (window)) {
    gtk_widget_shape_combine_region (window, NULL);
  }
  else {

    surface = gdk_window_create_similar_surface (gtk_widget_get_window (window),
                                                 CAIRO_CONTENT_COLOR_ALPHA,
                                                 gtk_widget_get_allocated_width (window),
                                                 gtk_widget_get_allocated_height (window));

    cr = cairo_create (surface);
    paint_background_and_frame (window, cr);
    cairo_destroy (cr);

    region = gdk_cairo_region_create_from_surface (surface);
    gtk_widget_shape_combine_region (window, region);

    cairo_surface_destroy (surface);
    cairo_region_destroy (region);
  }
}

static void
eel_tooltip_composited_changed (GtkWidget *window, GtkWidget *widget)
{
  if (gtk_widget_get_realized (window)) {
    maybe_update_shape (window);
  }
}

static int
eel_tooltip_draw_window (GtkWidget *window, cairo_t *cr)
{
  if (gtk_widget_is_composited (window)) {
      /* clear any background */
      cairo_save (cr);
      cairo_set_source_rgba (cr, 0, 0, 0, 0);
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      cairo_paint (cr);
      cairo_restore (cr);
    }

  maybe_update_shape (window);
  paint_background_and_frame (window, cr);
  return FALSE;
}

static void
eel_tooltip_realize_window (GtkWidget *window)
{
  maybe_update_shape (window);
}

void
eel_tooltip_map_window (EelTooltip *tooltip)
{
  if (tooltip->tip_timeout_id > 0) {
    eel_source_remove(tooltip->tip_timeout_id);
    tooltip->tip_timeout_id = 0;
  }
  if (tooltip->tip_timeout > 0) {
    tooltip->tip_timeout_id = gdk_threads_add_timeout (tooltip->tip_timeout,
                                                       hide_tooltip,
                                                       tooltip->window);
  }
}

void
eel_tooltip_unmap_window (EelTooltip *tooltip)
{
  if (tooltip->tip_timeout_id > 0) {
    eel_source_remove(tooltip->tip_timeout_id);
    tooltip->tip_timeout_id = 0;
  }
}

static void
eel_tooltip_init (EelTooltip *tooltip)
{
  GdkScreen       *screen;
  GdkVisual       *visual;
  GtkWidget       *box;
  GtkWidget       *label;
  GtkWidget       *widget;
  GtkWindow       *window;
  GtkStyleContext *context;

  window =(GtkWindow*) gtk_window_new (GTK_WINDOW_POPUP);

  widget = GTK_WIDGET(window);

  screen = gtk_widget_get_screen (widget);
  visual = gdk_screen_get_rgba_visual (screen);

  if (visual != NULL) {
    gtk_widget_set_visual (widget, visual);
  }

  gtk_window_set_type_hint (window,
                            GDK_WINDOW_TYPE_HINT_TOOLTIP);

  gtk_widget_set_app_paintable (widget, TRUE);
  gtk_window_set_resizable (window, FALSE);

  //gtk_widget_set_name (tooltips->tip_window, "gtk-tooltip");
  gtk_widget_set_name (widget, "gtk-tooltip-window");

  context = gtk_widget_get_style_context (widget);
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_TOOLTIP);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_widget_set_margin_left (box, 5);
  gtk_widget_set_margin_right (box, 5);
  gtk_widget_set_margin_top (box, 5);
  gtk_widget_set_margin_bottom (box, 5);
  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_widget_show (box);

  label  = gtk_label_new ("");
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_container_add (GTK_CONTAINER (box), label);
  gtk_widget_show (label);

  g_signal_connect_swapped (window, "draw",
                            G_CALLBACK (eel_tooltip_draw_window), window);

  g_signal_connect_swapped (window, "realize",
                            G_CALLBACK (eel_tooltip_realize_window), window);

  g_signal_connect_swapped (window, "map",
                            G_CALLBACK (eel_tooltip_map_window), tooltip);

  g_signal_connect_swapped (window, "unmap",
                            G_CALLBACK (eel_tooltip_unmap_window), tooltip);

  g_signal_connect_swapped (window, "composited-changed",
                            G_CALLBACK (eel_tooltip_composited_changed), window);

  tooltip->label = GTK_LABEL (label);
  tooltip->window = window;
}

EelTooltip *
eel_tooltip_new (void)
{
  return g_object_new (EEL_TYPE_TOOLTIP, NULL);
}

int eel_tooltip_set_timeout (EelTooltip *tooltip, int timeout)
{
   if (timeout < 0) {
     tooltip->tip_timeout = 0;
   }
   else if (timeout > 99) {
     tooltip->tip_timeout = 9900;
   }
   else {
     tooltip->tip_timeout = timeout * 1000;
   }

   return tooltip->tip_timeout;
}
