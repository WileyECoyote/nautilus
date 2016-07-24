/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-global-preferences.h - Nautilus specific preference keys and
                                   functions.

   Copyright (C) 1999, 2000, 2001 Eazel, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

#ifndef NAUTILUS_GLOBAL_PREFERENCES_H
#define NAUTILUS_GLOBAL_PREFERENCES_H

/* #include <libnautilus-private/nautilus-global-preferences.h> */

/* Trash options */
#define NAUTILUS_PREFERENCES_CONFIRM_TRASH                      "confirm-trash"
#define NAUTILUS_PREFERENCES_ENABLE_DELETE                      "enable-delete"
#define NAUTILUS_PREFERENCES_SWAP_TRASH_DELETE                  "swap-trash-delete"

/* Desktop options */
#define NAUTILUS_PREFERENCES_DESKTOP_IS_HOME_DIR                "desktop-is-home-dir"

/* Display  */
#define NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES                  "show-hidden-files"
#define NAUTILUS_PREFERENCES_SHOW_ADVANCED_PERMISSIONS          "show-advanced-permissions"
#define NAUTILUS_PREFERENCES_DATE_FORMAT                        "date-format"
#define NAUTILUS_PREFERENCES_SHOW_WARN_ROOT                     "show-warn-root"

/* Mouse */
#define NAUTILUS_PREFERENCES_MOUSE_USE_EXTRA_BUTTONS            "mouse-use-extra-buttons"
#define NAUTILUS_PREFERENCES_MOUSE_FORWARD_BUTTON               "mouse-forward-button"
#define NAUTILUS_PREFERENCES_MOUSE_BACK_BUTTON                  "mouse-back-button"

/* Dbus options */
#define NAUTILUS_PREFERENCES_DBUS_SERVICE_TIMEOUT               "dbus-service-timeout"

typedef enum
{
	NAUTILUS_DATE_FORMAT_LOCALE,
	NAUTILUS_DATE_FORMAT_ISO,
	NAUTILUS_DATE_FORMAT_INFORMAL
} NautilusDateFormat;

typedef enum
{
	NAUTILUS_NEW_TAB_POSITION_AFTER_CURRENT_TAB,
	NAUTILUS_NEW_TAB_POSITION_END,
} NautilusNewTabPosition;

/* Sidebar panels  */
#define NAUTILUS_PREFERENCES_TREE_SHOW_ONLY_DIRECTORIES         "show-only-directories"
#define NAUTILUS_PREFERENCES_TREE_SHOW_LINES                    "sidebar-show-lines"
#define NAUTILUS_PREFERENCES_TREE_ENABLE_HOVER                  "sidebar-enable-hover"
#define NAUTILUS_PREFERENCES_TREE_FIXED_HEIGHT                  "sidebar-fixed-height"

/* Single/Double click preference  */
#define NAUTILUS_PREFERENCES_CLICK_POLICY                       "click-policy"

/* Activating executable text files */
#define NAUTILUS_PREFERENCES_EXECUTABLE_TEXT_ACTIVATION         "executable-text-activation"

/* Installing new packages when unknown mime type activated */
#define NAUTILUS_PREFERENCES_INSTALL_MIME_ACTIVATION            "install-mime-activation"

/* Spatial or browser mode */
#define NAUTILUS_PREFERENCES_ALWAYS_USE_BROWSER                 "always-use-browser"
#define NAUTILUS_PREFERENCES_NEW_TAB_POSITION                   "tabs-open-position"

