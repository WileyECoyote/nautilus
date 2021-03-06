/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-file-undo-operations.c - Manages undo/redo of file operations
 *
 * Copyright (C) 2007-2011 Amos Brocco
 * Copyright (C) 2010, 2012 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Amos Brocco <amos.brocco@gmail.com>
 *          Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#include <config.h>

#include <glib.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#include <glib/gi18n.h>

#include <eel/eel-debug.h>
#include <eel/eel-icons.h>

#include <nautilus-file-attributes.h>
#include <nautilus-icon-info.h>
#include "nautilus-file.h"

#include "nautilus-file-undo-manager.h"

#include "nautilus-file-operations.h"
#include "nautilus-file-undo-operations.h"

G_DEFINE_TYPE (NautilusFileUndoInfo, nautilus_file_undo_info, G_TYPE_OBJECT)

enum {
	PROP_OP_TYPE = 1,
	PROP_ITEM_COUNT,
	N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

struct _NautilusFileUndoInfoDetails {
	NautilusFileUndoOp op_type;
	guint count;		/* Number of items */

	GSimpleAsyncResult *apply_async_result;

	char *undo_label;
	char *redo_label;
	char *undo_description;
	char *redo_description;
};

/* description helpers */
static void
nautilus_file_undo_info_init (NautilusFileUndoInfo *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, NAUTILUS_TYPE_FILE_UNDO_INFO,
						  NautilusFileUndoInfoDetails);
	self->priv->apply_async_result = NULL;
}

