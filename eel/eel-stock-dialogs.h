/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*!
 * \file eel-stock-dialogs.h: Various standard dialogs for Eel.
 *
 * Copyright (C) 2014 Wiley Edward Hill <wileyhill@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Darin Adler <darin@eazel.com>
 *          Wiley Edward Hill <wileyhill@gmail.com>
 *
 */

#ifndef EEL_STOCK_DIALOGS_H
#define EEL_STOCK_DIALOGS_H

#include <gtk/gtk.h>

typedef void (* EelCancelCallback) (gpointer callback_data);

/* Dialog for cancelling something that normally is fast enough not to need a dialog. */
void       eel_timed_wait_start               (EelCancelCallback  cancel_callback,
                                               void              *callback_data,
                                               const char        *wait_message,
                                               GtkWindow         *parent_window);
void       eel_timed_wait_start_with_duration (int                duration,
                                               EelCancelCallback  cancel_callback,
                                               void              *callback_data,
                                               const char        *wait_message,
                                               GtkWindow         *parent_window);
void       eel_timed_wait_stop                (EelCancelCallback  cancel_callback,
                                               void              *callback_data);

/* Basic dialog with buttons. */
int        eel_run_simple_dialog              (GtkWidget         *parent,
                                               _Bool              ignore_close_box,
                                               GtkMessageType     message_type,
                                               const char        *primary_text,
                                               const char        *secondary_text,
                                               ...);

/* Variations on gnome stock dialogs; these do line wrapping, we don't
 * bother with non-parented versions, we allow setting the title,
 * primary, and secondary messages, and we return GtkDialog pointers
 * instead of GtkWidget pointers. Both the title and the parent arguments
 * can be NULL.
 */
GtkDialog *eel_show_info_dialog               (const char        *primary_text,
                                               const char        *secondary_text,
                                               const char        *title,
                                               GtkWindow         *parent);
GtkDialog *eel_show_info_dialog_with_details  (const char        *primary_text,
                                               const char        *secondary_text,
                                               const char        *detailed_informative_message,
                                               const char        *title,
                                               GtkWindow         *parent);
GtkDialog *eel_show_warning_dialog            (const char        *primary_text,
                                               const char        *secondary_text,
                                               const char        *title,
                                               GtkWindow         *parent);
GtkDialog *eel_show_error_dialog              (const char        *primary_text,
                                               const char        *secondary_text,
                                               const char        *title,
                                               GtkWindow         *parent);
GtkDialog *eel_show_error_dialog_with_details (const char        *primary_text,
                                               const char        *secondary_text,
                                               const char        *detailed_error_message,
                                               const char        *title,
                                               GtkWindow         *parent);
GtkDialog *eel_create_question_dialog         (const char        *primary_text,
                                               const char        *secondary_text,
                                               const char        *answer_one,
                                               int                response_one,
                                               const char        *answer_two,
                                               int                response_two,
                                               const char        *title,
                                               GtkWindow         *parent);
GtkDialog *eel_show_yes_no_dialog             (const char        *primary_text,
                                               const char        *secondary_text,
                                               const char        *yes_label,
                                               const char        *no_label,
                                               const char        *title,
                                               GtkWindow         *parent);
GtkDialog *eel_create_info_dialog             (const char        *primary_text,
                                               const char        *secondary_text,
                                               const char        *title,
                                               GtkWindow         *parent);

#endif /* EEL_STOCK_DIALOGS_H */
