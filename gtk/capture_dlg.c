/* capture_dlg.c
 * Routines for packet capture windows
 *
 * $Id: capture_dlg.c,v 1.9 1999/11/06 02:05:32 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@zing.org>
 * Copyright 1998 Gerald Combs
 *
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

#ifdef HAVE_LIBPCAP

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <time.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#include <signal.h>
#include <errno.h>

#ifdef NEED_SNPRINTF_H
# ifdef HAVE_STDARG_H
#  include <stdarg.h>
# else
#  include <varargs.h>
# endif
# include "snprintf.h"
#endif

#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif

#include <wiretap/wtap.h>
#include "capture.h"
#include "globals.h"
#include "main.h"
#include "capture_dlg.h"
#include "prefs_dlg.h"
#include "util.h"

/* Capture callback data keys */
#define E_CAP_IFACE_KEY    "cap_iface"
#define E_CAP_FILT_KEY     "cap_filter_te"
#define E_CAP_FILE_TE_KEY  "cap_file_te"
#define E_CAP_COUNT_KEY    "cap_count"
#define E_CAP_SNAP_KEY     "cap_snap"
#define E_CAP_SYNC_KEY     "cap_sync"
#define E_CAP_RESOLVE_KEY  "cap_resolve"

static GList*
get_interface_list();

static void
capture_prep_file_cb(GtkWidget *w, gpointer te);

static void
cap_prep_fs_ok_cb(GtkWidget *w, gpointer data);

static void
cap_prep_fs_cancel_cb(GtkWidget *w, gpointer data);

static void
capture_prep_ok_cb(GtkWidget *ok_bt, gpointer parent_w);

static void
capture_prep_close_cb(GtkWidget *close_bt, gpointer parent_w);

static void
search_for_if_cb(gpointer data, gpointer user_data);

static void
free_if_cb(gpointer data, gpointer user_data);

