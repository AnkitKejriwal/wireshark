/* progress_dlg.c
 * Routines for progress-bar (modal) dialog
 *
 * $Id: progress_dlg.c,v 1.15 2002/09/05 18:47:47 jmayer Exp $
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>
#include "gtkglobals.h"
#include "dlg_utils.h"
#include "progress_dlg.h"

#define	PROG_BAR_KEY	"progress_bar"

static gint delete_event_cb(GtkWidget *w, GdkEvent *event, gpointer data);
static void stop_cb(GtkWidget *w, gpointer data);

/*
 * Define the structure describing a progress dialog.
 */
struct progdlg {
	GtkWidget *dlg_w;	/* top-level window widget */
	GTimeVal   start_time;
	GtkLabel  *status_lb;
	GtkLabel  *elapsed_lb;
	GtkLabel  *time_left_lb;
	GtkLabel  *percentage_lb;
	gchar      title[200];
};

/*
 * Create and pop up the progress dialog; allocate a "progdlg_t"
 * and initialize it to contain all information the implementation
 * needs in order to manipulate the dialog, and return a pointer to
 * it.
 *
 * The first argument is the task to do, e.g. "Loading".
 * The second argument is the item to do, e.g. "capture.cap".
 * The third argument is the string to put in the "stop this operation" button.
 * The fourth argument is a pointer to a Boolean variable that will be
 *   set to TRUE if the user hits that button.
 *
 * XXX - provide a way to specify the progress in units, with the total
 * number of units specified as an argument when the progress dialog
 * is created; updates would be given in units, with the progress dialog
 * code computing the percentage, and the progress bar would have a
 * label "0" on the left and <total number of units> on the right, with
 * a label in the middle giving the number of units we've processed
 * so far.  This could be used when filtering packets, for example; we
 * wouldn't always use it, as we have no idea how many packets are to
 * be read.
 */
