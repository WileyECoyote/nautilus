/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*!
 * \file eel-tooltips.h: Make GTK tooltips better.
 *
 * Copyright (C) 2014 Wiley Edward Hill wileyhill@gmail.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __EEL_TOOLTIP_H__
#define __EEL_TOOLTIP_H__

#include <gtk/gtk.h>

#define EEL_TYPE_TOOLTIP                  (eel_tooltip_get_type ())
#define EEL_TOOLTIP(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EEL_TYPE_TOOLTIP, EelTooltip))
#define EEL_TOOLTIP_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), EEL_TYPE_TOOLTIP, EelTooltipClass))
#define EEL_IS_TOOLTIP(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EEL_TYPE_TOOLTIP))
#define EEL_IS_TOOLTIP_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), EEL_TYPE_TOOLTIP))
#define EEL_TOOLTIP_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), EEL_TYPE_TOOLTIP, EelTooltipClass))


typedef struct _EelTooltip	 EelTooltip;
typedef struct _EelTooltipClass EelTooltipClass;

struct _EelTooltip
{
  GObject parent;

  GtkWindow     *window;
  GtkLabel      *label;

  int           tip_timeout;
  unsigned long tip_timeout_id;

};

struct _EelTooltipClass
{
  GtkWidgetClass parent_class;

};

EelTooltip *eel_tooltip_new (void);

int         eel_tooltip_set_timeout (EelTooltip *tooltip, int timeout);

#endif /* __EEL_TOOLTIP_H__ */

