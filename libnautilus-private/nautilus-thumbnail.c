/*
 * nautilus-thumbnail.c: Utilities for handling thumbnails
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

#include <config.h>

#include <math.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>
#include <time.h>

#define GDK_PIXBUF_ENABLE_BACKEND
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <eel/eel-glib-macros.h>

#include "nautilus-thumbnail.h"

#include <gconf/gconf-client.h>

#define SECONDS_BETWEEN_STATS 10

struct _NautilusThumbnailFactoryPrivate {
  char *application;
  NautilusThumbnailSize size;

  GMutex *lock;

  GHashTable *scripts_hash;
  unsigned int thumbnailers_notify;
  unsigned int reread_scheduled;
};

static void nautilus_thumbnail_factory_init          (NautilusThumbnailFactory      *factory);
static void nautilus_thumbnail_factory_class_init    (NautilusThumbnailFactoryClass *class);

G_DEFINE_TYPE (NautilusThumbnailFactory, nautilus_thumbnail_factory, G_TYPE_OBJECT)

#define parent_class nautilus_thumbnail_factory_parent_class

static void
nautilus_thumbnail_factory_finalize (GObject *object)
{
  NautilusThumbnailFactory *factory;
  NautilusThumbnailFactoryPrivate *priv;
  GConfClient *client;

  factory = NAUTILUS_THUMBNAIL_FACTORY (object);

  priv = factory->priv;

  g_free (priv->application);
  priv->application = NULL;

  EEL_SOURCE_REMOVE_IF_THEN_ZERO (priv->reread_scheduled);

  if (priv->thumbnailers_notify != 0) {
    client = gconf_client_get_default ();
    gconf_client_notify_remove (client, priv->thumbnailers_notify);
    priv->thumbnailers_notify = 0;
    g_object_unref (client);
  }

  if (priv->scripts_hash)
    {
      g_hash_table_destroy (priv->scripts_hash);
      priv->scripts_hash = NULL;
    }

  if (priv->lock)
    {
      g_mutex_free (priv->lock);
      priv->lock = NULL;
    }

  g_free (priv);
  factory->priv = NULL;

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/* Must be called on main thread */
static GHashTable *
read_scripts (void)
{
  GHashTable *scripts_hash;
  GConfClient *client;
  GSList *subdirs, *l;
  char *subdir, *enable, *escape, *commandkey, *command, *mimetype;

  client = gconf_client_get_default ();

  if (gconf_client_get_bool (client,
			     "/desktop/gnome/thumbnailers/disable_all",
			     NULL))
    {
      g_object_unref (G_OBJECT (client));
      return NULL;
    }

  scripts_hash = g_hash_table_new_full (g_str_hash,
					g_str_equal,
					g_free, g_free);


  subdirs = gconf_client_all_dirs (client, "/desktop/gnome/thumbnailers", NULL);

  for (l = subdirs; l != NULL; l = l->next)
    {
      subdir = l->data;

      enable = g_strdup_printf ("%s/enable", subdir);
      if (gconf_client_get_bool (client,
				 enable,
				 NULL))
	{
	  commandkey = g_strdup_printf ("%s/command", subdir);
	  command = gconf_client_get_string (client, commandkey, NULL);
	  g_free (commandkey);

	  if (command != NULL) {
	    mimetype = strrchr (subdir, '/');
	    if (mimetype != NULL)
	      {
		mimetype++; /* skip past slash */

		/* Convert '@' to slash in mimetype */
		escape = strchr (mimetype, '@');
		if (escape != NULL)
		  *escape = '/';

		/* Convert any remaining '@' to '+' in mimetype */
		while ((escape = strchr (mimetype, '@')) != NULL)
                  *escape = '+';

		g_hash_table_insert (scripts_hash,
				     g_strdup (mimetype), command);
	      }
	    else
	      {
		g_free (command);
	      }
	  }
	}
      g_free (enable);

      g_free (subdir);
    }

  g_slist_free(subdirs);

  g_object_unref (G_OBJECT (client));

  return scripts_hash;
}


