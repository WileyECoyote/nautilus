/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   nautilus-file.h: Nautilus file model.

   Copyright (C) 1999, 2000, 2001 Eazel, Inc.

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

   Author: Darin Adler <darin@bentspoon.com>
*/

#ifndef NAUTILUS_FILE_H
#define NAUTILUS_FILE_H

/* NautilusFile is an object used to represent a single element of a
 * NautilusDirectory. It's lightweight and relies on NautilusDirectory
 * to do most of the work.
 */

/* NautilusFile is defined here and in nautilus-directory.h, nautilus-file-info.h. */
#ifndef NAUTILUS_FILE_DEFINED
#define NAUTILUS_FILE_DEFINED
typedef struct NautilusFile NautilusFile;
#endif

#define NAUTILUS_TYPE_FILE nautilus_file_get_type()
#define NAUTILUS_FILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NAUTILUS_TYPE_FILE, NautilusFile))
#define NAUTILUS_FILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  NAUTILUS_TYPE_FILE, NautilusFileClass))
#define NAUTILUS_IS_FILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NAUTILUS_TYPE_FILE))
#define NAUTILUS_IS_FILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  NAUTILUS_TYPE_FILE))
#define NAUTILUS_FILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  NAUTILUS_TYPE_FILE, NautilusFileClass))
#define ERROR_NOT_NAUTILUS_FILE \
    fprintf(stderr, "%s ERROR: file is not a NautilusFile object, BAD pointer <%p>", __func__, file);

#define IS_A_NAUTILUS_FILE(obj) (obj && (NAUTILUS_TYPE_FILE == obj->head_marker)) ? TRUE : FALSE

typedef enum {
        NAUTILUS_FILE_SORT_NONE,
        NAUTILUS_FILE_SORT_BY_DISPLAY_NAME,
        NAUTILUS_FILE_SORT_BY_SIZE,
        NAUTILUS_FILE_SORT_BY_TYPE,
        NAUTILUS_FILE_SORT_BY_DETAILED_TYPE,
        NAUTILUS_FILE_SORT_BY_MTIME,
        NAUTILUS_FILE_SORT_BY_ATIME,
        NAUTILUS_FILE_SORT_BY_TRASHED_TIME
} NautilusFileSortType;

typedef enum {
	NAUTILUS_REQUEST_NOT_STARTED,
	NAUTILUS_REQUEST_IN_PROGRESS,
	NAUTILUS_REQUEST_DONE
} NautilusRequestStatus;

typedef enum {
	NAUTILUS_FILE_ICON_FLAGS_NONE = 0,
	NAUTILUS_FILE_ICON_FLAGS_USE_THUMBNAILS = (1<<0),
	NAUTILUS_FILE_ICON_FLAGS_IGNORE_VISITING = (1<<1),
	NAUTILUS_FILE_ICON_FLAGS_EMBEDDING_TEXT = (1<<2),
	NAUTILUS_FILE_ICON_FLAGS_FOR_DRAG_ACCEPT = (1<<3),
	NAUTILUS_FILE_ICON_FLAGS_FOR_OPEN_FOLDER = (1<<4),
	/* whether the thumbnail size must match the display icon size */
	NAUTILUS_FILE_ICON_FLAGS_FORCE_THUMBNAIL_SIZE = (1<<5),
	/* uses the icon of the mount if present */
	NAUTILUS_FILE_ICON_FLAGS_USE_MOUNT_ICON = (1<<6),
	/* render the mount icon as an emblem over the regular one */
	NAUTILUS_FILE_ICON_FLAGS_USE_MOUNT_ICON_AS_EMBLEM = (1<<7)
} NautilusFileIconFlags;

typedef enum {
    NAUTILUS_DATE_TYPE_MODIFIED,
    NAUTILUS_DATE_TYPE_CHANGED,
    NAUTILUS_DATE_TYPE_ACCESSED,
    NAUTILUS_DATE_TYPE_PERMISSIONS_CHANGED,
    NAUTILUS_DATE_TYPE_TRASHED
} NautilusDateType;