static void
nautilus_file_undo_info_get_property (GObject *object,
				      unsigned int property_id,
				      GValue *value,
				      GParamSpec *pspec)
{
	NautilusFileUndoInfo *self = NAUTILUS_FILE_UNDO_INFO (object);

	switch (property_id) {
	case PROP_OP_TYPE:
		g_value_set_int (value, self->priv->op_type);
		break;
	case PROP_ITEM_COUNT:
		g_value_set_int (value, self->priv->count);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
nautilus_file_undo_info_set_property (GObject *object,
				      unsigned int property_id,
				      const GValue *value,
				      GParamSpec *pspec)
{
	NautilusFileUndoInfo *self = NAUTILUS_FILE_UNDO_INFO (object);

	switch (property_id) {
	case PROP_OP_TYPE:
		self->priv->op_type = g_value_get_int (value);
		break;
	case PROP_ITEM_COUNT:
		self->priv->count = g_value_get_int (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
nautilus_file_redo_info_warn_redo (NautilusFileUndoInfo *self,
				   GtkWindow *parent_window)
{
	g_critical ("Object %p of type %s does not implement redo_func!!",
		    self, G_OBJECT_TYPE_NAME (self));
}

static void
nautilus_file_undo_info_warn_undo (NautilusFileUndoInfo *self,
				   GtkWindow *parent_window)
{
	g_critical ("Object %p of type %s does not implement undo_func!!",
		    self, G_OBJECT_TYPE_NAME (self));
}

static void
nautilus_file_undo_info_strings_func (NautilusFileUndoInfo *self,
				      char **undo_label,
				      char **undo_description,
				      char **redo_label,
				      char **redo_description)
{
	if (undo_label != NULL) {
		*undo_label = g_strdup (_("Undo"));
	}
	if (undo_description != NULL) {
		*undo_description = g_strdup (_("Undo last action"));
	}

	if (redo_label != NULL) {
		*redo_label = g_strdup (_("Redo"));
	}
	if (redo_description != NULL) {
		*redo_description = g_strdup (_("Redo last undone action"));
	}
}

static void
nautilus_file_undo_info_finalize (GObject *obj)
{
	NautilusFileUndoInfo *self = NAUTILUS_FILE_UNDO_INFO (obj);

	g_clear_object (&self->priv->apply_async_result);

	G_OBJECT_CLASS (nautilus_file_undo_info_parent_class)->finalize (obj);
}

static void
nautilus_file_undo_info_class_init (NautilusFileUndoInfoClass *klass)
{
	GObjectClass *oclass = G_OBJECT_CLASS (klass);

	oclass->finalize = nautilus_file_undo_info_finalize;
	oclass->get_property = nautilus_file_undo_info_get_property;
	oclass->set_property = nautilus_file_undo_info_set_property;

	klass->undo_func = nautilus_file_undo_info_warn_undo;
	klass->redo_func = nautilus_file_redo_info_warn_redo;
	klass->strings_func = nautilus_file_undo_info_strings_func;

	properties[PROP_OP_TYPE] =
		g_param_spec_int ("op-type",
				  "Undo info op type",
				  "Type of undo operation",
				  0, NAUTILUS_FILE_UNDO_OP_NUM_TYPES - 1, 0,
				  G_PARAM_READWRITE |
				  G_PARAM_CONSTRUCT_ONLY);
	properties[PROP_ITEM_COUNT] =
		g_param_spec_int ("item-count",
				  "Number of items",
				  "Number of items",
				  0, G_MAXINT, 0,
				  G_PARAM_READWRITE |
				  G_PARAM_CONSTRUCT_ONLY);

	g_type_class_add_private (klass, sizeof (NautilusFileUndoInfoDetails));
	g_object_class_install_properties (oclass, N_PROPERTIES, properties);
}

static NautilusFileUndoOp
nautilus_file_undo_info_get_op_type (NautilusFileUndoInfo *self)
{
	return self->priv->op_type;
}

static unsigned int
nautilus_file_undo_info_get_item_count (NautilusFileUndoInfo *self)
{
	return self->priv->count;
}

void
nautilus_file_undo_info_apply_async (NautilusFileUndoInfo *self,
				     _Bool undo,
				     GtkWindow *parent_window,
				     GAsyncReadyCallback callback,
				     void * user_data)
{
	g_assert (self->priv->apply_async_result == NULL);

	self->priv->apply_async_result =
		g_simple_async_result_new (G_OBJECT (self),
					   callback, user_data,
					   nautilus_file_undo_info_apply_async);

	if (undo) {
		NAUTILUS_FILE_UNDO_INFO_CLASS (G_OBJECT_GET_CLASS (self))->undo_func (self, parent_window);
	} else {
		NAUTILUS_FILE_UNDO_INFO_CLASS (G_OBJECT_GET_CLASS (self))->redo_func (self, parent_window);
	}
}

typedef struct {
	_Bool success;
	_Bool user_cancel;
} FileUndoInfoOpRes;

static void
file_undo_info_op_res_free (void * data)
{
	g_slice_free (FileUndoInfoOpRes, data);
}

_Bool
nautilus_file_undo_info_apply_finish (NautilusFileUndoInfo *self,
				      GAsyncResult *res,
				      _Bool *user_cancel,
				      GError **error)
{
	FileUndoInfoOpRes *op_res;

	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error)) {
		return FALSE;
	}

	op_res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT (res));
	*user_cancel = op_res->user_cancel;

	return op_res->success;
}

void
nautilus_file_undo_info_get_strings (NautilusFileUndoInfo *self,
				     char **undo_label,
				     char **undo_description,
				     char **redo_label,
				     char **redo_description)
{
	return NAUTILUS_FILE_UNDO_INFO_CLASS (G_OBJECT_GET_CLASS (self))->strings_func (self,
											undo_label, undo_description,
											redo_label, redo_description);
}

static void
file_undo_info_complete_apply (NautilusFileUndoInfo *self,
			       _Bool success,
			       _Bool user_cancel)
{
	FileUndoInfoOpRes *op_res = g_slice_new0 (FileUndoInfoOpRes);

	op_res->user_cancel = user_cancel;
	op_res->success = success;


	g_simple_async_result_set_op_res_gpointer(self->priv->apply_async_result, op_res,
						   file_undo_info_op_res_free);
	g_simple_async_result_complete_in_idle (self->priv->apply_async_result);

	g_clear_object (&self->priv->apply_async_result);
}

static void
file_undo_info_transfer_callback (GHashTable *debuting_uris,
                                  _Bool       success,
                                  void       *user_data)
{
	NautilusFileUndoInfo *self = user_data;

	/* TODO: we need to forward the cancelled state from
	 * the file operation to the file undo info object.
	 */
	file_undo_info_complete_apply (self, success, FALSE);
}

static void
file_undo_info_operation_callback (NautilusFile *file,
                                   GFile       *result_location,
                                   GError      *error,
                                   void        *user_data)
{
	NautilusFileUndoInfo *self = user_data;

	file_undo_info_complete_apply (self, (error == NULL),
				       g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED));
}

static void
file_undo_info_delete_callback (GHashTable *debuting_uris, _Bool user_cancel, void *user_data)
{
	NautilusFileUndoInfo *self = user_data;

	file_undo_info_complete_apply (self, !user_cancel, user_cancel);
}

/* copy/move/duplicate/link/restore from trash */
G_DEFINE_TYPE (NautilusFileUndoInfoExt, nautilus_file_undo_info_ext, NAUTILUS_TYPE_FILE_UNDO_INFO)

struct _NautilusFileUndoInfoExtDetails {
	GFile *src_dir;
	GFile *dest_dir;
	GList *sources;	     /* Relative to src_dir */
	GList *destinations; /* Relative to dest_dir */
};

static char *
ext_get_first_target_short_name (NautilusFileUndoInfoExt *self)
{
	GList *targets_first;
	char *file_name = NULL;

	targets_first = g_list_first (self->priv->destinations);

	if (targets_first != NULL &&
	    targets_first->data != NULL) {
		file_name = g_file_get_basename (targets_first->data);
	}

	return file_name;
}

static void
ext_strings_func (NautilusFileUndoInfo *info,
		  char **undo_label,
		  char **undo_description,
		  char **redo_label,
		  char **redo_description)
{
	NautilusFileUndoInfoExt *self = NAUTILUS_FILE_UNDO_INFO_EXT (info);
	NautilusFileUndoOp op_type = nautilus_file_undo_info_get_op_type (info);
	unsigned int count = nautilus_file_undo_info_get_item_count (info);
	char *name = NULL, *source, *destination;

	source = g_file_get_path (self->priv->src_dir);
	destination = g_file_get_path (self->priv->dest_dir);

	if (count <= 1) {
		name = ext_get_first_target_short_name (self);
	}

	if (op_type == NAUTILUS_FILE_UNDO_OP_MOVE) {
		if (count > 1) {
			*undo_description = g_strdup_printf (ngettext ("Move %d item back to '%s'",
								       "Move %d items back to '%s'", count),
							     count, source);
			*redo_description = g_strdup_printf (ngettext ("Move %d item to '%s'",
								       "Move %d items to '%s'", count),
							     count, destination);

			*undo_label = g_strdup_printf (ngettext ("_Undo Move %d item",
								 "_Undo Move %d items", count),
						       count);
			*redo_label = g_strdup_printf (ngettext ("_Redo Move %d item",
								 "_Redo Move %d items", count),
						       count);
		} else {
			*undo_description = g_strdup_printf (_("Move '%s' back to '%s'"), name, source);
			*redo_description = g_strdup_printf (_("Move '%s' to '%s'"), name, destination);

			*undo_label = g_strdup (_("_Undo Move"));
			*redo_label = g_strdup (_("_Redo Move"));
		}
	}
	else if (op_type == NAUTILUS_FILE_UNDO_OP_RESTORE_FROM_TRASH)  {
		*undo_label = g_strdup (_("_Undo Restore from Trash"));
		*redo_label = g_strdup (_("_Redo Restore from Trash"));

		if (count > 1) {
			*undo_description = g_strdup_printf (ngettext ("Move %d item back to trash",
								       "Move %d items back to trash", count),
							     count);
			*redo_description = g_strdup_printf (ngettext ("Restore %d item from trash",
								       "Restore %d items from trash", count),
							     count);
		}
		else {
			*undo_description = g_strdup_printf (_("Move '%s' back to trash"), name);
			*redo_description = g_strdup_printf (_("Restore '%s' from trash"), name);
		}
	}
	else if (op_type == NAUTILUS_FILE_UNDO_OP_COPY) {
		if (count > 1) {
			*undo_description = g_strdup_printf (ngettext ("Delete %d copied item",
								       "Delete %d copied items", count),
							     count);
			*redo_description = g_strdup_printf (ngettext ("Copy %d item to '%s'",
								       "Copy %d items to '%s'", count),
							     count, destination);

			*undo_label = g_strdup_printf (ngettext ("_Undo Copy %d item",
								 "_Undo Copy %d items", count),
						       count);
			*redo_label = g_strdup_printf (ngettext ("_Redo Copy %d item",
								 "_Redo Copy %d items", count),
						       count);
		} else {
			*undo_description = g_strdup_printf (_("Delete '%s'"), name);
			*redo_description = g_strdup_printf (_("Copy '%s' to '%s'"), name, destination);

			*undo_label = g_strdup (_("_Undo Copy"));
			*redo_label = g_strdup (_("_Redo Copy"));
		}
	}
	else if (op_type == NAUTILUS_FILE_UNDO_OP_DUPLICATE) {
		if (count > 1) {
			*undo_description = g_strdup_printf (ngettext ("Delete %d duplicated item",
								       "Delete %d duplicated items", count),
							     count);
			*redo_description = g_strdup_printf (ngettext ("Duplicate %d item in '%s'",
								       "Duplicate %d items in '%s'", count),
							     count, destination);

			*undo_label = g_strdup_printf (ngettext ("_Undo Duplicate %d item",
								 "_Undo Duplicate %d items", count),
						       count);
			*redo_label = g_strdup_printf (ngettext ("_Redo Duplicate %d item",
								 "_Redo Duplicate %d items", count),
						       count);
		}
		else {
			*undo_description = g_strdup_printf (_("Delete '%s'"), name);
			*redo_description = g_strdup_printf (_("Duplicate '%s' in '%s'"),
							   name, destination);

			*undo_label = g_strdup (_("_Undo Duplicate"));
			*redo_label = g_strdup (_("_Redo Duplicate"));
		}
	}
	else if (op_type == NAUTILUS_FILE_UNDO_OP_CREATE_LINK) {
		if (count > 1) {
			*undo_description = g_strdup_printf (ngettext ("Delete links to %d item",
								       "Delete links to %d items", count),
							     count);
			*redo_description = g_strdup_printf (ngettext ("Create links to %d item",
								       "Create links to %d items", count),
							     count);
		}
		else {
			*undo_description = g_strdup_printf (_("Delete link to '%s'"), name);
			*redo_description = g_strdup_printf (_("Create link to '%s'"), name);

			*undo_label = g_strdup (_("_Undo Create Link"));
			*redo_label = g_strdup (_("_Redo Create Link"));
		}
	}
	else {
		BUG_MSG("Invalid condition");
	}

	g_free (name);
	g_free (source);
	g_free (destination);
}

static void
ext_create_link_redo_func (NautilusFileUndoInfoExt *self,
                           GtkWindow               *parent_window)
{
	nautilus_file_operations_link (self->priv->sources, NULL,
				       self->priv->dest_dir, parent_window,
				       file_undo_info_transfer_callback, self);
}

static void
ext_duplicate_redo_func (NautilusFileUndoInfoExt *self,
			 GtkWindow *parent_window)
{
	nautilus_file_operations_duplicate (self->priv->sources, NULL, parent_window,
					    file_undo_info_transfer_callback, self);
}

static void
ext_copy_redo_func (NautilusFileUndoInfoExt *self,
		    GtkWindow *parent_window)
{
	nautilus_file_operations_copy (self->priv->sources, NULL,
				       self->priv->dest_dir, parent_window,
				       file_undo_info_transfer_callback, self);
}

static void
ext_move_restore_redo_func (NautilusFileUndoInfoExt *self,
			    GtkWindow *parent_window)
{
	nautilus_file_operations_move (self->priv->sources, NULL,
				       self->priv->dest_dir, parent_window,
				       file_undo_info_transfer_callback, self);
}

static void
ext_redo_func (NautilusFileUndoInfo *info,
	       GtkWindow *parent_window)
{
	NautilusFileUndoInfoExt *self = NAUTILUS_FILE_UNDO_INFO_EXT (info);
	NautilusFileUndoOp op_type = nautilus_file_undo_info_get_op_type (info);

	if (op_type == NAUTILUS_FILE_UNDO_OP_MOVE ||
	    op_type == NAUTILUS_FILE_UNDO_OP_RESTORE_FROM_TRASH)  {
		ext_move_restore_redo_func (self, parent_window);
	}
	else if (op_type == NAUTILUS_FILE_UNDO_OP_COPY) {
		ext_copy_redo_func (self, parent_window);
	}
	else if (op_type == NAUTILUS_FILE_UNDO_OP_DUPLICATE) {
		ext_duplicate_redo_func (self, parent_window);
	}
	else if (op_type == NAUTILUS_FILE_UNDO_OP_CREATE_LINK) {
		ext_create_link_redo_func (self, parent_window);
	}
	else {
		BUG_MSG("Invalid condition");
	}
}

static void
ext_restore_undo_func (NautilusFileUndoInfoExt *self, GtkWindow *parent_window)
{
	nautilus_file_operations_trash_or_delete (self->priv->destinations, parent_window,
						  file_undo_info_delete_callback, self);
}


static void
ext_move_undo_func (NautilusFileUndoInfoExt *self,
		    GtkWindow *parent_window)
{
	nautilus_file_operations_move (self->priv->destinations, NULL,
				       self->priv->src_dir, parent_window,
				       file_undo_info_transfer_callback, self);
}

static void
ext_copy_duplicate_undo_func (NautilusFileUndoInfoExt *self,
			      GtkWindow *parent_window)
{
	GList *files;

	files = g_list_copy (self->priv->destinations);
	files = g_list_reverse (files); /* Deleting must be done in reverse */

	nautilus_file_operations_delete (files, parent_window,
					 file_undo_info_delete_callback, self);

	g_list_free (files);
}

static void
ext_undo_func (NautilusFileUndoInfo *info,
	       GtkWindow *parent_window)
{
	NautilusFileUndoInfoExt *self = NAUTILUS_FILE_UNDO_INFO_EXT (info);
	NautilusFileUndoOp op_type = nautilus_file_undo_info_get_op_type (info);

	if (op_type == NAUTILUS_FILE_UNDO_OP_COPY ||
	    op_type == NAUTILUS_FILE_UNDO_OP_DUPLICATE ||
	    op_type == NAUTILUS_FILE_UNDO_OP_CREATE_LINK) {
		ext_copy_duplicate_undo_func (self, parent_window);
	}
	else if (op_type == NAUTILUS_FILE_UNDO_OP_MOVE) {
		ext_move_undo_func (self, parent_window);
	}
	else if (op_type == NAUTILUS_FILE_UNDO_OP_RESTORE_FROM_TRASH) {
		ext_restore_undo_func (self, parent_window);
	}
	else {
		BUG_MSG("Invalid condition");
	}
}

static void
nautilus_file_undo_info_ext_init (NautilusFileUndoInfoExt *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, nautilus_file_undo_info_ext_get_type (),
						  NautilusFileUndoInfoExtDetails);
}

