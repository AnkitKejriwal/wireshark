/* simple_dialog.h
 * Definitions for alert box routines with toolkit-independent APIs but
 * toolkit-dependent implementations.
 *
 * $Id: simple_dialog.h,v 1.13 2004/05/01 22:55:22 obiot Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __DIALOG_H__
#define __DIALOG_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Dialog type.

   INFO: tells the user something they should know, but not requiring
   any action; the only button should be "OK".

   WARN: tells the user about a problem; the only button should be "OK"

   CONFIRMATION: asks the user for confirmation; there should be more than
   one button
   
   ERROR: tells the user about a serious problem; the only button should be
   "OK". */
#define ESD_TYPE_INFO	      0x00
#define ESD_TYPE_WARN	      0x01
#define ESD_TYPE_CONFIRMATION 0x02
#define ESD_TYPE_ERROR	      0x03

/* Which buttons to display. */
#define ESD_BTN_OK     0x01
#define ESD_BTN_CANCEL 0x02
#define ESD_BTN_YES    0x04
#define ESD_BTN_NO     0x08
#define ESD_BTN_CLEAR  0x10

#define ESD_BTNS_OK_CANCEL	(ESD_BTN_OK|ESD_BTN_CANCEL)
#define ESD_BTNS_YES_NO		(ESD_BTN_YES|ESD_BTN_NO)
#define ESD_BTNS_YES_NO_CANCEL	(ESD_BTN_YES|ESD_BTN_NO|ESD_BTN_CANCEL)

/* show a simple dialog */
#if __GNUC__ >= 2
extern gpointer simple_dialog(gint type, gint btn_mask,
    const gchar *msg_format, ...)
    __attribute__((format (printf, 3, 4)));
extern gpointer vsimple_dialog(gint type, gint btn_mask,
   const gchar *msg_format, va_list ap);
#else
extern gpointer simple_dialog(gint type, gint btn_mask,
    const gchar *msg_format, ...);
extern gpointer vsimple_dialog(gint type, gint btn_mask,
    const gchar *msg_format, va_list ap);
#endif

/* callback function type */
typedef void (* simple_dialog_cb_t) (gpointer dialog, gint btn, gpointer data);

/* set the callback function, which has to be called when a button was pressed */
extern void simple_dialog_set_cb(gpointer dialog, simple_dialog_cb_t callback_fct, gpointer data);

extern char *simple_dialog_primary_start(void);
extern char *simple_dialog_format_message(const char *msg);
extern char *simple_dialog_primary_end(void);

/*
 * If a routine is called to display a dialog before there are any windows
 * open, information to use to display the dialog is queued up.  This
 * routine should be called once there are windows open, so that the queued
 * up dialogs are displayed on top of those windows.
 */
extern void display_queued_messages(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DIALOG_H__ */
