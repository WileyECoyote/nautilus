/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

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
   Free Software Foundation, Inc., 51 Franklin Street - Suite 500,
   Boston, MA 02110-1335, USA.

*/

#ifndef NAUTILUS_ACTION_MANAGER_H
#define NAUTILUS_ACTION_MANAGER_H

#define NAUTILUS_TYPE_ACTION_MANAGER nautilus_action_manager_get_type()
#define NAUTILUS_ACTION_MANAGER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), NAUTILUS_TYPE_ACTION_MANAGER, NautilusActionManager))
#define NAUTILUS_ACTION_MANAGER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), NAUTILUS_TYPE_ACTION_MANAGER, NautilusActionManagerClass))
#define NAUTILUS_IS_ACTION_MANAGER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NAUTILUS_TYPE_ACTION_MANAGER))
#define NAUTILUS_IS_ACTION_MANAGER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), NAUTILUS_TYPE_ACTION_MANAGER))
#define NAUTILUS_ACTION_MANAGER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), NAUTILUS_TYPE_ACTION_MANAGER, NautilusActionManagerClass))

typedef struct _NautilusActionManager NautilusActionManager;
typedef struct _NautilusActionManagerClass NautilusActionManagerClass;

struct _NautilusActionManager {
    GObject parent;
    GList *actions;
    GList *actions_directory_list;
    _Bool action_list_dirty;
};

struct _NautilusActionManagerClass {
    GObjectClass parent_class;
    void (* changed) (NautilusActionManager *action_manager);
};

unsigned int             nautilus_action_manager_get_type            (void);
NautilusActionManager   *nautilus_action_manager_new                 (void);
GList                   *nautilus_action_manager_list_actions        (NautilusActionManager *action_manager);

#endif /* NAUTILUS_ACTION_MANAGER_H */