static void
nautilus_file_undo_info_ext_finalize (GObject *obj)
{
	NautilusFileUndoInfoExt *self = NAUTILUS_FILE_UNDO_INFO_EXT (obj);

	if (self->priv->sources) {
		g_list_free_full (self->priv->sources, g_object_unref);
	}

	if (self->priv->destinations) {
		g_list_free_full (self->priv->destinations, g_object_unref);
	}

	g_clear_object (&self->priv->src_dir);
	g_clear_object (&self->priv->dest_dir);

	G_OBJECT_CLASS (nautilus_file_undo_info_ext_parent_class)->finalize (obj);
}

static void
nautilus_file_undo_info_ext_class_init (NautilusFileUndoInfoExtClass *klass)
{
	GObjectClass *oclass = G_OBJECT_CLASS (klass);
	NautilusFileUndoInfoClass *iclass = NAUTILUS_FILE_UNDO_INFO_CLASS (klass);

	oclass->finalize = nautilus_file_undo_info_ext_finalize;

	iclass->undo_func = ext_undo_func;
	iclass->redo_func = ext_redo_func;
	iclass->strings_func = ext_strings_func;

	g_type_class_add_private (klass, sizeof (NautilusFileUndoInfoExtDetails));
}

NautilusFileUndoInfo *
nautilus_file_undo_info_ext_new (NautilusFileUndoOp op_type,
                                 int item_count,
                                 GFile *src_dir,
                                 GFile *target_dir)
{
	NautilusFileUndoInfoExt *retval;

	retval = g_object_new (NAUTILUS_TYPE_FILE_UNDO_INFO_EXT,
			       "op-type", op_type,
			       "item-count", item_count,
			       NULL);

	retval->priv->src_dir = g_object_ref (src_dir);
	retval->priv->dest_dir = g_object_ref (target_dir);

	return NAUTILUS_FILE_UNDO_INFO (retval);
}

void
nautilus_file_undo_info_ext_add_origin_target_pair (NautilusFileUndoInfoExt *self,
						    GFile                   *origin,
						    GFile                   *target)
{
	self->priv->sources =
		g_list_append (self->priv->sources, g_object_ref (origin));
	self->priv->destinations =
		g_list_append (self->priv->destinations, g_object_ref (target));
}

/* create new file/folder */
G_DEFINE_TYPE (NautilusFileUndoInfoCreate, nautilus_file_undo_info_create, NAUTILUS_TYPE_FILE_UNDO_INFO)