progdlg_t *
create_progress_dlg(const gchar *task_title, const gchar *item_title, const gchar *stop_title,
    gboolean *stop_flag)
{
	progdlg_t *dlg;
	GtkWidget *dlg_w, *main_vb, *title_lb, *status_lb, *elapsed_lb, *time_left_lb, *percentage_lb;
	GtkWidget *prog_bar, *bbox, *stop_bt;
	GtkWidget *static_vb, *tmp_lb, *main_hb, *dynamic_vb, *percentage_hb;
	gchar tmp[100];


	dlg = g_malloc(sizeof (progdlg_t));

	g_snprintf(dlg->title, sizeof(dlg->title), "%s: %s",
		task_title, item_title);

	dlg_w = dlg_window_new(dlg->title);
	gtk_window_set_modal(GTK_WINDOW(dlg_w), TRUE);

	/*
	 * Container for dialog widgets.
	 */
	main_vb = gtk_vbox_new(FALSE, 1);
	gtk_container_border_width(GTK_CONTAINER(main_vb), 5);
	gtk_container_add(GTK_CONTAINER(dlg_w), main_vb);


	/*
	 * Static labels (left dialog side, labels aligned to the right)
	 */
	static_vb = gtk_vbox_new(FALSE, 1);
	g_snprintf (tmp, sizeof(tmp), "%s:", task_title);
	tmp_lb = gtk_label_new(tmp);
	gtk_misc_set_alignment(GTK_MISC(tmp_lb), 1.0, 0.0);
	gtk_box_pack_start(GTK_BOX(static_vb), tmp_lb, FALSE, TRUE, 3);
	tmp_lb = gtk_label_new("Status:");
	gtk_misc_set_alignment(GTK_MISC(tmp_lb), 1.0, 0.0);
	gtk_box_pack_start(GTK_BOX(static_vb), tmp_lb, FALSE, TRUE, 3);
	tmp_lb = gtk_label_new("Elapsed Time:");
	gtk_misc_set_alignment(GTK_MISC(tmp_lb), 1.0, 0.0);
	gtk_box_pack_start(GTK_BOX(static_vb), tmp_lb, FALSE, TRUE, 3);
	tmp_lb = gtk_label_new("Time Left:");
	gtk_misc_set_alignment(GTK_MISC(tmp_lb), 1.0, 0.0);
	gtk_box_pack_start(GTK_BOX(static_vb), tmp_lb, FALSE, TRUE, 3);
	tmp_lb = gtk_label_new("Progress:");
	gtk_misc_set_alignment(GTK_MISC(tmp_lb), 1.0, 0.0);
	gtk_box_pack_start(GTK_BOX(static_vb), tmp_lb, FALSE, TRUE, 3);


	/*
	 * Dynamic labels (right dialog side, labels aligned to the left)
	 */
	dynamic_vb = gtk_vbox_new(FALSE, 1);

	/*
	 * Put the item_title here as a label indicating what we're
	 * doing; set its alignment and padding so it's aligned on the
	 * left.
	 */
	title_lb = gtk_label_new(item_title);
	gtk_box_pack_start(GTK_BOX(dynamic_vb), title_lb, FALSE, TRUE, 3);
	gtk_misc_set_alignment(GTK_MISC(title_lb), 0.0, 0.0);
	gtk_misc_set_padding(GTK_MISC(title_lb), 0.0, 0.0);

	/* same for "Status" */
	status_lb = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(dynamic_vb), status_lb, FALSE, TRUE, 3);
	gtk_misc_set_alignment(GTK_MISC(status_lb), 0.0, 0.0);
	gtk_misc_set_padding(GTK_MISC(status_lb), 0.0, 0.0);
	dlg->status_lb = (GtkLabel *) status_lb;

	/* same for "Elapsed Time" */
	elapsed_lb = gtk_label_new("00:00");
	gtk_box_pack_start(GTK_BOX(dynamic_vb), elapsed_lb, FALSE, TRUE, 3);
	gtk_misc_set_alignment(GTK_MISC(elapsed_lb), 0.0, 0.0);
	gtk_misc_set_padding(GTK_MISC(elapsed_lb), 0.0, 0.0);
	dlg->elapsed_lb = (GtkLabel *) elapsed_lb;

	/* same for "Time Left" */
	time_left_lb = gtk_label_new("--:--");
	gtk_box_pack_start(GTK_BOX(dynamic_vb), time_left_lb, FALSE, TRUE, 3);
	gtk_misc_set_alignment(GTK_MISC(time_left_lb), 0.0, 0.0);
	gtk_misc_set_padding(GTK_MISC(time_left_lb), 0.0, 0.0);
	dlg->time_left_lb = (GtkLabel *) time_left_lb;

	/*
	 * The progress bar (in its own horizontal box, including percentage value)
	 */
	percentage_hb = gtk_hbox_new(FALSE, 1);
	gtk_box_pack_start(GTK_BOX(dynamic_vb), percentage_hb, FALSE, TRUE, 3);

	prog_bar = gtk_progress_bar_new();
	gtk_progress_set_activity_mode(GTK_PROGRESS(prog_bar), FALSE);
	gtk_box_pack_start(GTK_BOX(percentage_hb), prog_bar, FALSE, TRUE, 3);

	percentage_lb = gtk_label_new("  0%");
	gtk_misc_set_alignment(GTK_MISC(percentage_lb), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(percentage_hb), percentage_lb, FALSE, TRUE, 3);
	dlg->percentage_lb = (GtkLabel *) percentage_lb;

	/*
	 * Attach a pointer to the progress bar widget to the top-level widget.
	 */
	gtk_object_set_data(GTK_OBJECT(dlg_w), PROG_BAR_KEY, prog_bar);

	/*
	 * Static and dynamic boxes are now complete
	 */
	main_hb = gtk_hbox_new(FALSE, 1);
	gtk_box_pack_start(GTK_BOX(main_hb), static_vb, FALSE, TRUE, 3);
	gtk_box_pack_start(GTK_BOX(main_hb), dynamic_vb, FALSE, TRUE, 3);
	gtk_box_pack_start(GTK_BOX(main_vb), main_hb, FALSE, TRUE, 3);

	/*
	 * Button row: cancel button.
	 * (We put it in an HButtonBox, even though there's only one
	 * of them, so that it doesn't expand to the width of the window.)
	 */
	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_container_add(GTK_CONTAINER(main_vb), bbox);

	/*
	 * Allow user to either click a "stop this operation" button, or
	 * the close button on the window, to stop an operation in
	 * progress.
	 */
	stop_bt = gtk_button_new_with_label(stop_title);
	gtk_box_pack_start(GTK_BOX (bbox), stop_bt, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(stop_bt), "clicked",
	    GTK_SIGNAL_FUNC(stop_cb), (gpointer) stop_flag);
	gtk_signal_connect(GTK_OBJECT(dlg_w), "delete_event",
	    GTK_SIGNAL_FUNC(delete_event_cb), (gpointer) stop_flag);
	GTK_WIDGET_SET_FLAGS(stop_bt, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(stop_bt);
	GTK_WIDGET_SET_FLAGS(stop_bt, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(stop_bt);

	gtk_widget_show_all(dlg_w);

	dlg->dlg_w = dlg_w;

	g_get_current_time(&dlg->start_time);

	return dlg;
}

progdlg_t *
delayed_create_progress_dlg(const gchar *task_title, const gchar *item_title,
			    const gchar *stop_title, gboolean *stop_flag,
			    const GTimeVal *start_time, gfloat progress)
{
  GTimeVal    time_now;
  gfloat      delta_time;
  gfloat      min_display;
  progdlg_t   *dlg;

#define INIT_DELAY          0.1 * 1e6
#define MIN_DISPLAY_DEFAULT 2.0 * 1e6

/* Create a progress dialog, but only if it's not likely to disappear
 * immediately, which can be disconcerting for the user.
 *
 * Arguments are as for create_progress_dlg(), plus:
 *
 * (a) A pointer to a GTimeVal structure which holds the time at which
 *     the caller started to process the data.
 * (b) The current progress as a real number between 0 and 1.
 */

  g_get_current_time(&time_now);

  /* Get the time elapsed since the caller started processing the data */

  delta_time = (time_now.tv_sec - start_time->tv_sec) * 1e6 +
    time_now.tv_usec - start_time->tv_usec;

  /* Do nothing for the first INIT_DELAY microseconds */

  if (delta_time < INIT_DELAY)
    return NULL;

  /* If we create the progress dialog we want it to be displayed for a
   * minimum of MIN_DISPLAY_DEFAULT microseconds.  However, if we
   * previously estimated that the progress dialog didn't need to be
   * created and the caller's processing is slowing down (perhaps due
   * to the action of the operating system's scheduler on a compute-
   * intensive task), we tail off the minimum display time such that
   * the progress dialog will always be created after
   * 2*MIN_DISPLAY_DEFAULT microseconds.
   */

  if (delta_time <= INIT_DELAY + MIN_DISPLAY_DEFAULT)
    min_display = MIN_DISPLAY_DEFAULT;
  else
    min_display = 2 * MIN_DISPLAY_DEFAULT - delta_time;
             /* = MIN_DISPLAY_DEFAULT - (delta_time - MIN_DISPLAY_DEFAULT) */

  /* Assuming the progress increases linearly, see if the progress
   * dialog would be displayed for at least min_display microseconds if
   * we created it now.
   */

  if (progress >= (delta_time / (delta_time + min_display)))
    return NULL;

  dlg = create_progress_dlg(task_title, item_title, stop_title, stop_flag);

  /* set dialog start_time to the start of processing, not box creation */
  dlg->start_time = *start_time;

  return dlg;
}

/*
 * Called when the dialog box is to be deleted.
 * Set the "stop" flag to TRUE, and return TRUE - we don't want the dialog
 * box deleted now, our caller will do so when they see that the
 * "stop" flag is TRUE and abort the operation.
 */
static gint
delete_event_cb(GtkWidget *w _U_, GdkEvent *event _U_, gpointer data)
{
	gboolean *stop_flag = (gboolean *) data;

	*stop_flag = TRUE;
	return TRUE;
}

/*
 * Called when the "stop this operation" button is clicked.
 * Set the "stop" flag to TRUE; we don't have to destroy the dialog
 * box, as our caller will do so when they see that the "stop" flag is
 * true and abort the operation.
 */
static void
stop_cb(GtkWidget *w _U_, gpointer data)
{
	gboolean *stop_flag = (gboolean *) data;

	*stop_flag = TRUE;
}

/*
 * Update the progress information of the progress dialog box.
 */
void
update_progress_dlg(progdlg_t *dlg, gfloat percentage, gchar *status)
{
	GtkWidget *dlg_w = dlg->dlg_w;
	GtkWidget *prog_bar;
	GTimeVal   time_now;
	gfloat     delta_time;
	gulong     ul_left;
	gulong     ul_elapsed;
	gulong     ul_percentage;
	gchar      tmp[100];


	/* calculate some timing values */
	g_get_current_time(&time_now);

	delta_time = (time_now.tv_sec - dlg->start_time.tv_sec) * 1e6 +
		time_now.tv_usec - dlg->start_time.tv_usec;
	ul_percentage = percentage * 100;
	ul_elapsed = delta_time / 1000 / 1000;

	/* update labels */
	g_snprintf(tmp, sizeof(tmp), "%lu%% of %s", ul_percentage, dlg->title);
	gtk_window_set_title(GTK_WINDOW(dlg_w), tmp);

	gtk_label_set_text(dlg->status_lb, status);

	g_snprintf(tmp, sizeof(tmp), "%lu%%", ul_percentage);
	gtk_label_set_text(dlg->percentage_lb, tmp);

	g_snprintf(tmp, sizeof(tmp), "%02lu:%02lu", ul_elapsed / 60, ul_elapsed % 60);
	gtk_label_set_text(dlg->elapsed_lb, tmp);

	/* show "Time Left" only,
	 * if at least 5% and 3 seconds running (to get a useful estimation) */
	if (ul_percentage >= 5 && delta_time >= 3 * 1e6) {
		ul_left = (delta_time / percentage - delta_time) / 1000 / 1000;

		g_snprintf(tmp, sizeof(tmp), "%02lu:%02lu", ul_left / 60, ul_left % 60);
		gtk_label_set_text(dlg->time_left_lb, tmp);
	}

	/* update progress bar */
	prog_bar = gtk_object_get_data(GTK_OBJECT(dlg_w), PROG_BAR_KEY);
	gtk_progress_bar_update(GTK_PROGRESS_BAR(prog_bar), percentage);

	/*
	 * Flush out the update and process any input events.
	 */
	while (gtk_events_pending())
		gtk_main_iteration();
}

/*
 * Destroy the progress dialog.
 */
void
destroy_progress_dlg(progdlg_t *dlg)
{
	GtkWidget *dlg_w = dlg->dlg_w;

	gtk_widget_destroy(GTK_WIDGET(dlg_w));
	g_free(dlg);
}