typedef enum {
    NAUTILUS_FILE_TOOLTIP_FLAGS_NONE = 0,
    NAUTILUS_FILE_TOOLTIP_FLAGS_FILE_TYPE =  (1<<0),
    NAUTILUS_FILE_TOOLTIP_FLAGS_MOD_DATE = (1<<1),
    NAUTILUS_FILE_TOOLTIP_FLAGS_ACCESS_DATE = (1<<2),
    NAUTILUS_FILE_TOOLTIP_FLAGS_PATH = (1<<3),
    NAUTILUS_FILE_TOOLTIP_FLAGS_PERMISSIONS = (1<<4),
    NAUTILUS_FILE_TOOLTIP_FLAGS_OCTAL = (1<<5)
} NautilusFileTooltipFlags;

/* Emblems sometimes displayed for NautilusFiles. Do not localize. */
#define NAUTILUS_FILE_EMBLEM_NAME_SYMBOLIC_LINK "symbolic-link"
#define NAUTILUS_FILE_EMBLEM_NAME_CANT_READ "noread"
#define NAUTILUS_FILE_EMBLEM_NAME_CANT_WRITE "nowrite"
#define NAUTILUS_FILE_EMBLEM_NAME_TRASH "trash"
#define NAUTILUS_FILE_EMBLEM_NAME_NOTE "note"

typedef void (*NautilusFileCallback)          (NautilusFile  *file,
                                               void          *callback_data);
typedef void (*NautilusFileListCallback)      (GList         *file_list,
                                               void          *callback_data);
typedef void (*NautilusFileOperationCallback) (NautilusFile  *file,
                                               GFile         *result_location,
                                               GError        *error,
                                               void          *callback_data);
typedef int (*NautilusWidthMeasureCallback)   (const char    *string,
                                               void	     *context);
typedef char * (*NautilusTruncateCallback)    (const char    *string,
                                               int	      width,
                                               void	     *context);


#define NAUTILUS_FILE_ATTRIBUTES_FOR_ICON (NAUTILUS_FILE_ATTRIBUTE_INFO | NAUTILUS_FILE_ATTRIBUTE_LINK_INFO | NAUTILUS_FILE_ATTRIBUTE_THUMBNAIL)

typedef void NautilusFileListHandle;

/* GObject requirements. */
unsigned int            nautilus_file_get_type                          (void);

/* Getting at a single file. */
NautilusFile *          nautilus_file_get                               (GFile                          *location);
NautilusFile *          nautilus_file_get_by_uri                        (const char                     *uri);

/* Get a file only if the nautilus version already exists */
NautilusFile *          nautilus_file_get_existing                      (GFile                          *location);
NautilusFile *          nautilus_file_get_existing_by_uri               (const char                     *uri);

/* Covers for g_object_ref and g_object_unref that provide two conveniences:
 * 1) Using these is type safe.
 * 2) Can be called with NULL argument,
 */
NautilusFile *          nautilus_file_ref                               (NautilusFile                   *file);
void                    nautilus_file_unref                             (NautilusFile                   *file);

/* Monitor the file. */
void                    nautilus_file_monitor_add                       (NautilusFile                   *file,
                                                                         const void                     *client,
                                                                         NautilusFileAttributes          attributes);
void                    nautilus_file_monitor_remove                    (NautilusFile                   *file,
                                                                         const void                     *client);

/* Waiting for data that's read asynchronously.
 * This interface currently works only for metadata, but could be expanded
 * to other attributes as well.
 */
void                    nautilus_file_call_when_ready                   (NautilusFile                   *file,
                                                                         NautilusFileAttributes          attributes,
                                                                         NautilusFileCallback            callback,
                                                                         void                            *callback_data);