#define NAUTILUS_PREFERENCES_SHOW_LOCATION_ENTRY                "show-location-entry"
#define NAUTILUS_PREFERENCES_SHOW_PATH_IN_PANE                  "show-path-in-pane"
#define NAUTILUS_PREFERENCES_SHOW_UP_ICON_TOOLBAR               "show-up-icon-toolbar"
#define NAUTILUS_PREFERENCES_SHOW_EDIT_ICON_TOOLBAR             "show-edit-icon-toolbar"
#define NAUTILUS_PREFERENCES_SHOW_RELOAD_ICON_TOOLBAR           "show-reload-icon-toolbar"
#define NAUTILUS_PREFERENCES_SHOW_HOME_ICON_TOOLBAR             "show-home-icon-toolbar"
#define NAUTILUS_PREFERENCES_SHOW_COMPUTER_ICON_TOOLBAR         "show-computer-icon-toolbar"
#define NAUTILUS_PREFERENCES_SHOW_CUT_TOOLBAR                   "show-cut-toolbar"
#define NAUTILUS_PREFERENCES_SHOW_COPY_TOOLBAR                  "show-copy-toolbar"
#define NAUTILUS_PREFERENCES_SHOW_PASTE_TOOLBAR                 "show-paste-toolbar"
#define NAUTILUS_PREFERENCES_SHOW_SEARCH_ICON_TOOLBAR           "show-search-icon-toolbar"
#define NAUTILUS_PREFERENCES_SHOW_NEW_FOLDER_ICON_TOOLBAR       "show-new-folder-icon-toolbar"
#define NAUTILUS_PREFERENCES_SHOW_LABEL_SEARCH_ICON_TOOLBAR     "show-label-search-icon-toolbar"


/* Which views should be displayed for new windows */
#define NAUTILUS_WINDOW_STATE_START_WITH_SIDEBAR                "start-with-sidebar"
#define NAUTILUS_WINDOW_STATE_START_WITH_STATUS_BAR             "start-with-status-bar"
#define NAUTILUS_WINDOW_STATE_START_WITH_TOOLBAR                "start-with-toolbar"
#define NAUTILUS_WINDOW_STATE_START_WITH_MENU_BAR               "start-with-menu-bar"
#define NAUTILUS_WINDOW_STATE_SIDE_PANE_VIEW                    "side-pane-view"
#define NAUTILUS_WINDOW_STATE_GEOMETRY                          "geometry"
#define NAUTILUS_WINDOW_STATE_MAXIMIZED                         "maximized"
#define NAUTILUS_WINDOW_STATE_SIDEBAR_WIDTH                     "sidebar-width"
#define NAUTILUS_WINDOW_STATE_MY_COMPUTER_EXPANDED              "my-computer-expanded"
#define NAUTILUS_WINDOW_STATE_DEVICES_EXPANDED                  "devices-expanded"
#define NAUTILUS_WINDOW_STATE_NETWORK_EXPANDED                  "network-expanded"

/* Sorting order */
#define NAUTILUS_PREFERENCES_SORT_DIRECTORIES_FIRST             "sort-directories-first"
#define NAUTILUS_PREFERENCES_DEFAULT_SORT_ORDER                 "default-sort-order"
#define NAUTILUS_PREFERENCES_DEFAULT_SORT_IN_REVERSE_ORDER      "default-sort-in-reverse-order"

/* The default folder viewer - one of the two enums below */
#define NAUTILUS_PREFERENCES_DEFAULT_FOLDER_VIEWER              "default-folder-viewer"

#define NAUTILUS_PREFERENCES_SHOW_FULL_PATH_TITLES              "show-full-path-titles"

#define NAUTILUS_PREFERENCES_CLOSE_DEVICE_VIEW_ON_EJECT         "close-device-view-on-device-eject"

#define NAUTILUS_PREFERENCES_START_WITH_DUAL_PANE               "start-with-dual-pane"
#define NAUTILUS_PREFERENCES_IGNORE_VIEW_METADATA               "ignore-view-metadata"
#define NAUTILUS_PREFERENCES_SHOW_BOOKMARKS_IN_TO_MENUS         "show-bookmarks-in-to-menus"
#define NAUTILUS_PREFERENCES_SHOW_PLACES_IN_TO_MENUS            "show-places-in-to-menus"

enum
{
  NAUTILUS_DEFAULT_FOLDER_VIEWER_ICON_VIEW,
  NAUTILUS_DEFAULT_FOLDER_VIEWER_COMPACT_VIEW,
  NAUTILUS_DEFAULT_FOLDER_VIEWER_LIST_VIEW,
  NAUTILUS_DEFAULT_FOLDER_VIEWER_OTHER
};