/* Must be called on main thread */
static void
nautilus_thumbnail_factory_reread_scripts (NautilusThumbnailFactory *factory)
{
  NautilusThumbnailFactoryPrivate *priv = factory->priv;
  GHashTable *scripts_hash;

  scripts_hash = read_scripts ();

  g_mutex_lock (priv->lock);

  if (priv->scripts_hash != NULL)
    g_hash_table_destroy (priv->scripts_hash);

  priv->scripts_hash = scripts_hash;

  g_mutex_unlock (priv->lock);
}

/* Is really _Bool but glib errently defines gboolean as int */
static int
reread_idle_callback (void *user_data)
{
  NautilusThumbnailFactory *factory = user_data;
  NautilusThumbnailFactoryPrivate *priv = factory->priv;

  nautilus_thumbnail_factory_reread_scripts (factory);

  g_mutex_lock (priv->lock);
  priv->reread_scheduled = 0;
  g_mutex_unlock (priv->lock);

  return FALSE;
}

static void
schedule_reread (GConfClient  *client,
                 unsigned int  cnxn_id,
                 GConfEntry   *entry,
                 void         *user_data)
{
  NautilusThumbnailFactory *factory = user_data;
  NautilusThumbnailFactoryPrivate *priv = factory->priv;

  g_mutex_lock (priv->lock);

  if (priv->reread_scheduled == 0)
    {
      priv->reread_scheduled = g_idle_add (reread_idle_callback, factory);
    }

  g_mutex_unlock (priv->lock);
}


static void
nautilus_thumbnail_factory_init (NautilusThumbnailFactory *factory)
{

  NautilusThumbnailFactoryPrivate *priv;

  factory->priv = g_new0 (NautilusThumbnailFactoryPrivate, 1);

  priv = factory->priv;

  priv->size = NAUTILUS_THUMBNAIL_SIZE_NORMAL;
  priv->application = g_strdup ("nautilus-thumbnail-factory");

  priv->scripts_hash = NULL;

  priv->lock = (void*)g_mutex_new ();

  nautilus_thumbnail_factory_reread_scripts (factory);

#if HAVE_GNOME_DESKTOP

  GConfClient *client;

  client = gconf_client_get_default ();

  gconf_client_add_dir (client, "/desktop/gnome",
                        GCONF_CLIENT_PRELOAD_NONE, NULL);

  priv->thumbnailers_notify = gconf_client_notify_add (client, "/desktop/gnome/thumbnailers",
                                                       schedule_reread, factory, NULL,
                                                       NULL);

  g_object_unref (G_OBJECT (client));

#endif

}

static void
nautilus_thumbnail_factory_class_init (NautilusThumbnailFactoryClass *class)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = nautilus_thumbnail_factory_finalize;
}

/**
 * nautilus_thumbnail_factory_new:
 * @size: The thumbnail size to use
 *
 * Creates a new #NautilusThumbnailFactory.
 *
 * This function must be called on the main thread.
 *
 * Return value: a new #NautilusThumbnailFactory
 *
 * Since: 2.2
 **/
NautilusThumbnailFactory *
nautilus_thumbnail_factory_new (NautilusThumbnailSize size)
{
  NautilusThumbnailFactory *factory;

  factory = g_object_new (NAUTILUS_TYPE_THUMBNAIL_FACTORY, NULL);

  factory->priv->size = size;

  return factory;
}

/**
 * nautilus_thumbnail_factory_lookup:
 * @factory: a #NautilusThumbnailFactory
 * @uri: the uri of a file
 * @mtime: the mtime of the file
 *
 * Tries to locate an existing thumbnail for the file specified.
 *
 * Usage of this function is threadsafe.
 *
 * Return value: The absolute path of the thumbnail, or %NULL if none exist.
 *
 * Since: 2.2
 **/