struct _NautilusFileUndoInfoCreateDetails {
	char *template;
	GFile *target_file;
	unsigned int length;
};

static void
create_strings_func (NautilusFileUndoInfo *info,
		     char **undo_label,
		     char **undo_description,
		     char **redo_label,
		     char **redo_description)

{
	NautilusFileUndoInfoCreate *self = NAUTILUS_FILE_UNDO_INFO_CREATE (info);
	NautilusFileUndoOp op_type = nautilus_file_undo_info_get_op_type (info);
	char *name;

	name = g_file_get_parse_name (self->priv->target_file);
	*undo_description = g_strdup_printf (_("Delete '%s'"), name);

	if (op_type == NAUTILUS_FILE_UNDO_OP_CREATE_EMPTY_FILE) {
		*redo_description = g_strdup_printf (_("Create an empty file '%s'"), name);

		*undo_label = g_strdup (_("_Undo Create Empty File"));
		*redo_label = g_strdup (_("_Redo Create Empty File"));
	}
	else if (op_type == NAUTILUS_FILE_UNDO_OP_CREATE_FOLDER) {
		*redo_description = g_strdup_printf (_("Create a new folder '%s'"), name);

		*undo_label = g_strdup (_("_Undo Create Folder"));
		*redo_label = g_strdup (_("_Redo Create Folder"));
	}
	else if (op_type == NAUTILUS_FILE_UNDO_OP_CREATE_FILE_FROM_TEMPLATE) {
		*redo_description = g_strdup_printf (_("Create new file '%s' from template "), name);

		*undo_label = g_strdup (_("_Undo Create from Template"));
		*redo_label = g_strdup (_("_Redo Create from Template"));
	}
	else {
		BUG_MSG("Invalid condition");
	}

	g_free (name);
}

static void
create_callback (GFile * new_file, _Bool success, void *callback_data)
{
	file_undo_info_transfer_callback (NULL, success, callback_data);
}

static void
create_from_template_redo_func (NautilusFileUndoInfoCreate *self,
				GtkWindow *parent_window)
{
	GFile *parent;
	char *parent_uri, *new_name;

	parent = g_file_get_parent (self->priv->target_file);
	parent_uri = g_file_get_uri (parent);
	new_name = g_file_get_parse_name (self->priv->target_file);
	nautilus_file_operations_new_file_from_template (NULL, NULL,
							 parent_uri, new_name,
							 self->priv->template,
							 create_callback, self);

	g_free (parent_uri);
	g_free (new_name);
	g_object_unref (parent);
}

static void
create_folder_redo_func (NautilusFileUndoInfoCreate *self,
			 GtkWindow *parent_window)
{
	GFile *parent;
	char *parent_uri;

	parent = g_file_get_parent (self->priv->target_file);
	parent_uri = g_file_get_uri (parent);
	nautilus_file_operations_new_folder (NULL, NULL, parent_uri,
					     create_callback, self);

	g_free (parent_uri);
	g_object_unref (parent);
}

static void
create_empty_redo_func (NautilusFileUndoInfoCreate *self,
			GtkWindow *parent_window)

{
	GFile *parent;
	char *parent_uri;
	char *new_name;

	parent = g_file_get_parent (self->priv->target_file);
	parent_uri = g_file_get_uri (parent);
	new_name = g_file_get_parse_name (self->priv->target_file);
	nautilus_file_operations_new_file (NULL, NULL, parent_uri,
					   new_name,
					   self->priv->template,
					   self->priv->length,
					   create_callback, self);

	g_free (parent_uri);
	g_free (new_name);
	g_object_unref (parent);
}

static void
create_redo_func (NautilusFileUndoInfo *info,
		  GtkWindow *parent_window)
{
	NautilusFileUndoInfoCreate *self = NAUTILUS_FILE_UNDO_INFO_CREATE (info);
	NautilusFileUndoOp op_type = nautilus_file_undo_info_get_op_type (info);

	if (op_type == NAUTILUS_FILE_UNDO_OP_CREATE_EMPTY_FILE) {
		create_empty_redo_func (self, parent_window);
	}
	else if (op_type == NAUTILUS_FILE_UNDO_OP_CREATE_FOLDER) {
		create_folder_redo_func (self, parent_window);
	}
	else if (op_type == NAUTILUS_FILE_UNDO_OP_CREATE_FILE_FROM_TEMPLATE) {
		create_from_template_redo_func (self, parent_window);
	}
	else {
		BUG_MSG("Invalid condition");
	}
}

static void
create_undo_func (NautilusFileUndoInfo *info,
		  GtkWindow *parent_window)
{
	NautilusFileUndoInfoCreate *self = NAUTILUS_FILE_UNDO_INFO_CREATE (info);
	GList *files = NULL;

	files = g_list_append (files, g_object_ref (self->priv->target_file));
	nautilus_file_operations_delete (files, parent_window,
					 file_undo_info_delete_callback, self);

	g_list_free_full (files, g_object_unref);
}

static void
nautilus_file_undo_info_create_init (NautilusFileUndoInfoCreate *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, nautilus_file_undo_info_create_get_type (),
						  NautilusFileUndoInfoCreateDetails);
}

static void
nautilus_file_undo_info_create_finalize (GObject *obj)
{
	NautilusFileUndoInfoCreate *self = NAUTILUS_FILE_UNDO_INFO_CREATE (obj);
	g_clear_object (&self->priv->target_file);
	g_free (self->priv->template);

	G_OBJECT_CLASS (nautilus_file_undo_info_create_parent_class)->finalize (obj);
}

static void
nautilus_file_undo_info_create_class_init (NautilusFileUndoInfoCreateClass *klass)
{
	GObjectClass *oclass = G_OBJECT_CLASS (klass);
	NautilusFileUndoInfoClass *iclass = NAUTILUS_FILE_UNDO_INFO_CLASS (klass);

	oclass->finalize = nautilus_file_undo_info_create_finalize;

	iclass->undo_func = create_undo_func;
	iclass->redo_func = create_redo_func;
	iclass->strings_func = create_strings_func;

	g_type_class_add_private (klass, sizeof (NautilusFileUndoInfoCreateDetails));
}

NautilusFileUndoInfo *
nautilus_file_undo_info_create_new (NautilusFileUndoOp op_type)
{
	return g_object_new (NAUTILUS_TYPE_FILE_UNDO_INFO_CREATE,
			     "op-type", op_type,
			     "item-count", 1,
			     NULL);
}

void
nautilus_file_undo_info_create_set_data (NautilusFileUndoInfoCreate *self,
                                         GFile                      *file,
                                         const char                 *template,
                                         int                         length)
{
	self->priv->target_file = g_object_ref (file);
	self->priv->template = g_strdup (template);
	self->priv->length = length;
}

/* rename */
G_DEFINE_TYPE (NautilusFileUndoInfoRename, nautilus_file_undo_info_rename, NAUTILUS_TYPE_FILE_UNDO_INFO)

struct _NautilusFileUndoInfoRenameDetails {
	GFile *old_file;
	GFile *new_file;
};