/* These IIDs are used by the preferences code and in nautilus-application.c */
#define NAUTILUS_ICON_VIEW_IID                                     "OAFIID:Nautilus_File_Manager_Icon_View"
#define NAUTILUS_COMPACT_VIEW_IID                                  "OAFIID:Nautilus_File_Manager_Compact_View"
#define NAUTILUS_LIST_VIEW_IID                                     "OAFIID:Nautilus_File_Manager_List_View"


/* Icon View */
#define NAUTILUS_PREFERENCES_ICON_VIEW_DEFAULT_ZOOM_LEVEL          "default-zoom-level"
#define NAUTILUS_PREFERENCES_ICON_VIEW_DEFAULT_USE_TIGHTER_LAYOUT  "default-use-tighter-layout"
#define NAUTILUS_PREFERENCES_ICON_VIEW_LABELS_BESIDE_ICONS         "labels-beside-icons"

/* Which text attributes appear beneath icon names */
#define NAUTILUS_PREFERENCES_ICON_VIEW_CAPTIONS                    "captions"

/* The default size for thumbnail icons */
#define NAUTILUS_PREFERENCES_ICON_VIEW_THUMBNAIL_SIZE              "thumbnail-size"

/* ellipsization preferences */
#define NAUTILUS_PREFERENCES_ICON_VIEW_TEXT_ELLIPSIS_LIMIT         "text-ellipsis-limit"
#define NAUTILUS_PREFERENCES_DESKTOP_TEXT_ELLIPSIS_LIMIT           "text-ellipsis-limit"

/* Compact View */
#define NAUTILUS_PREFERENCES_COMPACT_VIEW_DEFAULT_ZOOM_LEVEL       "default-zoom-level"
#define NAUTILUS_PREFERENCES_COMPACT_VIEW_ALL_COLUMNS_SAME_WIDTH   "all-columns-have-same-width"

/* List View */
#define NAUTILUS_PREFERENCES_LIST_VIEW_AUTO_RESIZE_COLUMNS         "auto-resize-columns"
#define NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_ZOOM_LEVEL          "default-zoom-level"
#define NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_VISIBLE_COLUMNS     "default-visible-columns"
#define NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_COLUMN_ORDER        "default-column-order"

enum
{
  NAUTILUS_CLICK_POLICY_SINGLE,
  NAUTILUS_CLICK_POLICY_DOUBLE
};

enum
{
  NAUTILUS_EXECUTABLE_TEXT_LAUNCH,
  NAUTILUS_EXECUTABLE_TEXT_DISPLAY,
  NAUTILUS_EXECUTABLE_TEXT_ASK
};

typedef enum
{
  NAUTILUS_SPEED_TRADEOFF_ALWAYS,
  NAUTILUS_SPEED_TRADEOFF_LOCAL_ONLY,
  NAUTILUS_SPEED_TRADEOFF_NEVER
} NautilusSpeedTradeoffValue;

#define NAUTILUS_PREFERENCES_SHOW_TEXT_IN_ICONS           "show-icon-text"
#define NAUTILUS_PREFERENCES_SHOW_DIRECTORY_ITEM_COUNTS   "show-directory-item-counts"
#define NAUTILUS_PREFERENCES_SHOW_IMAGE_FILE_THUMBNAILS   "show-image-thumbnails"
#define NAUTILUS_PREFERENCES_IMAGE_FILE_THUMBNAIL_LIMIT   "thumbnail-limit"

typedef enum
{
  NAUTILUS_COMPLEX_SEARCH_BAR,
  NAUTILUS_SIMPLE_SEARCH_BAR
} NautilusSearchBarMode;