char *
nautilus_thumbnail_factory_lookup (NautilusThumbnailFactory *factory,
				const char            *uri,
				time_t                 mtime)
{
  NautilusThumbnailFactoryPrivate *priv = factory->priv;
  char *path, *file;
  GChecksum *checksum;
  unsigned char digest[16];
  size_t digest_len = sizeof (digest);
  GdkPixbuf *pixbuf;
  _Bool res;

  g_return_val_if_fail (uri != NULL, NULL);

  res = FALSE;

  checksum = g_checksum_new (G_CHECKSUM_MD5);
  g_checksum_update (checksum, (const guchar *) uri, strlen (uri));

  g_checksum_get_digest (checksum, digest, &digest_len);
  g_assert (digest_len == 16);

  file = g_strconcat (g_checksum_get_string (checksum), ".png", NULL);

 /* WEH 04/17/14: Changed from user_home to _user_cache dir*/
  path = g_build_filename (g_get_user_cache_dir (),"thumbnails",
                           (priv->size == NAUTILUS_THUMBNAIL_SIZE_NORMAL)?"normal":"large",
                           file,
                           NULL);
  g_free (file);

  pixbuf = gdk_pixbuf_new_from_file (path, NULL);
  if (pixbuf != NULL)
    {
      res = nautilus_thumbnail_is_valid (pixbuf, uri, mtime);
      g_object_unref (pixbuf);
    }

  g_checksum_free (checksum);

  if (res)
    return path;

  g_free (path);
  return NULL;
}

/**
 * nautilus_thumbnail_factory_has_valid_failed_thumbnail:
 * @factory: a #NautilusThumbnailFactory
 * @uri: the uri of a file
 * @mtime: the mtime of the file
 *
 * Tries to locate an failed thumbnail for the file specified. Writing
 * and looking for failed thumbnails is important to avoid to try to
 * thumbnail e.g. broken images several times.
 *
 * Usage of this function is threadsafe.
 *
 * Return value: TRUE if there is a failed thumbnail for the file.
 *
 * Since: 2.2
 **/
_Bool
nautilus_thumbnail_factory_has_valid_failed_thumbnail (NautilusThumbnailFactory *factory,
						    const char            *uri,
						    time_t                 mtime)
{
  char *path, *file;
  GdkPixbuf *pixbuf;
  _Bool res;
  GChecksum *checksum;
  unsigned char digest[16];
  size_t digest_len = sizeof (digest);

  checksum = g_checksum_new (G_CHECKSUM_MD5);
  g_checksum_update (checksum, (const guchar *) uri, strlen (uri));

  g_checksum_get_digest (checksum, digest, &digest_len);
  g_assert (digest_len == 16);

  res = FALSE;

  file = g_strconcat (g_checksum_get_string (checksum), ".png", NULL);

  path = g_build_filename (g_get_user_cache_dir (),
			   "thumbnails/fail",
			   factory->priv->application,
			   file,
			   NULL);
  g_free (file);

  pixbuf = gdk_pixbuf_new_from_file (path, NULL);
  g_free (path);

  if (pixbuf)
    {
      res = nautilus_thumbnail_is_valid (pixbuf, uri, mtime);
      g_object_unref (pixbuf);
    }

  g_checksum_free (checksum);

  return res;
}

static _Bool
mimetype_supported_by_gdk_pixbuf (const char *mime_type)
{
	unsigned int i;
	static GHashTable *formats_hash = NULL;

	if (!formats_hash) {
		GSList *formats, *list;

		formats_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

		formats = gdk_pixbuf_get_formats ();
		list = formats;

		while (list) {
			GdkPixbufFormat *format = list->data;
			gchar **mime_types;

			mime_types = gdk_pixbuf_format_get_mime_types (format);

			for (i = 0; mime_types[i] != NULL; i++)
				g_hash_table_insert (formats_hash,
						     (gpointer) g_strdup (mime_types[i]),
						     GUINT_TO_POINTER (1));

			g_strfreev (mime_types);
			list = list->next;
		}
		g_slist_free (formats);
	}

	if (g_hash_table_lookup (formats_hash, mime_type))
		return TRUE;

	return FALSE;
}