static void
rename_strings_func (NautilusFileUndoInfo *info,
		     char **undo_label,
		     char **undo_description,
		     char **redo_label,
		     char **redo_description)
{
	NautilusFileUndoInfoRename *self = NAUTILUS_FILE_UNDO_INFO_RENAME (info);
	char *new_name, *old_name;

	new_name = g_file_get_parse_name (self->priv->new_file);
	old_name = g_file_get_parse_name (self->priv->old_file);

	*undo_description = g_strdup_printf (_("Rename '%s' as '%s'"), new_name, old_name);
	*redo_description = g_strdup_printf (_("Rename '%s' as '%s'"), old_name, new_name);

	*undo_label = g_strdup (_("_Undo Rename"));
	*redo_label = g_strdup (_("_Redo Rename"));

	g_free (old_name);
	g_free (new_name);
}

static void
rename_redo_func (NautilusFileUndoInfo *info,
		  GtkWindow *parent_window)
{
	NautilusFileUndoInfoRename *self = NAUTILUS_FILE_UNDO_INFO_RENAME (info);
	char *new_name;
	NautilusFile *file;

	new_name = g_file_get_basename (self->priv->new_file);
	file = nautilus_file_get (self->priv->old_file);
	nautilus_file_rename (file, new_name,
			      file_undo_info_operation_callback, self);

	nautilus_file_unref (file);
	g_free (new_name);
}

static void
rename_undo_func (NautilusFileUndoInfo *info,
		  GtkWindow *parent_window)
{
	NautilusFileUndoInfoRename *self = NAUTILUS_FILE_UNDO_INFO_RENAME (info);
	char *new_name;
	NautilusFile *file;

	new_name = g_file_get_basename (self->priv->old_file);
	file = nautilus_file_get (self->priv->new_file);
	nautilus_file_rename (file, new_name,
			      file_undo_info_operation_callback, self);

	nautilus_file_unref (file);
	g_free (new_name);
}

static void
nautilus_file_undo_info_rename_init (NautilusFileUndoInfoRename *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, nautilus_file_undo_info_rename_get_type (),
						  NautilusFileUndoInfoRenameDetails);
}

static void
nautilus_file_undo_info_rename_finalize (GObject *obj)
{
	NautilusFileUndoInfoRename *self = NAUTILUS_FILE_UNDO_INFO_RENAME (obj);
	g_clear_object (&self->priv->old_file);
	g_clear_object (&self->priv->new_file);

	G_OBJECT_CLASS (nautilus_file_undo_info_rename_parent_class)->finalize (obj);
}

static void
nautilus_file_undo_info_rename_class_init (NautilusFileUndoInfoRenameClass *klass)
{
	GObjectClass *oclass = G_OBJECT_CLASS (klass);
	NautilusFileUndoInfoClass *iclass = NAUTILUS_FILE_UNDO_INFO_CLASS (klass);

	oclass->finalize = nautilus_file_undo_info_rename_finalize;

	iclass->undo_func = rename_undo_func;
	iclass->redo_func = rename_redo_func;
	iclass->strings_func = rename_strings_func;

	g_type_class_add_private (klass, sizeof (NautilusFileUndoInfoRenameDetails));
}

NautilusFileUndoInfo *
nautilus_file_undo_info_rename_new (void)
{
	return g_object_new (NAUTILUS_TYPE_FILE_UNDO_INFO_RENAME,
			     "op-type", NAUTILUS_FILE_UNDO_OP_RENAME,
			     "item-count", 1,
			     NULL);
}

void
nautilus_file_undo_info_rename_set_data (NautilusFileUndoInfoRename *self,
					 GFile                      *old_file,
					 GFile                      *new_file)
{
	self->priv->old_file = g_object_ref (old_file);
	self->priv->new_file = g_object_ref (new_file);
}

/* trash */
G_DEFINE_TYPE (NautilusFileUndoInfoTrash, nautilus_file_undo_info_trash, NAUTILUS_TYPE_FILE_UNDO_INFO)

struct _NautilusFileUndoInfoTrashDetails {
	GHashTable *trashed;
};

static void
trash_strings_func (NautilusFileUndoInfo *info,
		    char **undo_label,
		    char **undo_description,
		    char **redo_label,
		    char **redo_description)
{
	NautilusFileUndoInfoTrash *self = NAUTILUS_FILE_UNDO_INFO_TRASH (info);
	unsigned int count = g_hash_table_size (self->priv->trashed);

	if (count != 1) {
		*undo_description = g_strdup_printf (ngettext ("Restore %d item from trash",
							       "Restore %d items from trash", count),
						     count);
		*redo_description = g_strdup_printf (ngettext ("Move %d item to trash",
							       "Move %d items to trash", count),
						     count);
	} else {
		GList *keys;
		char *name, *orig_path;
		GFile *file;

		keys = g_hash_table_get_keys (self->priv->trashed);
		file = keys->data;
		name = g_file_get_basename (file);
		orig_path = g_file_get_path (file);
		*undo_description = g_strdup_printf (_("Restore '%s' to '%s'"), name, orig_path);

		g_free (name);
		g_free (orig_path);
		g_list_free (keys);

		name = g_file_get_parse_name (file);
		*redo_description = g_strdup_printf (_("Move '%s' to trash"), name);

		g_free (name);

		*undo_label = g_strdup (_("_Undo Trash"));
		*redo_label = g_strdup (_("_Redo Trash"));
	}
}

static void
trash_redo_func_callback (GHashTable *debuting_uris, _Bool user_cancel, void *user_data)
{
	NautilusFileUndoInfoTrash *self = user_data;
	GHashTable *new_trashed_files;
	GTimeVal current_time;
	size_t updated_trash_time;
	GFile *file;
	GList *keys, *l;

	if (!user_cancel) {
		new_trashed_files =
			g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal,
					       g_object_unref, NULL);

		keys = g_hash_table_get_keys (self->priv->trashed);

		g_get_current_time (&current_time);
		updated_trash_time = current_time.tv_sec;

		for (l = keys; l != NULL; l = l->next) {
			file = l->data;
			g_hash_table_insert (new_trashed_files,
					     g_object_ref (file), GSIZE_TO_POINTER (updated_trash_time));
		}

		g_list_free (keys);
		g_hash_table_destroy (self->priv->trashed);

		self->priv->trashed = new_trashed_files;
	}

	file_undo_info_delete_callback (debuting_uris, user_cancel, user_data);
}

static void
trash_redo_func (NautilusFileUndoInfo *info,
		 GtkWindow *parent_window)
{
	NautilusFileUndoInfoTrash *self = NAUTILUS_FILE_UNDO_INFO_TRASH (info);

	if (g_hash_table_size (self->priv->trashed) > 0) {
		GList *locations;

		locations = g_hash_table_get_keys (self->priv->trashed);
		nautilus_file_operations_trash_or_delete (locations, parent_window,
							  trash_redo_func_callback, self);

		g_list_free (locations);
	}
}

static GHashTable *
trash_retrieve_files_to_restore_finish (NautilusFileUndoInfoTrash *self,
					GAsyncResult *res,
					GError **error)
{
	GHashTable *retval = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT (res));

	if (retval == NULL) {
		g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error);
	}

	return retval;
}

