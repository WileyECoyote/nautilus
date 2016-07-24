/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 *  nautilus-module.h - Interface to nautilus extensions
 *
 *  Copyright (C) 2003 Novell, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Dave Camp <dave@ximian.com>
 *
 */

#include <config.h>

#include <stdio.h>

#include <glib-object.h>

#include "nautilus-module.h"

#include <eel/eel-glib-macros.h>
#include <eel/eel-debug.h>
#include <gmodule.h>

#define NAUTILUS_TYPE_MODULE    	(nautilus_module_get_type ())
#define NAUTILUS_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), NAUTILUS_TYPE_MODULE, NautilusModule))
#define NAUTILUS_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), NAUTILUS_TYPE_MODULE, NautilusModule))
#define NAUTILUS_IS_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), NAUTILUS_TYPE_MODULE))
#define NAUTILUS_IS_MODULE_CLASS(klass)	(G_TYPE_CLASS_CHECK_CLASS_TYPE ((klass), NAUTILUS_TYPE_MODULE))

typedef struct _NautilusModule        NautilusModule;
typedef struct _NautilusModuleClass   NautilusModuleClass;

struct _NautilusModule {
    GTypeModule parent;

    GModule *library;

    char *path;

    void (*initialize) (GTypeModule  *module);
    void (*shutdown)   (void);

    void (*list_types) (const unsigned int **types,
                        int          *num_types);

};

struct _NautilusModuleClass {
    GTypeModuleClass parent;
};

static GList *module_objects = NULL;

static unsigned int nautilus_module_get_type (void);

G_DEFINE_TYPE (NautilusModule, nautilus_module, G_TYPE_TYPE_MODULE);
#define parent_class nautilus_module_parent_class

static _Bool
module_pulls_in_orbit (GModule *module)
{
    void *symbol;
    _Bool result;

    result = g_module_symbol (module, "ORBit_realloc_tcval", &symbol);

    return result;
}

static int
nautilus_module_load (GTypeModule *gmodule)
{
    NautilusModule *module;

    module = NAUTILUS_MODULE (gmodule);

    module->library = g_module_open (module->path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

    /* ORBit installs atexit() handlers, which would get unloaded together
     * with the module now that the main process doesn't depend on GConf anymore,
     * causing nautilus to sefgault at exit.
     * If we detect that an extension would pull in ORBit, we make the
     * module resident to prevent that.
     */
    if (module_pulls_in_orbit (module->library)) {
        g_module_make_resident (module->library);
    }

    if (!module->library) {
        g_warning ("%s", g_module_error ());
        return FALSE;
    }

    if (!g_module_symbol (module->library, "nautilus_module_initialize", (void **)&module->initialize) ||
        !g_module_symbol (module->library, "nautilus_module_shutdown", (void **)&module->shutdown) ||
        !g_module_symbol (module->library, "nautilus_module_list_types", (void **)&module->list_types))
    {
        g_warning ("%s", g_module_error ());
        g_module_close (module->library);

        return FALSE;
    }

    module->initialize (gmodule);

    return TRUE;
}

static void
nautilus_module_unload (GTypeModule *gmodule)
{
    NautilusModule *module;

    module = NAUTILUS_MODULE (gmodule);

    module->shutdown ();

    g_module_close (module->library);

    module->initialize = NULL;
    module->shutdown = NULL;
    module->list_types = NULL;
}

static void
nautilus_module_finalize (GObject *object)
{
    NautilusModule *module;

    module = NAUTILUS_MODULE (object);

    g_free (module->path);

    EEL_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
nautilus_module_init (NautilusModule *module)
{
}

static void
nautilus_module_class_init (NautilusModuleClass *class)
{
    G_OBJECT_CLASS (class)->finalize    = nautilus_module_finalize;
    G_TYPE_MODULE_CLASS (class)->load   = nautilus_module_load;
    G_TYPE_MODULE_CLASS (class)->unload = nautilus_module_unload;
}

static void
module_object_weak_notify (void *user_data, GObject *object)
{
    int len = g_list_length(module_objects);
#ifdef DEBUG
    fprintf(stderr, "%s user_data=%p removing provider=%p of %d\n", __func__, user_data, object, len);
#endif
    module_objects = g_list_remove (module_objects, object);
#ifdef DEBUG
    fprintf(stderr, "%s now there are %d providers\n", __func__, g_list_length(module_objects));
#endif
}

static void
add_module_objects (NautilusModule *module)
{
    const unsigned int *types;
    int num_types;
    int i;

    module->list_types (&types, &num_types);

    for (i = 0; i < num_types; i++) {
        if (types[i] == 0) { /* Work around broken extensions */
            break;
    }
    nautilus_module_add_type (types[i]);
}
}

static NautilusModule *
nautilus_module_load_file (const char *filename)
{
    NautilusModule *module;

    module = g_object_new (NAUTILUS_TYPE_MODULE, NULL);
    module->path = g_strdup (filename);

    if (g_type_module_use (G_TYPE_MODULE (module))) {
        add_module_objects (module);
        g_type_module_unuse (G_TYPE_MODULE (module));
        return module;
    } else {
        g_object_unref (module);
        return NULL;
    }
}

static void
load_module_dir (const char *dirname)
{
    GDir *dir;

    dir = g_dir_open (dirname, 0, NULL);

    if (dir) {
        const char *name;

        while ((name = g_dir_read_name (dir))) {
            if (g_str_has_suffix (name, ".so")) {
                char *filename;

                filename = g_build_filename (dirname, name, NULL);
                nautilus_module_load_file (filename);
                g_free (filename);
            }
        }

        g_dir_close (dir);
    }
}

static void
free_module_objects (void)
{
    GList *iter, *next;

    for (iter = module_objects; iter != NULL; iter = next) {
        next = iter->next;
        g_object_unref (iter->data);
    }

    g_list_free (module_objects);
}

void
nautilus_module_setup (void)
{
    static _Bool initialized = FALSE;

    if (!initialized) {
        initialized = TRUE;

        load_module_dir (NAUTILUS_EXTENSIONDIR);

        eel_debug_call_at_shutdown (free_module_objects);
    }
}

GList *
nautilus_module_get_extensions_for_type (unsigned int type)
{
    GList *iter;
    GList *ret_list = NULL;

    int i = 0;

#ifdef DEBUG
    fprintf(stderr, "%s begin looking for type=%u\n", __func__, type);
#endif

    for (iter = module_objects; iter != NULL; iter =iter->next) {
        if (G_TYPE_CHECK_INSTANCE_TYPE (G_OBJECT (iter->data), type)) {
            g_object_ref (iter->data);
            ret_list = g_list_prepend (ret_list, iter->data);
        }
        ++i;
    }
#ifdef DEBUG
    fprintf(stderr, "%s exit,returning %d type %u items\n",
            __func__, g_list_length(ret_list), type);
#endif
    return ret_list;
}

void
nautilus_module_extension_list_free (GList *extensions)
{
    GList *iter, *next;

    for (iter = extensions; iter != NULL; iter = next) {
        next = iter->next;
        g_object_unref (iter->data);
    }
    g_list_free (extensions);
}
#include <libnautilus-extension/nautilus-menu-provider.h>
void
nautilus_module_add_type (unsigned int type)
{
  GObject *object;

  if (type != 0) {
    object = g_object_new (type, NULL);
    g_object_weak_ref (object, (GWeakNotify)module_object_weak_notify, NULL);

    module_objects = g_list_prepend (module_objects, object);
  }
}