void                    nautilus_file_cancel_call_when_ready            (NautilusFile                   *file,
                                                                         NautilusFileCallback            callback,
                                                                         void                            *callback_data);
_Bool                   nautilus_file_check_if_ready                    (NautilusFile                   *file,
                                                                         NautilusFileAttributes          attributes);
void                    nautilus_file_invalidate_attributes             (NautilusFile                   *file,
                                                                         NautilusFileAttributes          attributes);
void                    nautilus_file_invalidate_all_attributes         (NautilusFile                   *file);

void                    nautilus_file_increment_thumbnail_try_count     (NautilusFile                   *file);

/* Basic attributes for file objects. */
_Bool                   nautilus_file_contains_text                     (NautilusFile                   *file);
char                   *nautilus_file_get_display_name                  (NautilusFile                   *file);
char                   *nautilus_file_get_edit_name                     (NautilusFile                   *file);
char                   *nautilus_file_get_name                          (NautilusFile                   *file);
GFile                  *nautilus_file_get_location                      (NautilusFile                   *file);
char                   *nautilus_file_get_description                   (NautilusFile                   *file);
char                   *nautilus_file_get_uri                           (NautilusFile                   *file);
char                   *nautilus_file_get_uri_scheme                    (NautilusFile                   *file);
char                   *nautilus_file_get_path                          (NautilusFile                   *file);
NautilusFile           *nautilus_file_get_parent                        (NautilusFile                   *file);
GFile                  *nautilus_file_get_parent_location               (NautilusFile                   *file);
char                   *nautilus_file_get_parent_uri                    (NautilusFile                   *file);
char                   *nautilus_file_get_parent_uri_for_display        (NautilusFile                   *file);
_Bool                   nautilus_file_can_get_size                      (NautilusFile                   *file);
goffset                 nautilus_file_get_size                          (NautilusFile                   *file);
time_t                  nautilus_file_get_mtime                         (NautilusFile                   *file);
time_t                  nautilus_file_get_ctime                         (NautilusFile                   *file);
GFileType               nautilus_file_get_file_type                     (NautilusFile                   *file);
char                   *nautilus_file_get_mime_type                     (NautilusFile                   *file);
_Bool                   nautilus_file_is_mime_type                      (NautilusFile                   *file,
                                                                         const char                     *mime_type);
_Bool                   nautilus_file_is_launchable                     (NautilusFile                   *file);
_Bool                   nautilus_file_is_symbolic_link                  (NautilusFile                   *file);
_Bool                   nautilus_file_is_mountpoint                     (NautilusFile                   *file);
GMount                 *nautilus_file_get_mount                         (NautilusFile                   *file);
char                   *nautilus_file_get_volume_free_space             (NautilusFile                   *file);
char                   *nautilus_file_get_volume_name                   (NautilusFile                   *file);
char                   *nautilus_file_get_symbolic_link_target_path     (NautilusFile                   *file);
char                   *nautilus_file_get_symbolic_link_target_uri      (NautilusFile                   *file);
_Bool                   nautilus_file_is_broken_symbolic_link           (NautilusFile                   *file);
_Bool                   nautilus_file_is_nautilus_link                  (NautilusFile                   *file);
_Bool                   nautilus_file_is_executable                     (NautilusFile                   *file);
_Bool                   nautilus_file_is_directory                      (NautilusFile                   *file);
_Bool                   nautilus_file_is_user_special_directory         (NautilusFile                   *file,
                                                                         GUserDirectory                 special_directory);
_Bool                   nautilus_file_is_archive                        (NautilusFile                   *file);
_Bool                   nautilus_file_is_in_trash                       (NautilusFile                   *file);
_Bool                   nautilus_file_is_in_recent                      (NautilusFile                   *file);
_Bool                   nautilus_file_is_in_desktop                     (NautilusFile                   *file);
_Bool                   nautilus_file_is_home                           (NautilusFile                   *file);
_Bool                   nautilus_file_is_desktop_directory              (NautilusFile                   *file);
GError                 *nautilus_file_get_file_info_error               (NautilusFile                   *file);
_Bool                   nautilus_file_get_directory_item_count          (NautilusFile                   *file,
                                                                         unsigned int                   *count,
                                                                         _Bool                          *count_unreadable);