/**
 * nautilus_thumbnail_factory_can_thumbnail:
 * @factory: a #NautilusThumbnailFactory
 * @uri: the uri of a file
 * @mime_type: the mime type of the file
 * @mtime: the mtime of the file
 *
 * Returns TRUE if this GnomeIconFactory can (at least try) to thumbnail
 * this file. Thumbnails or files with failed thumbnails won't be thumbnailed.
 *
 * Usage of this function is threadsafe.
 *
 * Return value: TRUE if the file can be thumbnailed.
 *
 * Since: 2.2
 **/
_Bool
nautilus_thumbnail_factory_can_thumbnail (NautilusThumbnailFactory *factory,
				       const char            *uri,
				       const char            *mime_type,
				       time_t                 mtime)
{
  /* Don't thumbnail thumbnails */
  /* Note that this causes the thumbnails directory to show up empty */
  if (uri &&
      strncmp (uri, "file:/", 6) == 0 &&
      strstr (uri, "/.thumbnails/") != NULL)
    return FALSE;

  if (mime_type != NULL &&
      (mimetype_supported_by_gdk_pixbuf (mime_type) ||
       (factory->priv->scripts_hash != NULL &&
	g_hash_table_lookup (factory->priv->scripts_hash, mime_type))))
    {
      return !nautilus_thumbnail_factory_has_valid_failed_thumbnail (factory,
								  uri,
								  mtime);
    }

  return FALSE;
}

static char *
expand_thumbnailing_script (const char *script,
			    const int   size,
			    const char *inuri,
			    const char *outfile)
{
  GString *str;
  const char *p, *last;
  char *localfile, *quoted;
  _Bool got_in;

  str = g_string_new (NULL);

  got_in = FALSE;
  last = script;
  while ((p = strchr (last, '%')) != NULL)
    {
      g_string_append_len (str, last, p - last);
      p++;

      switch (*p) {
      case 'u':
	quoted = g_shell_quote (inuri);
	g_string_append (str, quoted);
	g_free (quoted);
	got_in = TRUE;
	p++;
	break;
      case 'i':
        //localfile = gnome_vfs_get_local_path_from_uri (inuri);
	localfile = g_filename_from_uri (inuri, NULL, NULL);
	if (localfile)
	  {
	    quoted = g_shell_quote (localfile);
	    g_string_append (str, quoted);
	    got_in = TRUE;
	    g_free (quoted);
	    g_free (localfile);
	  }
	p++;
	break;
      case 'o':
	quoted = g_shell_quote (outfile);
	g_string_append (str, quoted);
	g_free (quoted);
	p++;
	break;
      case 's':
	g_string_append_printf (str, "%d", size);
	p++;
	break;
      case '%':
	g_string_append_c (str, '%');
	p++;
	break;
      case 0:
      default:
	break;
      }
      last = p;
    }
  g_string_append (str, last);

  if (got_in)
    return g_string_free (str, FALSE);

  g_string_free (str, TRUE);
  return NULL;
}


/**
 * nautilus_thumbnail_factory_generate_thumbnail:
 * @factory: a #NautilusThumbnailFactory
 * @uri: the uri of a file
 * @mime_type: the mime type of the file
 *
 * Tries to generate a thumbnail for the specified file. If it succeeds
 * it returns a pixbuf that can be used as a thumbnail.
 *
 * Usage of this function is threadsafe.
 *
 * Return value: thumbnail pixbuf if thumbnailing succeeded, %NULL otherwise.
 *
 * Since: 2.2
 **/