void
capture_prep_cb(GtkWidget *w, gpointer d)
{
  GtkWidget     *cap_open_w, *if_cb, *if_lb,
                *count_lb, *count_cb, *main_vb, *if_hb, *count_hb,
                *filter_hb, *filter_bt, *filter_te,
                *file_hb, *file_bt, *file_te,
                *caplen_hb,
                *bbox, *ok_bt, *cancel_bt, *snap_lb,
                *snap_sb, *sync_cb, *resolv_cb;
  GtkAdjustment *adj;
  GList         *if_list, *count_list = NULL;
  gchar         *count_item1 = "0 (Infinite)", count_item2[16];

  if_list = get_interface_list();
  if (if_list == NULL)
    return;
  
  cap_open_w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(cap_open_w), "Ethereal: Capture Preferences");
  
  /* Container for each row of widgets */
  main_vb = gtk_vbox_new(FALSE, 3);
  gtk_container_border_width(GTK_CONTAINER(main_vb), 5);
  gtk_container_add(GTK_CONTAINER(cap_open_w), main_vb);
  gtk_widget_show(main_vb);
  
  /* Interface row */
  if_hb = gtk_hbox_new(FALSE, 3);
  gtk_container_add(GTK_CONTAINER(main_vb), if_hb);
  gtk_widget_show(if_hb);
  
  if_lb = gtk_label_new("Interface:");
  gtk_box_pack_start(GTK_BOX(if_hb), if_lb, FALSE, FALSE, 0);
  gtk_widget_show(if_lb);
  
  if_cb = gtk_combo_new();
  gtk_combo_set_popdown_strings(GTK_COMBO(if_cb), if_list);
  if (cf.iface)
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(if_cb)->entry), cf.iface);
  else if (if_list)
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(if_cb)->entry), if_list->data);
  gtk_box_pack_start(GTK_BOX(if_hb), if_cb, FALSE, FALSE, 0);
  gtk_widget_show(if_cb);
  
  while (if_list) {
    g_free(if_list->data);
    if_list = g_list_remove_link(if_list, if_list);
  }

  /* Count row */
  count_hb = gtk_hbox_new(FALSE, 3);
  gtk_container_add(GTK_CONTAINER(main_vb), count_hb);
  gtk_widget_show(count_hb);
  
  count_lb = gtk_label_new("Count:");
  gtk_box_pack_start(GTK_BOX(count_hb), count_lb, FALSE, FALSE, 0);
  gtk_widget_show(count_lb);
  
  count_list = g_list_append(count_list, count_item1);
  if (cf.count) {
    snprintf(count_item2, 15, "%d", cf.count);
    count_list = g_list_append(count_list, count_item2);
  }

  count_cb = gtk_combo_new();
  gtk_combo_set_popdown_strings(GTK_COMBO(count_cb), count_list);
  gtk_box_pack_start(GTK_BOX(count_hb), count_cb, FALSE, FALSE, 0);
  gtk_widget_show(count_cb);

  while (count_list)
    count_list = g_list_remove_link(count_list, count_list);

  /* Filter row */
  filter_hb = gtk_hbox_new(FALSE, 3);
  gtk_container_add(GTK_CONTAINER(main_vb), filter_hb);
  gtk_widget_show(filter_hb);
  
  filter_bt = gtk_button_new_with_label("Filter:");
  gtk_signal_connect(GTK_OBJECT(filter_bt), "clicked",
    GTK_SIGNAL_FUNC(prefs_cb), (gpointer) E_PR_PG_FILTER);
  gtk_box_pack_start(GTK_BOX(filter_hb), filter_bt, FALSE, TRUE, 0);
  gtk_widget_show(filter_bt);
  
  filter_te = gtk_entry_new();
  if (cf.cfilter) gtk_entry_set_text(GTK_ENTRY(filter_te), cf.cfilter);
  gtk_object_set_data(GTK_OBJECT(filter_bt), E_FILT_TE_PTR_KEY, filter_te);
  gtk_box_pack_start(GTK_BOX(filter_hb), filter_te, TRUE, TRUE, 0);
  gtk_widget_show(filter_te);
  
  /* File row */
  file_hb = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(main_vb), file_hb);
  gtk_widget_show(file_hb);
  
  file_bt = gtk_button_new_with_label("File:");
  gtk_box_pack_start(GTK_BOX(file_hb), file_bt, FALSE, FALSE, 3);
  gtk_widget_show(file_bt);
  
  file_te = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(file_hb), file_te, TRUE, TRUE, 3);
  gtk_widget_show(file_te);

  gtk_signal_connect(GTK_OBJECT(file_bt), "clicked",
    GTK_SIGNAL_FUNC(capture_prep_file_cb), GTK_OBJECT(file_te));

  /* Misc row: Capture file checkbox and snap spinbutton */
  caplen_hb = gtk_hbox_new(FALSE, 3);
  gtk_container_add(GTK_CONTAINER(main_vb), caplen_hb);
  gtk_widget_show(caplen_hb);

  snap_lb = gtk_label_new("Capture length");
  gtk_misc_set_alignment(GTK_MISC(snap_lb), 0, 0.5);
  gtk_box_pack_start(GTK_BOX(caplen_hb), snap_lb, FALSE, FALSE, 6);
  gtk_widget_show(snap_lb);

  adj = (GtkAdjustment *) gtk_adjustment_new((float) cf.snap,
    MIN_PACKET_SIZE, WTAP_MAX_PACKET_SIZE, 1.0, 10.0, 0.0);
  snap_sb = gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (snap_sb), TRUE);
  gtk_widget_set_usize (snap_sb, 80, 0);
  gtk_box_pack_start (GTK_BOX(caplen_hb), snap_sb, FALSE, FALSE, 3); 
  gtk_widget_show(snap_sb);
  
  sync_cb = gtk_check_button_new_with_label("Update list of packets in real time");
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(sync_cb), sync_mode);
  gtk_container_add(GTK_CONTAINER(main_vb), sync_cb);
  gtk_widget_show(sync_cb);

  resolv_cb = gtk_check_button_new_with_label("Enable name resolution");
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(resolv_cb), g_resolving_actif);
  gtk_container_add(GTK_CONTAINER(main_vb), resolv_cb);
  gtk_widget_show(resolv_cb);
  
  /* Button row: OK and cancel buttons */
  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
  gtk_container_add(GTK_CONTAINER(main_vb), bbox);
  gtk_widget_show(bbox);

  ok_bt = gtk_button_new_with_label ("OK");
  gtk_signal_connect(GTK_OBJECT(ok_bt), "clicked",
    GTK_SIGNAL_FUNC(capture_prep_ok_cb), GTK_OBJECT(cap_open_w));
  GTK_WIDGET_SET_FLAGS(ok_bt, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (bbox), ok_bt, TRUE, TRUE, 0);
  gtk_widget_grab_default(ok_bt);
  gtk_widget_show(ok_bt);

  cancel_bt = gtk_button_new_with_label ("Cancel");
  gtk_signal_connect(GTK_OBJECT(cancel_bt), "clicked",
    GTK_SIGNAL_FUNC(capture_prep_close_cb), GTK_OBJECT(cap_open_w));
  GTK_WIDGET_SET_FLAGS(cancel_bt, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (bbox), cancel_bt, TRUE, TRUE, 0);
  gtk_widget_show(cancel_bt);

  /* Attach pointers to needed widgets to the capture prefs window/object */
  gtk_object_set_data(GTK_OBJECT(cap_open_w), E_CAP_IFACE_KEY, if_cb);
  gtk_object_set_data(GTK_OBJECT(cap_open_w), E_CAP_FILT_KEY,  filter_te);
  gtk_object_set_data(GTK_OBJECT(cap_open_w), E_CAP_FILE_TE_KEY,  file_te);
  gtk_object_set_data(GTK_OBJECT(cap_open_w), E_CAP_COUNT_KEY, count_cb);
  gtk_object_set_data(GTK_OBJECT(cap_open_w), E_CAP_SNAP_KEY,  snap_sb);
  gtk_object_set_data(GTK_OBJECT(cap_open_w), E_CAP_SYNC_KEY,  sync_cb);
  gtk_object_set_data(GTK_OBJECT(cap_open_w), E_CAP_RESOLVE_KEY,  resolv_cb);

  gtk_widget_show(cap_open_w);
}