const char             *nautilus_file_peek_display_name                 (NautilusFile                   *file);
void                    nautilus_file_recompute_deep_counts             (NautilusFile                   *file);
NautilusRequestStatus   nautilus_file_get_deep_counts                   (NautilusFile                   *file,
                                                                         unsigned int                   *directory_count,
                                                                         unsigned int                   *file_count,
                                                                         unsigned int                   *unreadable_directory_count,
                                                                         unsigned int                   *hidden_count,
                                                                         goffset                        *total_size,
                                                                        _Bool                            force);
_Bool                   nautilus_file_should_show_thumbnail             (NautilusFile                   *file);
_Bool                   nautilus_file_should_show_directory_item_count  (NautilusFile                   *file);
_Bool                   nautilus_file_should_show_type                  (NautilusFile                   *file);
GList                  *nautilus_file_get_keywords                      (NautilusFile                   *file);
GList                  *nautilus_file_get_emblem_icons                  (NautilusFile                   *file,
                                                                         char                          **exclude);
char                   *nautilus_file_get_top_left_text                 (NautilusFile                   *file);
char                   *nautilus_file_peek_top_left_text                (NautilusFile                   *file,
                                                                         _Bool                           need_large_text,
                                                                         _Bool                          *got_top_left_text);
_Bool                   nautilus_file_get_directory_item_mime_types     (NautilusFile                   *file,
                                                                         GList                         **mime_list);

void                    nautilus_file_set_attributes                    (NautilusFile                   *file,
                                                                         GFileInfo                      *attributes,
                                                                         NautilusFileOperationCallback   callback,
                                                                         void                            *callback_data);
GFilesystemPreviewType  nautilus_file_get_filesystem_use_preview        (NautilusFile *file);

char                   *nautilus_file_get_filesystem_id                 (NautilusFile                   *file);

NautilusFile           *nautilus_file_get_trash_original_file           (NautilusFile                   *file);

/* Permissions. */
_Bool                   nautilus_file_can_get_permissions               (NautilusFile                   *file);
_Bool                   nautilus_file_can_set_permissions               (NautilusFile                   *file);
unsigned int            nautilus_file_get_permissions                   (NautilusFile                   *file);
_Bool                   nautilus_file_can_get_owner                     (NautilusFile                   *file);
_Bool                   nautilus_file_can_set_owner                     (NautilusFile                   *file);
_Bool                   nautilus_file_can_get_group                     (NautilusFile                   *file);
_Bool                   nautilus_file_can_set_group                     (NautilusFile                   *file);
char                   *nautilus_file_get_octal_permissions_as_string   (NautilusFile                   *file);
char                   *nautilus_file_get_permissions_as_string         (NautilusFile                   *file);
char                   *nautilus_file_get_owner_name                    (NautilusFile                   *file);
char                   *nautilus_file_get_group_name                    (NautilusFile                   *file);
GList                  *nautilus_get_user_names                         (void);
GList                  *nautilus_get_all_group_names                    (void);
GList                  *nautilus_file_get_settable_group_names          (NautilusFile                   *file);
_Bool                   nautilus_file_can_get_selinux_context           (NautilusFile                   *file);
char                   *nautilus_file_get_selinux_context               (NautilusFile                   *file);

/* "Capabilities". */
_Bool                   nautilus_file_can_read                          (NautilusFile                   *file);
_Bool                   nautilus_file_can_write                         (NautilusFile                   *file);
_Bool                   nautilus_file_can_execute                       (NautilusFile                   *file);
_Bool                   nautilus_file_can_rename                        (NautilusFile                   *file);
_Bool                   nautilus_file_can_delete                        (NautilusFile                   *file);
_Bool                   nautilus_file_can_trash                         (NautilusFile                   *file);