GdkPixbuf *
nautilus_thumbnail_factory_generate_thumbnail (NautilusThumbnailFactory *factory,
					    const char            *uri,
					    const char            *mime_type)
{
  GdkPixbuf *pixbuf, *scaled, *tmp_pixbuf;
  char *script, *expanded_script;
  int width, height, size;
  int original_width = 0;
  int original_height = 0;
  char dimension[12];
  double scale;
  int exit_status;
  char *tmpname;

  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);

  /* Doesn't access any volatile fields in factory, so it's threadsafe */

  size = 128;
  if (factory->priv->size == NAUTILUS_THUMBNAIL_SIZE_LARGE)
    size = 256;

  pixbuf = NULL;

  script = NULL;
  if (factory->priv->scripts_hash != NULL)
    script = g_hash_table_lookup (factory->priv->scripts_hash, mime_type);

  if (script)
    {
      int fd;

      fd = g_file_open_tmp (".nautilus_thumbnail.XXXXXX", &tmpname, NULL);

      if (fd != -1)
	{
	  close (fd);

	  expanded_script = expand_thumbnailing_script (script, size, uri, tmpname);
	  if (expanded_script != NULL &&
	      g_spawn_command_line_sync (expanded_script,
					 NULL, NULL, &exit_status, NULL) &&
	      exit_status == 0)
	    {
	      pixbuf = gdk_pixbuf_new_from_file (tmpname, NULL);
	    }

	  g_free (expanded_script);
	  g_unlink(tmpname);
	  g_free (tmpname);
	}
    }

  /* Fall back to gdk-pixbuf */
  if (pixbuf == NULL)
    {

      //pixbuf = gnome_gdk_pixbuf_new_from_uri_at_scale (uri, size, size, TRUE);
      char *filename = g_filename_from_uri (uri, NULL, NULL);
      pixbuf = gdk_pixbuf_new_from_file_at_scale (filename, size, size, TRUE, NULL);
      g_free(filename);

      if (pixbuf != NULL)
        {
          original_width = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (pixbuf),
                                                               "gnome-original-width"));
          original_height = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (pixbuf),
                                                                "gnome-original-height"));
        }
    }

  if (pixbuf == NULL)
    return NULL;

  /* The pixbuf loader may attach an "orientation" option to the pixbuf,
     if the tiff or exif jpeg file had an orientation tag. Rotate/flip
     the pixbuf as specified by this tag, if present. */
  tmp_pixbuf = gdk_pixbuf_apply_embedded_orientation (pixbuf);
  g_object_unref (pixbuf);
  pixbuf = tmp_pixbuf;

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  if (width > size || height > size)
    {
      const gchar *orig_width, *orig_height;
      scale = (double)size / MAX (width, height);

      scaled = gdk_pixbuf_scale_simple (pixbuf,
                                        floor (width * scale + 0.5),
                                        floor (height * scale + 0.5),
                                        GDK_INTERP_BILINEAR);
/*
      scaled = nautilus_thumbnail_scale_down_pixbuf (pixbuf,
						  floor (width * scale + 0.5),
						  floor (height * scale + 0.5));
*/
      orig_width = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Image::Width");
      orig_height = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Image::Height");

      if (orig_width != NULL) {
	      gdk_pixbuf_set_option (scaled, "tEXt::Thumb::Image::Width", orig_width);
      }
      if (orig_height != NULL) {
	      gdk_pixbuf_set_option (scaled, "tEXt::Thumb::Image::Height", orig_height);
      }

      g_object_unref (pixbuf);
      pixbuf = scaled;
    }

  if (original_width > 0) {
	  g_snprintf (dimension, sizeof (dimension), "%i", original_width);
	  gdk_pixbuf_set_option (pixbuf, "tEXt::Thumb::Image::Width", dimension);
  }
  if (original_height > 0) {
	  g_snprintf (dimension, sizeof (dimension), "%i", original_height);
	  gdk_pixbuf_set_option (pixbuf, "tEXt::Thumb::Image::Height", dimension);
  }

  return pixbuf;
}