static void
trash_retrieve_files_to_restore_thread (GSimpleAsyncResult *res,
					GObject *object,
					GCancellable *cancellable)
{
	NautilusFileUndoInfoTrash *self = NAUTILUS_FILE_UNDO_INFO_TRASH (object);
	GFileEnumerator *enumerator;
	GHashTable *to_restore;
	GFile *trash;
	GError *error = NULL;

	to_restore = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal,
					    g_object_unref, g_object_unref);

	trash = g_file_new_for_uri ("trash:///");

	enumerator = g_file_enumerate_children (trash,
			G_FILE_ATTRIBUTE_STANDARD_NAME","
			G_FILE_ATTRIBUTE_TRASH_DELETION_DATE","
			G_FILE_ATTRIBUTE_TRASH_ORIG_PATH,
			G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
			NULL, &error);

	if (enumerator) {
		GFileInfo *info;
		void * lookupvalue;
		GFile *item;
		GTimeVal timeval;
		glong trash_time, orig_trash_time;
		const char *origpath;
		GFile *origfile;
		const char *time_string;

		while ((info = g_file_enumerator_next_file (enumerator, NULL, &error)) != NULL) {
			/* Retrieve the original file uri */
			origpath = g_file_info_get_attribute_byte_string (info, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH);
			origfile = g_file_new_for_path (origpath);

			lookupvalue = g_hash_table_lookup (self->priv->trashed, origfile);

			if (lookupvalue) {
				orig_trash_time = GPOINTER_TO_SIZE (lookupvalue);
				time_string = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_TRASH_DELETION_DATE);
				if (time_string != NULL) {
					g_time_val_from_iso8601 (time_string, &timeval);
					trash_time = timeval.tv_sec;
				} else {
					trash_time = 0;
				}

				if (trash_time == orig_trash_time) {
					/* File in the trash */
					item = g_file_get_child (trash, g_file_info_get_name (info));
					g_hash_table_insert (to_restore, item, g_object_ref (origfile));
				}
			}

			g_object_unref (origfile);

		}
		g_file_enumerator_close (enumerator, FALSE, NULL);
		g_object_unref (enumerator);
	}
	g_object_unref (trash);

	if (error != NULL) {
		g_simple_async_result_take_error (res, error);
		g_hash_table_destroy (to_restore);
	} else {
		g_simple_async_result_set_op_res_gpointer(res, to_restore, NULL);
	}
}

static void
trash_retrieve_files_to_restore_async (NautilusFileUndoInfoTrash *self,
				 GAsyncReadyCallback callback,
				 void * user_data)
{
	GSimpleAsyncResult *async_op;

	async_op = g_simple_async_result_new (G_OBJECT (self), callback, user_data,
					      trash_retrieve_files_to_restore_async);
	g_simple_async_result_run_in_thread (async_op, trash_retrieve_files_to_restore_thread,
					     G_PRIORITY_DEFAULT, NULL);

	g_object_unref (async_op);
}

static void
trash_retrieve_files_ready (GObject *source,
			    GAsyncResult *res,
			    void * user_data)
{
	NautilusFileUndoInfoTrash *self = NAUTILUS_FILE_UNDO_INFO_TRASH (source);
	GHashTable *files_to_restore;
	GError *error = NULL;

	files_to_restore = trash_retrieve_files_to_restore_finish (self, res, &error);

	if (error == NULL && g_hash_table_size (files_to_restore) > 0) {
		GList *gfiles_in_trash, *l;
		GFile *item;
		GFile *dest;

		gfiles_in_trash = g_hash_table_get_keys (files_to_restore);

		for (l = gfiles_in_trash; l != NULL; l = l->next) {
			item = l->data;
			dest = g_hash_table_lookup (files_to_restore, item);

			g_file_move (item, dest, G_FILE_COPY_NOFOLLOW_SYMLINKS, NULL, NULL, NULL, NULL);
		}

		g_list_free (gfiles_in_trash);

		/* Here we must do what's necessary for the callback */
		file_undo_info_transfer_callback (NULL, (error == NULL), self);
	} else {
		file_undo_info_transfer_callback (NULL, FALSE, self);
	}

	if (files_to_restore != NULL) {
		g_hash_table_destroy (files_to_restore);
	}

	g_clear_error (&error);
}

static void
trash_undo_func (NautilusFileUndoInfo *info,
		 GtkWindow *parent_window)
{
	NautilusFileUndoInfoTrash *self = NAUTILUS_FILE_UNDO_INFO_TRASH (info);

	/* Internally managed op, pop flag. */
	nautilus_file_undo_manager_pop_flag ();

	trash_retrieve_files_to_restore_async (self, trash_retrieve_files_ready, NULL);
}

static void
nautilus_file_undo_info_trash_init (NautilusFileUndoInfoTrash *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, nautilus_file_undo_info_trash_get_type (),
						  NautilusFileUndoInfoTrashDetails);
	self->priv->trashed =
		g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal,
				       g_object_unref, NULL);
}

static void
nautilus_file_undo_info_trash_finalize (GObject *obj)
{
	NautilusFileUndoInfoTrash *self = NAUTILUS_FILE_UNDO_INFO_TRASH (obj);
	g_hash_table_destroy (self->priv->trashed);

	G_OBJECT_CLASS (nautilus_file_undo_info_trash_parent_class)->finalize (obj);
}

static void
nautilus_file_undo_info_trash_class_init (NautilusFileUndoInfoTrashClass *klass)
{
	GObjectClass *oclass = G_OBJECT_CLASS (klass);
	NautilusFileUndoInfoClass *iclass = NAUTILUS_FILE_UNDO_INFO_CLASS (klass);

	oclass->finalize = nautilus_file_undo_info_trash_finalize;

	iclass->undo_func = trash_undo_func;
	iclass->redo_func = trash_redo_func;
	iclass->strings_func = trash_strings_func;

	g_type_class_add_private (klass, sizeof (NautilusFileUndoInfoTrashDetails));
}

NautilusFileUndoInfo *
nautilus_file_undo_info_trash_new (int item_count)
{
	return g_object_new (NAUTILUS_TYPE_FILE_UNDO_INFO_TRASH,
			     "op-type", NAUTILUS_FILE_UNDO_OP_MOVE_TO_TRASH,
			     "item-count", item_count,
			     NULL);
}

void
nautilus_file_undo_info_trash_add_file (NautilusFileUndoInfoTrash *self,
					GFile                     *file)
{
	GTimeVal current_time;
	size_t orig_trash_time;

	g_get_current_time (&current_time);
	orig_trash_time = current_time.tv_sec;

	g_hash_table_insert (self->priv->trashed, g_object_ref (file), GSIZE_TO_POINTER (orig_trash_time));
}

/* recursive permissions */
G_DEFINE_TYPE (NautilusFileUndoInfoRecPermissions, nautilus_file_undo_info_rec_permissions, NAUTILUS_TYPE_FILE_UNDO_INFO)

struct _NautilusFileUndoInfoRecPermissionsDetails {
	GFile *dest_dir;
	GHashTable *original_permissions;
	unsigned int dir_mask;
	unsigned int dir_permissions;
	unsigned int file_mask;
	unsigned int file_permissions;
};

