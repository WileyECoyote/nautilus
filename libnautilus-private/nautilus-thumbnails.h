/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   nautilus-thumbnails.h: Thumbnail code for icon factory.

   Copyright (C) 2000 Eazel, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Andy Hertzfeld <andy@eazel.com>
*/

#ifndef NAUTILUS_THUMBNAILS_H
#define NAUTILUS_THUMBNAILS_H

typedef struct NautilusThumbnailAsyncLoadHandle NautilusThumbnailAsyncLoadHandle;

typedef void (* NautilusThumbnailAsyncLoadFunc) (NautilusThumbnailAsyncLoadHandle *handle,
						 const char *path,
						 GdkPixbuf  *pixbuf,
						 double      scale_x,
						 double      scale_y,
						 gpointer    user_data);


/* Returns NULL if there's no thumbnail yet. */
void       nautilus_create_thumbnail                (NautilusFile *file);
_Bool   nautilus_can_thumbnail                   (NautilusFile *file);
_Bool   nautilus_can_thumbnail_internally        (NautilusFile *file);
_Bool   nautilus_thumbnail_is_mimetype_limited_by_size
						    (const char *mime_type);
void       nautilus_thumbnail_frame_image           (GdkPixbuf **pixbuf);
GdkPixbuf *nautilus_thumbnail_unframe_image         (GdkPixbuf  *pixbuf);
GdkPixbuf *nautilus_thumbnail_load_image            (const char *path,
						     guint       base_size,
						     guint       nominal_size,
						     _Bool    force_nominal,
						     double     *scale_x_out,
						     double     *scale_y_out);
NautilusThumbnailAsyncLoadHandle *
	   nautilus_thumbnail_load_image_async	    (const char *path,
						     guint       base_size,
						     guint       nominal_size,
						     _Bool    force_nominal,
						     NautilusThumbnailAsyncLoadFunc load_func,
						     gpointer    load_func_user_data);
void       nautilus_thumbnail_load_image_cancel     (NautilusThumbnailAsyncLoadHandle *handle);


/* Queue handling: */
void       nautilus_thumbnail_remove_from_queue     (const char   *file_uri);
void       nautilus_thumbnail_remove_all_from_queue (void);
void       nautilus_thumbnail_prioritize            (const char   *file_uri);


#endif /* NAUTILUS_THUMBNAILS_H */
