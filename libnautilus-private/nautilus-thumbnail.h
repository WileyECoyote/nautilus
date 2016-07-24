/*
 * gnome-thumbnail.h: Utilities for handling thumbnails
 *
 * Copyright (C) 2002 Red Hat, Inc.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef NAUTILUS_THUMBNAIL_H
#define NAUTILUS_THUMBNAIL_H

/* WEH: Including headers in a header is BAD programming practice -removed */

/* WEH: Including foreign headers in header is not acceptable -removed */

G_BEGIN_DECLS

typedef enum {
  NAUTILUS_THUMBNAIL_SIZE_NORMAL,
  NAUTILUS_THUMBNAIL_SIZE_LARGE
} NautilusThumbnailSize;

#define NAUTILUS_TYPE_THUMBNAIL_FACTORY	        (nautilus_thumbnail_factory_get_type ())
#define NAUTILUS_THUMBNAIL_FACTORY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), NAUTILUS_TYPE_THUMBNAIL_FACTORY, NautilusThumbnailFactory))
#define NAUTILUS_THUMBNAIL_FACTORY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), NAUTILUS_TYPE_THUMBNAIL_FACTORY, NautilusThumbnailFactoryClass))
#define NAUTILUS_IS_THUMBNAIL_FACTORY(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), NAUTILUS_TYPE_THUMBNAIL_FACTORY))
#define NAUTILUS_IS_THUMBNAIL_FACTORY_CLASS(klass)	(G_TYPE_CLASS_CHECK_CLASS_TYPE ((klass), NAUTILUS_TYPE_THUMBNAIL_FACTORY))

typedef struct _NautilusThumbnailFactory        NautilusThumbnailFactory;
typedef struct _NautilusThumbnailFactoryClass   NautilusThumbnailFactoryClass;
typedef struct _NautilusThumbnailFactoryPrivate NautilusThumbnailFactoryPrivate;

struct _NautilusThumbnailFactory {
	GObject parent;

	NautilusThumbnailFactoryPrivate *priv;
};

struct _NautilusThumbnailFactoryClass {
	GObjectClass parent;
};

GType                   nautilus_thumbnail_factory_get_type                  (void);

NautilusThumbnailFactory *nautilus_thumbnail_factory_new                     (NautilusThumbnailSize     size);

char *                 nautilus_thumbnail_factory_lookup                     (NautilusThumbnailFactory *factory,
                                                                              const char            *uri,
                                                                              time_t                 mtime);

_Bool                  nautilus_thumbnail_factory_has_valid_failed_thumbnail (NautilusThumbnailFactory *factory,
                                                                              const char            *uri,
                                                                              time_t                 mtime);
_Bool                   nautilus_thumbnail_factory_can_thumbnail             (NautilusThumbnailFactory *factory,
                                                                              const char            *uri,
                                                                              const char            *mime_type,
                                                                              time_t                 mtime);
GdkPixbuf *             nautilus_thumbnail_factory_generate_thumbnail        (NautilusThumbnailFactory *factory,
                                                                              const char            *uri,
                                                                              const char            *mime_type);
void                    nautilus_thumbnail_factory_save_thumbnail            (NautilusThumbnailFactory *factory,
                                                                              GdkPixbuf             *thumbnail,
                                                                              const char            *uri,
                                                                              time_t                 original_mtime);
void                    nautilus_thumbnail_factory_create_failed_thumbnail   (NautilusThumbnailFactory *factory,
                                                                              const char            *uri,
                                                                              time_t                 mtime);


/* Thumbnailing utils: */
_Bool   nautilus_thumbnail_has_uri              (GdkPixbuf          *pixbuf,
                                                 const char         *uri);
_Bool   nautilus_thumbnail_is_valid             (GdkPixbuf          *pixbuf,
                                                 const char         *uri,
                                                 time_t              mtime);
char *     nautilus_thumbnail_md5               (const char         *uri);
char *     nautilus_thumbnail_path_for_uri      (const char         *uri,
                                                 NautilusThumbnailSize  size);


/* Pixbuf utils */

GdkPixbuf *nautilus_thumbnail_scale_down_pixbuf (GdkPixbuf          *pixbuf,
                                                 int                 dest_width,
                                                 int                 dest_height);

G_END_DECLS

#endif /* NAUTILUS_THUMBNAIL_H */