static _Bool
make_thumbnail_dirs (NautilusThumbnailFactory *factory)
{
  char *thumbnail_dir;
  char *image_dir;
  _Bool res;

  res = FALSE;

  thumbnail_dir = g_build_filename (g_get_user_cache_dir (), "thumbnails", NULL);

  if (!g_file_test (thumbnail_dir, G_FILE_TEST_IS_DIR))
    {
      mkdir (thumbnail_dir, 0700);
      res = TRUE;
    }

  image_dir = g_build_filename (thumbnail_dir,
				(factory->priv->size == NAUTILUS_THUMBNAIL_SIZE_NORMAL)?"normal":"large",
				NULL);
  if (!g_file_test (image_dir, G_FILE_TEST_IS_DIR))
    {
      mkdir (image_dir, 0700);
      res = TRUE;
    }

  g_free (thumbnail_dir);
  g_free (image_dir);

  return res;
}

static _Bool
make_thumbnail_fail_dirs (NautilusThumbnailFactory *factory)
{
  char *thumbnail_dir;
  char *fail_dir;
  char *app_dir;
  _Bool res;

  res = FALSE;

  thumbnail_dir = g_build_filename (g_get_user_cache_dir (), "thumbnails", NULL);
  if (!g_file_test (thumbnail_dir, G_FILE_TEST_IS_DIR))
    {
      mkdir (thumbnail_dir, 0700);
      res = TRUE;
    }

  fail_dir = g_build_filename (thumbnail_dir,
			       "fail",
			       NULL);
  if (!g_file_test (fail_dir, G_FILE_TEST_IS_DIR))
    {
      mkdir (fail_dir, 0700);
      res = TRUE;
    }

  app_dir = g_build_filename (fail_dir,
			      factory->priv->application,
			      NULL);
  if (!g_file_test (app_dir, G_FILE_TEST_IS_DIR))
    {
      mkdir (app_dir, 0700);
      res = TRUE;
    }

  g_free (thumbnail_dir);
  g_free (fail_dir);
  g_free (app_dir);

  return res;
}


/**
 * nautilus_thumbnail_factory_save_thumbnail:
 * @factory: a #NautilusThumbnailFactory
 * @thumbnail: the thumbnail as a pixbuf
 * @uri: the uri of a file
 * @original_mtime: the modification time of the original file
 *
 * Saves @thumbnail at the right place. If the save fails a
 * failed thumbnail is written.
 *
 * Usage of this function is threadsafe.
 *
 * Since: 2.2
 **/