static void
rec_permissions_strings_func (NautilusFileUndoInfo *info,
			      char **undo_label,
			      char **undo_description,
			      char **redo_label,
			      char **redo_description)
{
	NautilusFileUndoInfoRecPermissions *self = NAUTILUS_FILE_UNDO_INFO_REC_PERMISSIONS (info);
	char *name;

	name = g_file_get_path (self->priv->dest_dir);

	*undo_description = g_strdup_printf (_("Restore original permissions of items enclosed in '%s'"), name);
	*redo_description = g_strdup_printf (_("Set permissions of items enclosed in '%s'"), name);

	*undo_label = g_strdup (_("_Undo Change Permissions"));
	*redo_label = g_strdup (_("_Redo Change Permissions"));

	g_free (name);
}

static void
rec_permissions_callback (_Bool success,
			  void * callback_data)
{
	file_undo_info_transfer_callback (NULL, success, callback_data);
}

static void
rec_permissions_redo_func (NautilusFileUndoInfo *info,
			   GtkWindow *parent_window)
{
	NautilusFileUndoInfoRecPermissions *self = NAUTILUS_FILE_UNDO_INFO_REC_PERMISSIONS (info);
	char *parent_uri;

	parent_uri = g_file_get_uri (self->priv->dest_dir);
	nautilus_file_set_permissions_recursive (parent_uri,
						 self->priv->file_permissions,
						 self->priv->file_mask,
						 self->priv->dir_permissions,
						 self->priv->dir_mask,
						 rec_permissions_callback, self);
	g_free (parent_uri);
}

static void
rec_permissions_undo_func (NautilusFileUndoInfo *info,
			   GtkWindow *parent_window)
{
	NautilusFileUndoInfoRecPermissions *self = NAUTILUS_FILE_UNDO_INFO_REC_PERMISSIONS (info);

	/* Internally managed op, pop flag. */
	/* TODO: why? */
	nautilus_file_undo_manager_pop_flag ();

	if (g_hash_table_size (self->priv->original_permissions) > 0) {
		GList *gfiles_list;
		unsigned int perm;
		GList *l;
		GFile *dest;
		char *item;

		gfiles_list = g_hash_table_get_keys (self->priv->original_permissions);
		for (l = gfiles_list; l != NULL; l = l->next) {
			item = l->data;
			perm = GPOINTER_TO_UINT (g_hash_table_lookup (self->priv->original_permissions, item));
			dest = g_file_new_for_uri (item);
			g_file_set_attribute_uint32 (dest,
						     G_FILE_ATTRIBUTE_UNIX_MODE,
						     perm, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
			g_object_unref (dest);
		}

		g_list_free (gfiles_list);
		/* Here we must do what's necessary for the callback */
		file_undo_info_transfer_callback (NULL, TRUE, self);
	}
}

static void
nautilus_file_undo_info_rec_permissions_init (NautilusFileUndoInfoRecPermissions *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, nautilus_file_undo_info_rec_permissions_get_type (),
						  NautilusFileUndoInfoRecPermissionsDetails);

	self->priv->original_permissions =
		g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static void
nautilus_file_undo_info_rec_permissions_finalize (GObject *obj)
{
	NautilusFileUndoInfoRecPermissions *self = NAUTILUS_FILE_UNDO_INFO_REC_PERMISSIONS (obj);

	g_hash_table_destroy (self->priv->original_permissions);
	g_clear_object (&self->priv->dest_dir);

	G_OBJECT_CLASS (nautilus_file_undo_info_rec_permissions_parent_class)->finalize (obj);
}

static void
nautilus_file_undo_info_rec_permissions_class_init (NautilusFileUndoInfoRecPermissionsClass *klass)
{
	GObjectClass *oclass = G_OBJECT_CLASS (klass);
	NautilusFileUndoInfoClass *iclass = NAUTILUS_FILE_UNDO_INFO_CLASS (klass);

	oclass->finalize = nautilus_file_undo_info_rec_permissions_finalize;

	iclass->undo_func = rec_permissions_undo_func;
	iclass->redo_func = rec_permissions_redo_func;
	iclass->strings_func = rec_permissions_strings_func;

	g_type_class_add_private (klass, sizeof (NautilusFileUndoInfoRecPermissionsDetails));
}

NautilusFileUndoInfo *
nautilus_file_undo_info_rec_permissions_new (GFile   *dest,
					     unsigned int file_permissions,
					     unsigned int file_mask,
					     unsigned int dir_permissions,
					     unsigned int dir_mask)
{
	NautilusFileUndoInfoRecPermissions *retval;

	retval = g_object_new (NAUTILUS_TYPE_FILE_UNDO_INFO_REC_PERMISSIONS,
			       "op-type", NAUTILUS_FILE_UNDO_OP_RECURSIVE_SET_PERMISSIONS,
			       "item-count", 1,
			       NULL);

	retval->priv->dest_dir = g_object_ref (dest);
	retval->priv->file_permissions = file_permissions;
	retval->priv->file_mask = file_mask;
	retval->priv->dir_permissions = dir_permissions;
	retval->priv->dir_mask = dir_mask;

	return NAUTILUS_FILE_UNDO_INFO (retval);
}

void
nautilus_file_undo_info_rec_permissions_add_file (NautilusFileUndoInfoRecPermissions *self,
						  GFile                              *file,
						  unsigned int                             permission)
{
	char *original_uri = g_file_get_uri (file);
	g_hash_table_insert (self->priv->original_permissions, original_uri, GUINT_TO_POINTER (permission));
}

/* single file change permissions */
G_DEFINE_TYPE (NautilusFileUndoInfoPermissions, nautilus_file_undo_info_permissions, NAUTILUS_TYPE_FILE_UNDO_INFO)

struct _NautilusFileUndoInfoPermissionsDetails {
	GFile *target_file;
	unsigned int current_permissions;
	unsigned int new_permissions;
};

static void
permissions_strings_func (NautilusFileUndoInfo *info,
			  char **undo_label,
			  char **undo_description,
			  char **redo_label,
			  char **redo_description)
{
	NautilusFileUndoInfoPermissions *self = NAUTILUS_FILE_UNDO_INFO_PERMISSIONS (info);
	char *name;

	name = g_file_get_parse_name (self->priv->target_file);
	*undo_description = g_strdup_printf (_("Restore original permissions of '%s'"), name);
	*redo_description = g_strdup_printf (_("Set permissions of '%s'"), name);

	*undo_label = g_strdup (_("_Undo Change Permissions"));
	*redo_label = g_strdup (_("_Redo Change Permissions"));

	g_free (name);
}

static void
permissions_real_func (NautilusFileUndoInfoPermissions *self,
		       unsigned int permissions)
{
	NautilusFile *file;

	file = nautilus_file_get (self->priv->target_file);
	nautilus_file_set_permissions (file, permissions,
				       file_undo_info_operation_callback, self);

	nautilus_file_unref (file);
}

static void
permissions_redo_func (NautilusFileUndoInfo *info,
		       GtkWindow *parent_window)
{
	NautilusFileUndoInfoPermissions *self = NAUTILUS_FILE_UNDO_INFO_PERMISSIONS (info);
	permissions_real_func (self, self->priv->new_permissions);
}

static void
permissions_undo_func (NautilusFileUndoInfo *info,
		       GtkWindow *parent_window)
{
	NautilusFileUndoInfoPermissions *self = NAUTILUS_FILE_UNDO_INFO_PERMISSIONS (info);
	permissions_real_func (self, self->priv->current_permissions);
}

static void
nautilus_file_undo_info_permissions_init (NautilusFileUndoInfoPermissions *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, nautilus_file_undo_info_permissions_get_type (),
						  NautilusFileUndoInfoPermissionsDetails);
}