_Bool                   nautilus_file_can_mount                         (NautilusFile                   *file);
_Bool                   nautilus_file_can_unmount                       (NautilusFile                   *file);
_Bool                   nautilus_file_can_eject                         (NautilusFile                   *file);
_Bool                   nautilus_file_can_start                         (NautilusFile                   *file);
_Bool                   nautilus_file_can_start_degraded                (NautilusFile                   *file);
_Bool                   nautilus_file_can_stop                          (NautilusFile                   *file);
GDriveStartStopType     nautilus_file_get_start_stop_type               (NautilusFile                   *file);
_Bool                   nautilus_file_can_poll_for_media                (NautilusFile                   *file);
_Bool                   nautilus_file_is_media_check_automatic          (NautilusFile                   *file);

void                    nautilus_file_mount                             (NautilusFile                   *file,
                                                                         GMountOperation                *mount_op,
                                                                         GCancellable                   *cancellable,
                                                                         NautilusFileOperationCallback   callback,
                                                                         void                            *callback_data);
void                    nautilus_file_unmount                           (NautilusFile                   *file,
                                                                         GMountOperation                *mount_op,
                                                                         GCancellable                   *cancellable,
                                                                         NautilusFileOperationCallback   callback,
                                                                         void                            *callback_data);
void                    nautilus_file_eject                             (NautilusFile                   *file,
                                                                         GMountOperation                *mount_op,
                                                                         GCancellable                   *cancellable,
                                                                         NautilusFileOperationCallback   callback,
                                                                         void                            *callback_data);

void                    nautilus_file_start                             (NautilusFile                   *file,
                                                                         GMountOperation                *start_op,
                                                                         GCancellable                   *cancellable,
                                                                         NautilusFileOperationCallback   callback,
                                                                         void                            *callback_data);
void                    nautilus_file_stop                              (NautilusFile                   *file,
                                                                         GMountOperation                *mount_op,
                                                                         GCancellable                   *cancellable,
                                                                         NautilusFileOperationCallback   callback,
                                                                         void                            *callback_data);
void                    nautilus_file_poll_for_media                    (NautilusFile                   *file);

/* Basic operations for file objects. */
void                    nautilus_file_set_owner                         (NautilusFile                   *file,
                                                                         const char                     *user_name_or_id,
                                                                         NautilusFileOperationCallback   callback,
                                                                         void                            *callback_data);
void                    nautilus_file_set_group                         (NautilusFile                   *file,
                                                                         const char                     *group_name_or_id,
                                                                         NautilusFileOperationCallback   callback,
                                                                         void                            *callback_data);
void                    nautilus_file_set_permissions                   (NautilusFile                   *file,
                                                                         unsigned int                    permissions,
                                                                         NautilusFileOperationCallback   callback,
                                                                         void                            *callback_data);
void                    nautilus_file_rename                            (NautilusFile                   *file,
                                                                         const char                     *new_name,
                                                                         NautilusFileOperationCallback   callback,
                                                                         void                            *callback_data);
void                    nautilus_file_cancel                            (NautilusFile                   *file,
                                                                         NautilusFileOperationCallback   callback,
                                                                         void                            *callback_data);

/* Return true if this file has already been deleted.
 * This object will be unref'd after sending the files_removed signal,
 * but it could hang around longer if someone ref'd it.
 */
_Bool                   nautilus_file_is_gone                           (NautilusFile                   *file);

/* Return true if this file is not confirmed to have ever really
 * existed. This is true when the NautilusFile object has been created, but no I/O
 * has yet confirmed the existence of a file by that name.
 */
_Bool                   nautilus_file_is_not_yet_confirmed              (NautilusFile                   *file);