#define NAUTILUS_PREFERENCES_DESKTOP_FONT                  "font"
#define NAUTILUS_PREFERENCES_DESKTOP_HOME_VISIBLE          "home-icon-visible"
#define NAUTILUS_PREFERENCES_DESKTOP_HOME_NAME             "home-icon-name"
#define NAUTILUS_PREFERENCES_DESKTOP_COMPUTER_VISIBLE      "computer-icon-visible"
#define NAUTILUS_PREFERENCES_DESKTOP_COMPUTER_NAME         "computer-icon-name"
#define NAUTILUS_PREFERENCES_DESKTOP_TRASH_VISIBLE         "trash-icon-visible"
#define NAUTILUS_PREFERENCES_DESKTOP_TRASH_NAME            "trash-icon-name"
#define NAUTILUS_PREFERENCES_DESKTOP_VOLUMES_VISIBLE       "volumes-visible"
#define NAUTILUS_PREFERENCES_DESKTOP_NETWORK_VISIBLE       "network-icon-visible"
#define NAUTILUS_PREFERENCES_DESKTOP_NETWORK_NAME          "network-icon-name"
#define NAUTILUS_PREFERENCES_DESKTOP_BACKGROUND_FADE       "background-fade"

/* bulk rename utility */
#define NAUTILUS_PREFERENCES_BULK_RENAME_TOOL              "bulk-rename-tool"

/* Lockdown */
#define NAUTILUS_PREFERENCES_LOCKDOWN_COMMAND_LINE         "disable-command-line"

/* Desktop background */
#define NAUTILUS_PREFERENCES_SHOW_DESKTOP                  "show-desktop-icons"

/* File size unit prefix */
#define NAUTILUS_PREFERENCES_SIZE_PREFIXES                 "size-prefixes"

/* Tooltips */
#define NAUTILUS_TOOLTIP_PREFERENCES_DESKTOP               "tooltips-on-desktop"
#define NAUTILUS_TOOLTIP_PREFERENCES_ICON_VIEW             "tooltips-in-icon-view"

#define NAUTILUS_TOOLTIP_PREFERENCES_LIST_VIEW             "tooltips-in-list-view"
#define NAUTILUS_TOOLTIP_PREFERENCES_LIST_TIME_OUT         "listtips-timeout"
#define NAUTILUS_TOOLTIP_PREFERENCES_FILE_TYPE             "tooltips-show-file-type"
#define NAUTILUS_TOOLTIP_PREFERENCES_MOD_DATE              "tooltips-show-mod-date"
#define NAUTILUS_TOOLTIP_PREFERENCES_ACCESS_DATE           "tooltips-show-access-date"
#define NAUTILUS_TOOLTIP_PREFERENCES_FULL_PATH             "tooltips-show-path"
#define NAUTILUS_TOOLTIP_PREFERENCES_PERMISSIONS           "tooltips-show-permissions"
#define NAUTILUS_TOOLTIP_PREFERENCES_OCTAL                 "tooltips-show-octal"

#define NAUTILUS_TOOLTIP_PREFERENCES_TREE_VIEW             "tooltips-in-tree-view"
#define NAUTILUS_TOOLTIP_PREFERENCES_TREE_TIME_OUT         "treetips-timeout"
#define NAUTILUS_TREETIP_PREFERENCES_FILE_TYPE             "treetips-show-file-type"
#define NAUTILUS_TREETIP_PREFERENCES_MOD_DATE              "treetips-show-mod-date"
#define NAUTILUS_TREETIP_PREFERENCES_ACCESS_DATE           "treetips-show-access-date"
#define NAUTILUS_TREETIP_PREFERENCES_PERMISSIONS           "treetips-show-permissions"
#define NAUTILUS_TREETIP_PREFERENCES_OCTAL                 "treetips-show-octal"

G_BEGIN_DECLS

void  nautilus_global_preferences_init                      (void);
int   nautilus_global_preferences_get_tooltip_flags         (void);
int   nautilus_global_preferences_get_treetip_flags         (void);

GSettings *nautilus_preferences;
GSettings *nautilus_icon_view_preferences;
GSettings *nautilus_list_view_preferences;
GSettings *nautilus_compact_view_preferences;
GSettings *nautilus_desktop_preferences;
GSettings *nautilus_tooltips_preferences;
GSettings *nautilus_tree_sidebar_preferences;
GSettings *nautilus_window_state;
GSettings *gnome_lockdown_preferences;
GSettings *gnome_background_preferences;

G_END_DECLS

#endif /* NAUTILUS_GLOBAL_PREFERENCES_H */