static void
nautilus_file_undo_info_permissions_finalize (GObject *obj)
{
	NautilusFileUndoInfoPermissions *self = NAUTILUS_FILE_UNDO_INFO_PERMISSIONS (obj);
	g_clear_object (&self->priv->target_file);

	G_OBJECT_CLASS (nautilus_file_undo_info_permissions_parent_class)->finalize (obj);
}

static void
nautilus_file_undo_info_permissions_class_init (NautilusFileUndoInfoPermissionsClass *klass)
{
	GObjectClass *oclass = G_OBJECT_CLASS (klass);
	NautilusFileUndoInfoClass *iclass = NAUTILUS_FILE_UNDO_INFO_CLASS (klass);

	oclass->finalize = nautilus_file_undo_info_permissions_finalize;

	iclass->undo_func = permissions_undo_func;
	iclass->redo_func = permissions_redo_func;
	iclass->strings_func = permissions_strings_func;

	g_type_class_add_private (klass, sizeof (NautilusFileUndoInfoPermissionsDetails));
}

NautilusFileUndoInfo *
nautilus_file_undo_info_permissions_new (GFile   *file,
					 unsigned int  current_permissions,
					 unsigned int  new_permissions)
{
	NautilusFileUndoInfoPermissions *retval;

	retval = g_object_new (NAUTILUS_TYPE_FILE_UNDO_INFO_PERMISSIONS,
			     "op-type", NAUTILUS_FILE_UNDO_OP_SET_PERMISSIONS,
			     "item-count", 1,
			     NULL);

	retval->priv->target_file = g_object_ref (file);
	retval->priv->current_permissions = current_permissions;
	retval->priv->new_permissions = new_permissions;

	return NAUTILUS_FILE_UNDO_INFO (retval);
}

/* group and owner change */
G_DEFINE_TYPE (NautilusFileUndoInfoOwnership, nautilus_file_undo_info_ownership, NAUTILUS_TYPE_FILE_UNDO_INFO)

struct _NautilusFileUndoInfoOwnershipDetails {
	GFile *target_file;
	char *original_ownership;
	char *new_ownership;
};

static void
ownership_strings_func (NautilusFileUndoInfo *info,
			char **undo_label,
			char **undo_description,
			char **redo_label,
			char **redo_description)
{
	NautilusFileUndoInfoOwnership *self = NAUTILUS_FILE_UNDO_INFO_OWNERSHIP (info);
	NautilusFileUndoOp op_type = nautilus_file_undo_info_get_op_type (info);
	char *name;

	name = g_file_get_parse_name (self->priv->target_file);

	if (op_type == NAUTILUS_FILE_UNDO_OP_CHANGE_OWNER) {
		*undo_description = g_strdup_printf (_("Restore group of '%s' to '%s'"),
						     name, self->priv->original_ownership);
		*redo_description = g_strdup_printf (_("Set group of '%s' to '%s'"),
						     name, self->priv->new_ownership);

		*undo_label = g_strdup (_("_Undo Change Group"));
		*redo_label = g_strdup (_("_Redo Change Group"));
	} else if (op_type == NAUTILUS_FILE_UNDO_OP_CHANGE_GROUP) {
		*undo_description = g_strdup_printf (_("Restore owner of '%s' to '%s'"),
						     name, self->priv->original_ownership);
		*redo_description = g_strdup_printf (_("Set owner of '%s' to '%s'"),
						     name, self->priv->new_ownership);

		*undo_label = g_strdup (_("_Undo Change Owner"));
		*redo_label = g_strdup (_("_Redo Change Owner"));
	}

	g_free (name);
}

static void
ownership_real_func (NautilusFileUndoInfoOwnership *self,
		     const char *ownership)
{
	NautilusFileUndoOp op_type = nautilus_file_undo_info_get_op_type (NAUTILUS_FILE_UNDO_INFO (self));
	NautilusFile *file;

	file = nautilus_file_get (self->priv->target_file);

	if (op_type == NAUTILUS_FILE_UNDO_OP_CHANGE_OWNER) {
		nautilus_file_set_owner (file,
					 ownership,
					 file_undo_info_operation_callback, self);
	} else if (op_type == NAUTILUS_FILE_UNDO_OP_CHANGE_GROUP) {
		nautilus_file_set_group (file,
					 ownership,
					 file_undo_info_operation_callback, self);
	}

	nautilus_file_unref (file);
}

static void
ownership_redo_func (NautilusFileUndoInfo *info,
		     GtkWindow *parent_window)
{
	NautilusFileUndoInfoOwnership *self = NAUTILUS_FILE_UNDO_INFO_OWNERSHIP (info);
	ownership_real_func (self, self->priv->new_ownership);
}

static void
ownership_undo_func (NautilusFileUndoInfo *info,
		     GtkWindow *parent_window)
{
	NautilusFileUndoInfoOwnership *self = NAUTILUS_FILE_UNDO_INFO_OWNERSHIP (info);
	ownership_real_func (self, self->priv->original_ownership);
}

static void
nautilus_file_undo_info_ownership_init (NautilusFileUndoInfoOwnership *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, nautilus_file_undo_info_ownership_get_type (),
						  NautilusFileUndoInfoOwnershipDetails);
}

static void
nautilus_file_undo_info_ownership_finalize (GObject *obj)
{
	NautilusFileUndoInfoOwnership *self = NAUTILUS_FILE_UNDO_INFO_OWNERSHIP (obj);

	g_clear_object (&self->priv->target_file);
	g_free (self->priv->original_ownership);
	g_free (self->priv->new_ownership);

	G_OBJECT_CLASS (nautilus_file_undo_info_ownership_parent_class)->finalize (obj);
}

static void
nautilus_file_undo_info_ownership_class_init (NautilusFileUndoInfoOwnershipClass *klass)
{
	GObjectClass *oclass = G_OBJECT_CLASS (klass);
	NautilusFileUndoInfoClass *iclass = NAUTILUS_FILE_UNDO_INFO_CLASS (klass);

	oclass->finalize = nautilus_file_undo_info_ownership_finalize;

	iclass->undo_func = ownership_undo_func;
	iclass->redo_func = ownership_redo_func;
	iclass->strings_func = ownership_strings_func;

	g_type_class_add_private (klass, sizeof (NautilusFileUndoInfoOwnershipDetails));
}

NautilusFileUndoInfo *
nautilus_file_undo_info_ownership_new (NautilusFileUndoOp  op_type,
				       GFile              *file,
				       const char         *current_data,
				       const char         *new_data)
{
	NautilusFileUndoInfoOwnership *retval;

	retval = g_object_new (NAUTILUS_TYPE_FILE_UNDO_INFO_OWNERSHIP,
			       "item-count", 1,
			       "op-type", op_type,
			       NULL);

	retval->priv->target_file = g_object_ref (file);
	retval->priv->original_ownership = g_strdup (current_data);
	retval->priv->new_ownership = g_strdup (new_data);

	return NAUTILUS_FILE_UNDO_INFO (retval);
}