/* Simple getting and setting top-level metadata. */
char                   *nautilus_file_get_metadata                      (NautilusFile                   *file,
                                                                         const char                     *key,
                                                                         const char                     *default_metadata);
GList                  *nautilus_file_get_metadata_list                 (NautilusFile                   *file,
                                                                         const char                     *key);
void                    nautilus_file_set_metadata                      (NautilusFile                   *file,
                                                                         const char                     *key,
                                                                         const char                     *default_metadata,
                                                                         const char                     *metadata);
void                    nautilus_file_set_metadata_list                 (NautilusFile                   *file,
                                                                         const char                     *key,
                                                                         GList                          *list);

/* Covers for common data types. */
_Bool                   nautilus_file_get_boolean_metadata              (NautilusFile                   *file,
                                                                         const char                     *key,
                                                                         _Bool                           default_metadata);
void                    nautilus_file_set_boolean_metadata              (NautilusFile                   *file,
                                                                         const char                     *key,
                                                                         _Bool                           default_metadata,
                                                                         _Bool                           metadata);
int                     nautilus_file_get_integer_metadata              (NautilusFile                   *file,
                                                                         const char                     *key,
                                                                         int                             default_metadata);
void                    nautilus_file_set_integer_metadata              (NautilusFile                   *file,
                                                                         const char                     *key,
                                                                         int                             default_metadata,
                                                                         int                             metadata);

#define UNDEFINED_TIME ((time_t) (-1))

time_t                  nautilus_file_get_time_metadata                 (NautilusFile                  *file,
                                                                         const char                    *key);
void                    nautilus_file_set_time_metadata                 (NautilusFile                  *file,
                                                                         const char                    *key,
                                                                         time_t                         time);


/* Attributes for file objects as user-displayable strings. */
char                   *nautilus_file_get_string_attribute              (NautilusFile                   *file,
                                                                         const char                     *attribute_name);
char                   *nautilus_file_get_string_attribute_q            (NautilusFile                   *file,
                                                                         unsigned int                    attribute_q);
char                   *nautilus_file_get_string_attribute_with_default (NautilusFile                   *file,
                                                                         const char                     *attribute_name);
char                   *nautilus_file_get_string_attribute_with_default_q (NautilusFile                 *file,
                                                                         unsigned int                    attribute_q);
char                    *nautilus_file_fit_modified_date_as_string	      (NautilusFile 			  *file,
                                                                         int				   width,
                                                                         NautilusWidthMeasureCallback    measure_callback,
                                                                         NautilusTruncateCallback	   truncate_callback,
                                                                         void				  *measure_truncate_context);

/* Matching with another URI. */
_Bool                   nautilus_file_matches_uri                       (NautilusFile                   *file,
                                                                         const char                     *uri);

/* Is the file local? */
_Bool                   nautilus_file_is_local                          (NautilusFile                   *file);

/* Comparing two file objects for sorting */
NautilusFileSortType    nautilus_file_get_default_sort_type             (NautilusFile                   *file,
                                                                         _Bool                          *reversed);
const char             *nautilus_file_get_default_sort_attribute        (NautilusFile                   *file,
                                                                         _Bool                          *reversed);

int                     nautilus_file_compare_for_sort                  (NautilusFile                   *file_1,
                                                                         NautilusFile                   *file_2,
                                                                         NautilusFileSortType            sort_type,
                                                                         _Bool   			   directories_first,
                                                                         _Bool   		  	   reversed);
int                     nautilus_file_compare_for_sort_by_attribute     (NautilusFile                   *file_1,
                                                                         NautilusFile                   *file_2,
                                                                         const char                     *attribute,
                                                                         _Bool                           directories_first,
                                                                         _Bool                           reversed);
int                     nautilus_file_compare_for_sort_by_attribute_q   (NautilusFile                   *file_1,
                                                                         NautilusFile                   *file_2,
                                                                         unsigned int                    attribute,
                                                                         _Bool                           directories_first,
                                                                         _Bool                           reversed);
