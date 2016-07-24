/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
   nautilus-job-queue.h - file operation queue

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

#ifndef __NAUTILUS_JOB_QUEUE_H__
#define __NAUTILUS_JOB_QUEUE_H__

#include <glib-object.h>

//#include <libnautilus-private/nautilus-progress-info.h>

#define NAUTILUS_TYPE_JOB_QUEUE             (nautilus_job_queue_get_type())
#define NAUTILUS_JOB_QUEUE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), NAUTILUS_TYPE_JOB_QUEUE, NautilusJobQueue))
#define NAUTILUS_JOB_QUEUE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), NAUTILUS_TYPE_JOB_QUEUE, NautilusJobQueueClass))
#define NAUTILUS_IS_JOB_QUEUE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NAUTILUS_TYPE_JOB_QUEUE))
#define NAUTILUS_IS_JOB_QUEUE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), NAUTILUS_TYPE_JOB_QUEUE))
#define NAUTILUS_JOB_QUEUE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), NAUTILUS_TYPE_JOB_QUEUE, NautilusJobQueueClass))

typedef struct _NautilusJobQueue NautilusJobQueue;
typedef struct _NautilusJobQueueClass NautilusJobQueueClass;
typedef struct _NautilusJobQueuePriv NautilusJobQueuePriv;

struct _NautilusJobQueue {
  GObject parent;

  /* private */
  NautilusJobQueuePriv *priv;
};

struct _NautilusJobQueueClass {
  GObjectClass parent_class;
};

GType nautilus_job_queue_get_type (void);

NautilusJobQueue *nautilus_job_queue_get (void);

void nautilus_job_queue_add_new_job (NautilusJobQueue     *self,
                                     GIOSchedulerJobFunc   job_func,
                                     void                 *user_data,
                                     GCancellable         *cancellable,
                                     NautilusProgressInfo *info,
                                     _Bool                 skip_queue);

void nautilus_job_queue_start_next_job    (NautilusJobQueue *self);

void nautilus_job_queue_start_job_by_info (NautilusJobQueue     *self,
                                           NautilusProgressInfo *info);

GList *nautilus_job_queue_get_all_jobs    (NautilusJobQueue *self);

G_END_DECLS

#endif /* __NAUTILUS_JOB_QUEUE_H__ */