void
nautilus_thumbnail_factory_save_thumbnail (NautilusThumbnailFactory *factory,
                                           GdkPixbuf                *thumbnail,
                                           const char               *uri,
                                           time_t                    original_mtime)
{
  NautilusThumbnailFactoryPrivate *priv = factory->priv;
  char *path, *file, *dir;
  char *tmp_path;
  const char *width, *height;
  int tmp_fd;
  char mtime_str[21];
  _Bool saved_ok;
  GChecksum *checksum;
  unsigned char digest[16];
  size_t digest_len = sizeof (digest);

  checksum = g_checksum_new (G_CHECKSUM_MD5);
  g_checksum_update (checksum, (const guchar *) uri, strlen (uri));

  g_checksum_get_digest (checksum, digest, &digest_len);
  g_assert (digest_len == 16);

  file = g_strconcat (g_checksum_get_string (checksum), ".png", NULL);

  dir = g_build_filename (g_get_user_cache_dir (), "thumbnails",
			  (priv->size == NAUTILUS_THUMBNAIL_SIZE_NORMAL)?"normal":"large",
			  NULL);

  path = g_build_filename (dir,
			   file,
			   NULL);
  g_free (file);

  g_checksum_free (checksum);

  tmp_path = g_strconcat (path, ".XXXXXX", NULL);

  tmp_fd = g_mkstemp (tmp_path);
  if (tmp_fd == -1 &&
      make_thumbnail_dirs (factory))
    {
      g_free (tmp_path);
      tmp_path = g_strconcat (path, ".XXXXXX", NULL);
      tmp_fd = g_mkstemp (tmp_path);
    }

  if (tmp_fd == -1)
    {
      nautilus_thumbnail_factory_create_failed_thumbnail (factory, uri, original_mtime);
      g_free (dir);
      g_free (tmp_path);
      g_free (path);
      return;
    }
  close (tmp_fd);

  g_snprintf (mtime_str, 21, "%ld",  original_mtime);
  width = gdk_pixbuf_get_option (thumbnail, "tEXt::Thumb::Image::Width");
  height = gdk_pixbuf_get_option (thumbnail, "tEXt::Thumb::Image::Height");

  if (width != NULL && height != NULL)
    saved_ok  = gdk_pixbuf_save (thumbnail,
				 tmp_path,
				 "png", NULL,
				 "tEXt::Thumb::Image::Width", width,
				 "tEXt::Thumb::Image::Height", height,
				 "tEXt::Thumb::URI", uri,
				 "tEXt::Thumb::MTime", mtime_str,
				 "tEXt::Software", "GNOME::ThumbnailFactory",
				 NULL);
  else
    saved_ok  = gdk_pixbuf_save (thumbnail,
				 tmp_path,
				 "png", NULL,
				 "tEXt::Thumb::URI", uri,
				 "tEXt::Thumb::MTime", mtime_str,
				 "tEXt::Software", "GNOME::ThumbnailFactory",
				 NULL);


  if (saved_ok)
    {
      chmod (tmp_path, 0600);
      g_rename(tmp_path, path);
    }
  else
    {
      nautilus_thumbnail_factory_create_failed_thumbnail (factory, uri, original_mtime);
    }

  g_free (dir);
  g_free (path);
  g_free (tmp_path);
}

/**
 * nautilus_thumbnail_factory_create_failed_thumbnail:
 * @factory: a #NautilusThumbnailFactory
 * @uri: the uri of a file
 * @mtime: the modification time of the file
 *
 * Creates a failed thumbnail for the file so that we don't try
 * to re-thumbnail the file later.
 *
 * Usage of this function is threadsafe.
 *
 * Since: 2.2
 **/
void
nautilus_thumbnail_factory_create_failed_thumbnail (NautilusThumbnailFactory *factory,
						 const char            *uri,
						 time_t                 mtime)
{
  char *path, *file, *dir;
  char *tmp_path;
  int tmp_fd;
  char mtime_str[21];
  _Bool saved_ok;
  GdkPixbuf *pixbuf;
  GChecksum *checksum;
  unsigned char digest[16];
  size_t digest_len = sizeof (digest);

  checksum = g_checksum_new (G_CHECKSUM_MD5);
  g_checksum_update (checksum, (const guchar *) uri, strlen (uri));

  g_checksum_get_digest (checksum, digest, &digest_len);
  g_assert (digest_len == 16);

  file = g_strconcat (g_checksum_get_string (checksum), ".png", NULL);

  dir = g_build_filename (g_get_user_cache_dir (), "thumbnails/fail",
                          factory->priv->application,
                          NULL);

  path = g_build_filename (dir,
			   file,
			   NULL);
  g_free (file);

  g_checksum_free (checksum);

  tmp_path = g_strconcat (path, ".XXXXXX", NULL);

  tmp_fd = g_mkstemp (tmp_path);
  if (tmp_fd == -1 &&
      make_thumbnail_fail_dirs (factory))
    {
      g_free (tmp_path);
      tmp_path = g_strconcat (path, ".XXXXXX", NULL);
      tmp_fd = g_mkstemp (tmp_path);
    }

  if (tmp_fd == -1)
    {
      g_free (dir);
      g_free (tmp_path);
      g_free (path);
      return;
    }
  close (tmp_fd);

  g_snprintf (mtime_str, 21, "%ld",  mtime);
  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);
  saved_ok  = gdk_pixbuf_save (pixbuf,
			       tmp_path,
			       "png", NULL,
			       "tEXt::Thumb::URI", uri,
			       "tEXt::Thumb::MTime", mtime_str,
			       "tEXt::Software", "GNOME::ThumbnailFactory",
			       NULL);
  g_object_unref (pixbuf);
  if (saved_ok)
    {
      chmod (tmp_path, 0600);
      g_rename(tmp_path, path);
    }

  g_free (dir);
  g_free (path);
  g_free (tmp_path);
}