_Bool                   nautilus_file_is_date_sort_attribute_q          (unsigned int                    attribute);

int                     nautilus_file_compare_display_name              (NautilusFile                   *file_1,
                                                                         const char                     *pattern);
int                     nautilus_file_compare_location                  (NautilusFile                    *file_1,
                                                                         NautilusFile                    *file_2);

/* filtering functions for use by various directory views */
_Bool                   nautilus_file_is_hidden_file                    (NautilusFile                   *file);
_Bool                   nautilus_file_should_show                       (NautilusFile                   *file,
                                                                         _Bool                           show_hidden,
                                                                         _Bool                           show_foreign);
GList                  *nautilus_file_list_filter_hidden                (GList                          *files,
                                                                         _Bool                           show_hidden);


/* Get the URI that's used when activating the file.
 * Getting this can require reading the contents of the file.
 */
_Bool                   nautilus_file_is_launcher                       (NautilusFile                   *file);
_Bool                   nautilus_file_is_foreign_link                   (NautilusFile                   *file);
_Bool                   nautilus_file_is_trusted_link                   (NautilusFile                   *file);
_Bool                   nautilus_file_has_activation_uri                (NautilusFile                   *file);
char                   *nautilus_file_get_activation_uri                (NautilusFile                   *file);
GFile                  *nautilus_file_get_activation_location           (NautilusFile                   *file);

char                   *nautilus_file_get_drop_target_uri               (NautilusFile                   *file);

GIcon                  *nautilus_file_get_gicon                         (NautilusFile                   *file,
                                                                         NautilusFileIconFlags           flags);
NautilusIconInfo       *nautilus_file_get_icon                          (NautilusFile                   *file,
                                                                         int                             size,
                                                                         NautilusFileIconFlags           flags);
GdkPixbuf              *nautilus_file_get_icon_pixbuf                   (NautilusFile                   *file,
                                                                         int                             size,
                                                                        _Bool                            force_size,
                                                                         NautilusFileIconFlags           flags);

_Bool                   nautilus_file_has_open_window                   (NautilusFile                   *file);
void                    nautilus_file_set_has_open_window               (NautilusFile                   *file,
                                                                         _Bool                           has_open_window);

/* Thumbnailing handling */
_Bool                   nautilus_file_is_thumbnailing                   (NautilusFile                   *file);

/* Convenience functions for dealing with a list of NautilusFile objects that each have a ref.
 * These are just convenient names for functions that work on lists of GtkObject *.
 */
GList                  *nautilus_file_list_ref                            (GList                        *file_list);
void                    nautilus_file_list_unref                          (GList                        *file_list);
void                    nautilus_file_list_free                           (GList                        *file_list);
GList                  *nautilus_file_list_copy                           (GList                        *file_list);
GList                  *nautilus_file_list_from_uris                      (GList                        *uri_list);
GList                  *nautilus_file_list_sort_by_display_name           (GList                        *file_list);
void                    nautilus_file_list_call_when_ready                (GList                        *file_list,
                                                                           NautilusFileAttributes        attributes,
                                                                           NautilusFileListHandle      **handle,
                                                                           NautilusFileListCallback      callback,
                                                                           void                         *callback_data);
void                    nautilus_file_list_cancel_call_when_ready         (NautilusFileListHandle       *handle);

/* From Nautilus don't got yet */
char                   *nautilus_file_get_owner_as_string                 (NautilusFile                *file,
                                                                           _Bool                        include_real_name);
char                   *nautilus_file_get_type_as_string                  (NautilusFile                *file);
char                   *nautilus_file_get_detailed_type_as_string         (NautilusFile                *file);

char                   *nautilus_file_get_date_as_string                  (NautilusFile                *file,
                                                                           NautilusDateType date_type);

char                   *nautilus_file_construct_tooltip                   (NautilusFile                *file,
                                                                           NautilusFileIconFlags        flags);