static void
capture_prep_file_cb(GtkWidget *w, gpointer file_te)
{
  GtkWidget *fs;

  fs = gtk_file_selection_new ("Ethereal: Capture File");

  gtk_object_set_data(GTK_OBJECT(fs), E_CAP_FILE_TE_KEY, file_te);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(fs)->ok_button),
    "clicked", (GtkSignalFunc) cap_prep_fs_ok_cb, fs);

  /* Connect the cancel_button to destroy the widget */
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(fs)->cancel_button),
    "clicked", (GtkSignalFunc) cap_prep_fs_cancel_cb, fs);
  
  gtk_widget_show(fs);
}

static void
cap_prep_fs_ok_cb(GtkWidget *w, gpointer data)
{
  gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(data),
      E_CAP_FILE_TE_KEY)),
      gtk_file_selection_get_filename (GTK_FILE_SELECTION(data)));
  gtk_widget_destroy(GTK_WIDGET(data));
}

static void
cap_prep_fs_cancel_cb(GtkWidget *w, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(data));
}  

static void
capture_prep_ok_cb(GtkWidget *ok_bt, gpointer parent_w) {
  GtkWidget *if_cb, *filter_te, *file_te, *count_cb, *snap_sb, *sync_cb,
            *resolv_cb;
  gchar *filter_text;
  gchar *save_file;

  if_cb     = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(parent_w), E_CAP_IFACE_KEY);
  filter_te = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(parent_w), E_CAP_FILT_KEY);
  file_te   = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(parent_w), E_CAP_FILE_TE_KEY);
  count_cb  = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(parent_w), E_CAP_COUNT_KEY);
  snap_sb   = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(parent_w), E_CAP_SNAP_KEY);
  sync_cb   = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(parent_w), E_CAP_SYNC_KEY);
  resolv_cb = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(parent_w), E_CAP_RESOLVE_KEY);

  if (cf.iface)
    g_free(cf.iface);
  cf.iface =
    g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(if_cb)->entry)));

  filter_text = gtk_entry_get_text(GTK_ENTRY(filter_te));
  if (cf.cfilter)
    g_free(cf.cfilter);
  cf.cfilter = NULL; /* ead 06/16/99 */
  if (filter_text && filter_text[0]) {
    cf.cfilter = g_strdup(filter_text); 
  }

  save_file = gtk_entry_get_text(GTK_ENTRY(file_te));
  if (save_file && save_file[0]) {
    /* User specified a file to which the capture should be written. */
    save_file = g_strdup(save_file);
  } else {
    /* User didn't specify a file; save to a temporary file. */
    save_file = NULL;
  }

  cf.count = atoi(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(count_cb)->entry)));

  cf.snap = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(snap_sb));
  if (cf.snap < 1)
    cf.snap = WTAP_MAX_PACKET_SIZE;
  else if (cf.snap < MIN_PACKET_SIZE)
    cf.snap = MIN_PACKET_SIZE;

  sync_mode = GTK_TOGGLE_BUTTON (sync_cb)->active;

  g_resolving_actif = GTK_TOGGLE_BUTTON (resolv_cb)->active;

  gtk_widget_destroy(GTK_WIDGET(parent_w));

  do_capture(save_file);
}

static void
capture_prep_close_cb(GtkWidget *close_bt, gpointer parent_w)
{
  gtk_grab_remove(GTK_WIDGET(parent_w));
  gtk_widget_destroy(GTK_WIDGET(parent_w));
}

struct search_user_data {
	char	*name;
	int	found;
};