/**
 * nautilus_thumbnail_md5:
 * @uri: an uri
 *
 * Calculates the MD5 checksum of the uri. This can be useful
 * if you want to manually handle thumbnail files.
 *
 * Return value: A string with the MD5 digest of the uri string.
 *
 * Since: 2.2
 *
 * @Deprecated: 2.22: Use #GChecksum instead
 **/
char *
nautilus_thumbnail_md5 (const char *uri)
{
  return g_compute_checksum_for_data (G_CHECKSUM_MD5,
                                      (const guchar *) uri,
                                      strlen (uri));
}

/**
 * nautilus_thumbnail_path_for_uri:
 * @uri: an uri
 * @size: a thumbnail size
 *
 * Returns the filename that a thumbnail of size @size for @uri would have.
 *
 * Return value: an absolute filename
 *
 * Since: 2.2
 **/
char *
nautilus_thumbnail_path_for_uri (const char         *uri,
			      NautilusThumbnailSize  size)
{
  char *md5;
  char *file;
  char *path;

  md5 = nautilus_thumbnail_md5 (uri);
  file = g_strconcat (md5, ".png", NULL);
  g_free (md5);

  path = g_build_filename (g_get_user_cache_dir (), "thumbnails",
			   (size == NAUTILUS_THUMBNAIL_SIZE_NORMAL)?"normal":"large",
			   file,
			   NULL);

  g_free (file);

  return path;
}

/**
 * nautilus_thumbnail_has_uri:
 * @pixbuf: an loaded thumbnail pixbuf
 * @uri: a uri
 *
 * Returns whether the thumbnail has the correct uri embedded in the
 * Thumb::URI option in the png.
 *
 * Return value: TRUE if the thumbnail is for @uri
 *
 * Since: 2.2
 **/
_Bool
nautilus_thumbnail_has_uri (GdkPixbuf          *pixbuf,
			 const char         *uri)
{
  const char *thumb_uri;

  thumb_uri = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::URI");
  if (!thumb_uri)
    return FALSE;

  return strcmp (uri, thumb_uri) == 0;
}

/**
 * nautilus_thumbnail_is_valid:
 * @pixbuf: an loaded thumbnail #GdkPixbuf
 * @uri: a uri
 * @mtime: the mtime
 *
 * Returns whether the thumbnail has the correct uri and mtime embedded in the
 * png options.
 *
 * Return value: TRUE if the thumbnail has the right @uri and @mtime
 *
 * Since: 2.2
 **/
_Bool
nautilus_thumbnail_is_valid (GdkPixbuf          *pixbuf,
			  const char         *uri,
			  time_t              mtime)
{
  const char *thumb_uri, *thumb_mtime_str;
  time_t thumb_mtime;

  thumb_uri = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::URI");
  if (!thumb_uri)
    return FALSE;
  if (strcmp (uri, thumb_uri) != 0)
    return FALSE;

  thumb_mtime_str = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::MTime");
  if (!thumb_mtime_str)
    return FALSE;
  thumb_mtime = atol (thumb_mtime_str);
  if (mtime != thumb_mtime)
    return FALSE;

  return TRUE;
}