char                   *nautilus_file_construct_treetip                   (NautilusFile                *file,
                                                                           NautilusFileIconFlags        flags);

/* Debugging */
void                    nautilus_file_dump                                (NautilusFile                 *file);

typedef struct NautilusFileDetails NautilusFileDetails;

struct NautilusFile {
	GObject parent_slot;
        unsigned int head_marker;            /* structure type signature */
	NautilusFileDetails *details;
};

typedef struct {
	GObjectClass parent_slot;

	/* Subclasses can set this to something other than G_FILE_TYPE_UNKNOWN and
	   it will be used as the default file type. This is useful when creating
	   a "virtual" NautilusFile subclass that you can't actually get real
	   information about. For exaple NautilusDesktopDirectoryFile. */
	GFileType default_file_type;

	/* Called when the file notices any change. */
	void                  (* changed)                (NautilusFile *file);

	/* Called periodically while directory deep count is being computed. */
	void                  (* updated_deep_count_in_progress) (NautilusFile *file);

	/* Virtual functions (mainly used for trash directory). */
	void                  (* monitor_add)            (NautilusFile           *file,
							  gconstpointer           client,
							  NautilusFileAttributes  attributes);
	void                  (* monitor_remove)         (NautilusFile           *file,
							  gconstpointer           client);
	void                  (* call_when_ready)        (NautilusFile           *file,
							  NautilusFileAttributes  attributes,
							  NautilusFileCallback    callback,
							  gpointer                callback_data);
	void                  (* cancel_call_when_ready) (NautilusFile           *file,
							  NautilusFileCallback    callback,
							  gpointer                callback_data);
	_Bool                 (* check_if_ready)         (NautilusFile           *file,
							  NautilusFileAttributes  attributes);
	_Bool                 (* get_item_count)         (NautilusFile           *file,
							  unsigned int           *count,
							  _Bool                  *count_unreadable);
	NautilusRequestStatus (* get_deep_counts)        (NautilusFile             *file,
                                                          unsigned int             *directory_count,
                                                          unsigned int             *file_count,
                                                          unsigned int             *unreadable_directory_count,
                                                          unsigned int             *hidden_count,
                                                          goffset                  *total_size);
	_Bool                 (* get_date)               (NautilusFile           *file,
							  NautilusDateType        type,
							  time_t                 *date);
	char *                (* get_where_string)       (NautilusFile           *file);

	void                  (* set_metadata)           (NautilusFile           *file,
							  const char             *key,
							  const char             *value);
	void                  (* set_metadata_as_list)   (NautilusFile           *file,
							  const char             *key,
							  char                  **value);

	void                  (* mount)                  (NautilusFile                   *file,
							  GMountOperation                *mount_op,
							  GCancellable                   *cancellable,
							  NautilusFileOperationCallback   callback,
							  void                            *callback_data);
	void                 (* unmount)                 (NautilusFile                   *file,
							  GMountOperation                *mount_op,
							  GCancellable                   *cancellable,
							  NautilusFileOperationCallback   callback,
							  void                            *callback_data);
	void                 (* eject)                   (NautilusFile                   *file,
							  GMountOperation                *mount_op,
							  GCancellable                   *cancellable,
							  NautilusFileOperationCallback   callback,
							  void                            *callback_data);

	void                  (* start)                  (NautilusFile                   *file,
							  GMountOperation                *start_op,
							  GCancellable                   *cancellable,
							  NautilusFileOperationCallback   callback,
							  void                            *callback_data);
	void                 (* stop)                    (NautilusFile                   *file,
							  GMountOperation                *mount_op,
							  GCancellable                   *cancellable,
							  NautilusFileOperationCallback   callback,
							  void                            *callback_data);

	void                 (* poll_for_media)          (NautilusFile                   *file);
} NautilusFileClass;

#endif /* NAUTILUS_FILE_H */