static GList *
get_interface_list() {
  GList  *il = NULL;
  gint    nonloopback_pos = 0;
  struct  ifreq *ifr, *last;
  struct  ifconf ifc;
  struct  ifreq ifrflags;
  int     sock = socket(AF_INET, SOCK_DGRAM, 0);
  struct search_user_data user_data;
  pcap_t *pch;
  gchar   err_str[PCAP_ERRBUF_SIZE];

  if (sock < 0)
  {
    simple_dialog(ESD_TYPE_WARN, NULL,
      "Can't list interfaces: error opening socket.");
    return NULL;
  }

  /* Since we have to grab the interface list all at once, we'll make
     plenty of room */
  ifc.ifc_len = 1024 * sizeof(struct ifreq);
  ifc.ifc_buf = malloc(ifc.ifc_len);

  if (ioctl(sock, SIOCGIFCONF, &ifc) < 0 ||
    ifc.ifc_len < sizeof(struct ifreq))
  {
    simple_dialog(ESD_TYPE_WARN, NULL,
      "Can't list interfaces: SIOCGIFCONF error: %s", strerror(errno));
    goto fail;
  }

  ifr  = (struct ifreq *) ifc.ifc_req;
  last = (struct ifreq *) ((char *) ifr + ifc.ifc_len);
  while (ifr < last)
  {
    /*
     * Skip addresses that begin with "dummy", or that include a ":"
     * (the latter are Solaris virtuals).
     */
    if (strncmp(ifr->ifr_name, "dummy", 5) == 0 ||
	strchr(ifr->ifr_name, ':') != NULL)
      goto next;

    /*
     * If we already have this interface name on the list, don't add
     * it (SIOCGIFCONF returns, at least on BSD-flavored systems, one
     * entry per interface *address*; if an interface has multiple
     * addresses, we get multiple entries for it).
     */
    user_data.name = ifr->ifr_name;
    user_data.found = FALSE;
    g_list_foreach(il, search_for_if_cb, &user_data);
    if (user_data.found)
      goto next;

    /*
     * Get the interface flags.
     */
    memset(&ifrflags, 0, sizeof ifrflags);
    strncpy(ifrflags.ifr_name, ifr->ifr_name, sizeof ifrflags.ifr_name);
    if (ioctl(sock, SIOCGIFFLAGS, (char *)&ifrflags) < 0) {
      if (errno == ENXIO)
        goto next;
      simple_dialog(ESD_TYPE_WARN, NULL,
        "Can't list interfaces: SIOCGIFFLAGS error on %s: %s",
        ifr->ifr_name, strerror(errno));
      goto fail;
    }

    /*
     * Skip interfaces that aren't up.
     */
    if (!(ifrflags.ifr_flags & IFF_UP))
      goto next;

    /*
     * Skip interfaces that we can't open with "libpcap".
     */
    pch = pcap_open_live(ifr->ifr_name, WTAP_MAX_PACKET_SIZE, 0, 0, err_str);
    if (pch == NULL)
      goto next;
    pcap_close(pch);

    /*
     * If it's a loopback interface, add it at the end of the list,
     * otherwise add it after the last non-loopback interface,
     * so all loopback interfaces go at the end - we don't want a
     * loopback interface to be the default capture device unless there
     * are no non-loopback devices.
     */
    if ((ifrflags.ifr_flags & IFF_LOOPBACK) ||
	strncmp(ifr->ifr_name, "lo", 2) == 0)
      il = g_list_insert(il, g_strdup(ifr->ifr_name), -1);
    else {
      il = g_list_insert(il, g_strdup(ifr->ifr_name), nonloopback_pos);
      /* Insert the next non-loopback interface after this one. */
      nonloopback_pos++;
    }

next:
#ifdef HAVE_SA_LEN
    ifr = (struct ifreq *) ((char *) ifr + ifr->ifr_addr.sa_len + IFNAMSIZ);
#else
    ifr = (struct ifreq *) ((char *) ifr + sizeof(struct ifreq));
#endif
  }

  free(ifc.ifc_buf);
  close(sock);

  if (il == NULL) {
    simple_dialog(ESD_TYPE_WARN, NULL,
      "There are no network interfaces that can be opened.\n"
      "Please check to make sure you have sufficient permission\n"
      "to capture packets.");
    return NULL;
  }

  return il;

fail:
  if (il != NULL) {
    g_list_foreach(il, free_if_cb, NULL);
    g_list_free(il);
  }
  free(ifc.ifc_buf);
  close(sock);
  return NULL;
}

static void
search_for_if_cb(gpointer data, gpointer user_data)
{
	struct search_user_data *search_user_data = user_data;

	if (strcmp((char *)data, search_user_data->name) == 0)
		search_user_data->found = TRUE;
}

static void
free_if_cb(gpointer data, gpointer user_data)
{
	g_free(data);
}

#endif /* HAVE_LIBPCAP */
