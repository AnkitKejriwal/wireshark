/* win32-file-dlg.c
 * Native Windows file dialog routines
 *
 * $Id$
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 2004 Gerald Combs
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

#include "globals.h"

#include <glib.h>

#include <stdio.h>

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <richedit.h>

#include <stdlib.h>
#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>

#include "alert_box.h"
#include "color.h"
#include "color_filters.h"
#include "epan/filesystem.h"
#include "epan/addr_resolv.h"
#include "merge.h"
#include "epan/prefs.h"
#include "print.h"
#include "simple_dialog.h"
#include "util.h"

// XXX - declarations from gtk/file_dlg.h.  These should be moved
// somewhere less GTK-specific.
/** the action to take, after save has been done */
typedef enum {
    after_save_no_action,           /**< no action to take */
    after_save_close_file,          /**< close the file */
    after_save_open_dialog,         /**< open the file open dialog */
    after_save_open_recent_file,    /**< open the specified recent file */
    after_save_open_dnd_file,       /**< open the specified file from drag and drop */
    after_save_merge_dialog,        /**< open the file merge dialog */
    after_save_capture_dialog,      /**< open the capture dialog */
    after_save_exit                 /**< exit program */
} action_after_save_e;

#include "win32-file-dlg.h"

// XXX - declarations from gtk/dlg_utils.h.  These should be moved
// somewhere less GTK-specific.
extern void set_last_open_dir(char *dirname);

// XXX - declarations from gtk/capture_dlg.h.  These should be moved
// somewhere less GTK-specific.
/* capture start confirmed by "Save unsaved capture", so do it now */
void capture_start_confirmed(void);

// gtk/main.h:
extern gboolean main_do_quit(void);




typedef enum {
    merge_append,
    merge_chrono,
    merge_prepend
} merge_action_e;

#define FILE_TYPE_LIST 	\
    "Accellent 5Views (*.5vw)\0"		"*.5vw\0"               \
    "Ethereal/tcpdump (*.cap, *.pcap)\0"	"*.cap;*.pcap\0"        \
    "Novell LANalyzer (*.tr1)\0"		"*.tr1\0"               \
    "NG/NAI Sniffer (*.cap, *.enc, *.trc)\0"	"*.cap;*.enc;*.trc\0"   \
    "Sun snoop (*.snoop)\0"			"*.snoop\0"             \
    "WildPackets EtherPeek (*.pkt)\0"		"*.pkt\0"               \
    "All Files (*.*)\0"				"*.*\0"                 \
    "\0"

#define FILE_TYPE_DEFAULT 2 /* Ethereal/tcpdump */


static UINT CALLBACK open_file_hook_proc(HWND of_hwnd, UINT ui_msg, WPARAM w_param, LPARAM l_param);
static UINT CALLBACK save_as_file_hook_proc(HWND of_hwnd, UINT ui_msg, WPARAM w_param, LPARAM l_param);
static UINT CALLBACK merge_file_hook_proc(HWND mf_hwnd, UINT ui_msg, WPARAM w_param, LPARAM l_param);
static UINT CALLBACK export_file_hook_proc(HWND of_hwnd, UINT ui_msg, WPARAM w_param, LPARAM l_param);
static UINT CALLBACK export_raw_file_hook_proc(HWND of_hwnd, UINT ui_msg, WPARAM w_param, LPARAM l_param);
static void range_update_dynamics(HWND sf_hwnd, packet_range_t *range);
static void range_handle_wm_initdialog(HWND dlg_hwnd, packet_range_t *range);
static void range_handle_wm_command(HWND dlg_hwnd, HWND ctrl, WPARAM w_param, packet_range_t *range);

static gboolean       initialized = FALSE;
static int            filetype;
static packet_range_t range;
static merge_action_e merge_action;
static print_args_t   print_args;
/* XXX - The reason g_sf_hwnd exists is so that we can call
 *       file_set_save_marked_sensitive() from anywhere.  However, the
 *       save file dialog hogs the foreground, so this may not be
 *       necessary.
 */
static HWND           g_sf_hwnd = NULL;

static void
win32_file_init() {
    INITCOMMONCONTROLSEX   comm_ctrl;

    if (initialized)
        return;
    initialized = TRUE;

    /* Initialize our controls. */
    memset (&comm_ctrl, 0, sizeof(comm_ctrl));
    comm_ctrl.dwSize = sizeof(comm_ctrl);
    /* Includes the animate, header, hot key, list view, progress bar,
     * status bar, tab, tooltip, toolbar, trackbar, tree view, and
     * up-down controls
     */
    comm_ctrl.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&comm_ctrl);

    /* RichEd20.DLL is needed for filter entries. */
    LoadLibrary("riched20.dll");
}

gboolean
win32_open_file (HWND h_wnd) {
    static OPENFILENAME ofn;
    gchar  file_name[MAX_PATH] = "";
    int    err;
    char *dirname;

    win32_file_init();

    /* XXX - Check for version and set OPENFILENAME_SIZE_VERSION_400
       where appropriate */
    ZeroMemory(&ofn, sizeof(ofn));
#ifdef OPENFILENAME_SIZE_VERSION_400
    of.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#else
    ofn.lStructSize = sizeof(ofn);
#endif
    ofn.hwndOwner = h_wnd;
    ofn.hInstance = (HINSTANCE) GetWindowLong(h_wnd, GWL_HINSTANCE);
    /* XXX - Grab the rest of the extension list from ethereal.nsi. */
    ofn.lpstrFilter = FILE_TYPE_LIST;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = FILE_TYPE_DEFAULT;
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    if (prefs.gui_fileopen_style == FO_STYLE_SPECIFIED && prefs.gui_fileopen_dir[0] != '\0') {
	ofn.lpstrInitialDir = prefs.gui_fileopen_dir;
    } else {
	ofn.lpstrInitialDir = NULL;
    }
    ofn.lpstrTitle = "Ethereal: Select a capture file";
    ofn.Flags = OFN_ENABLESIZING | OFN_ENABLETEMPLATE | OFN_EXPLORER |
	    OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
	    OFN_ENABLEHOOK;
    ofn.lpstrDefExt = NULL;
    ofn.lpfnHook = open_file_hook_proc;
    ofn.lpTemplateName = "ETHEREAL_OPENFILENAME_TEMPLATE";

    /* XXX - Get our filter */

    if (GetOpenFileName(&ofn)) {
	if (cf_open(&cfile, file_name, FALSE, &err) != CF_OK) {
	    epan_cleanup();
	    exit(2);
	}
	switch (cf_read(&cfile)) {
            case CF_READ_OK:
            case CF_READ_ERROR:
                dirname = get_dirname(file_name);
                set_last_open_dir(dirname);
//                menu_name_resolution_changed(h_wnd);
                return TRUE;
                break;
	}
    }
    return FALSE;
}


void
win32_save_as_file(HWND h_wnd, action_after_save_e action_after_save, gpointer action_after_save_data) {
    static OPENFILENAME ofn;
    gchar  file_name[MAX_PATH] = "";
    gchar *dirname;

    /* XXX - Check for version and set OPENFILENAME_SIZE_VERSION_400
       where appropriate */
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = h_wnd;
    ofn.hInstance = (HINSTANCE) GetWindowLong(h_wnd, GWL_HINSTANCE);
    /* XXX - Grab the rest of the extension list from ethereal.nsi. */
    ofn.lpstrFilter = FILE_TYPE_LIST;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = FILE_TYPE_DEFAULT;
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    if (prefs.gui_fileopen_style == FO_STYLE_SPECIFIED && prefs.gui_fileopen_dir[0] != '\0') {
	ofn.lpstrInitialDir = prefs.gui_fileopen_dir;
    } else {
	ofn.lpstrInitialDir = NULL;
    }
    ofn.lpstrTitle = "Ethereal: Save file as";
    ofn.Flags = OFN_ENABLESIZING | OFN_ENABLETEMPLATE | OFN_EXPLORER |
	    OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY |
	    OFN_PATHMUSTEXIST | OFN_ENABLEHOOK;
    ofn.lpstrDefExt = NULL;
    ofn.lpfnHook = save_as_file_hook_proc;
    ofn.lpTemplateName = "ETHEREAL_SAVEFILENAME_TEMPLATE";

    if (GetSaveFileName(&ofn)) {
	g_sf_hwnd = NULL;
	/* Write out the packets (all, or only the ones from the current
	   range) to the file with the specified name. */
	/* XXX - If we're overwriting a file, GetSaveFileName does the
	   standard windows confirmation.  cf_save() then rejects the overwrite. */
	if (cf_save(&cfile, file_name, &range, filetype, FALSE) != CF_OK) {
	    /* The write failed.  Try again. */
	    win32_save_as_file(h_wnd, action_after_save, action_after_save_data);
	    return;
	}

	/* Save the directory name for future file dialogs. */
	dirname = get_dirname(file_name);  /* Overwrites cf_name */
	set_last_open_dir(dirname);

	/* we have finished saving, do we have pending things to do? */
	switch(action_after_save) {
	    case(after_save_no_action):
		break;
	    case(after_save_open_dialog):
		win32_open_file(h_wnd);
		break;
	    case(after_save_open_recent_file):
//		menu_open_recent_file_cmd(action_after_save_data_g);
		break;
	    case(after_save_open_dnd_file):
//		dnd_open_file_cmd(action_after_save_data_g);
		break;
	    case(after_save_merge_dialog):
//		file_merge_cmd(action_after_save_data_g);
		break;
#ifdef HAVE_LIBPCAP
	    case(after_save_capture_dialog):
		capture_start_confirmed();
		break;
#endif
	    case(after_save_close_file):
		cf_close(&cfile);
		break;
	     case(after_save_exit):
		main_do_quit();
		break;
	    default:
		g_assert_not_reached();
	}
    }
    g_sf_hwnd = NULL;
}


void
win32_merge_file (HWND h_wnd) {
    static      OPENFILENAME ofn;
    gchar       file_name[MAX_PATH] = "";
    char       *dirname;
    cf_status_t merge_status;
    char       *in_filenames[2];
    int         err;
    char       *tmpname;

    /* XXX - Check for temp file and prompt accordingly */

    /* XXX - Check for version and set OPENFILENAME_SIZE_VERSION_400
       where appropriate */
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = h_wnd;
    ofn.hInstance = (HINSTANCE) GetWindowLong(h_wnd, GWL_HINSTANCE);
    /* XXX - Grab the rest of the extension list from ethereal.nsi. */
    ofn.lpstrFilter = FILE_TYPE_LIST;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = FILE_TYPE_DEFAULT;
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    if (prefs.gui_fileopen_style == FO_STYLE_SPECIFIED && prefs.gui_fileopen_dir[0] != '\0') {
	ofn.lpstrInitialDir = prefs.gui_fileopen_dir;
    } else {
	ofn.lpstrInitialDir = NULL;
    }
    ofn.lpstrTitle = "Ethereal: Merge with capture file";
    ofn.Flags = OFN_ENABLESIZING | OFN_ENABLETEMPLATE | OFN_EXPLORER |
	    OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
	    OFN_ENABLEHOOK;
    ofn.lpstrDefExt = NULL;
    ofn.lpfnHook = merge_file_hook_proc;
    ofn.lpTemplateName = "ETHEREAL_MERGEFILENAME_TEMPLATE";

    if (GetOpenFileName(&ofn)) {
	filetype = cfile.cd_t;

	/* merge or append the two files */

        tmpname = NULL;
	switch (merge_action) {
	    case merge_append:
		/* append file */
		in_filenames[0] = file_name;
		in_filenames[1] = cfile.filename;
		merge_status = cf_merge_files(&tmpname, 2, in_filenames, filetype, TRUE);
		break;
	    case merge_chrono:
		/* chonological order */
		in_filenames[0] = cfile.filename;
		in_filenames[1] = file_name;
                merge_status = cf_merge_files(&tmpname, 2, in_filenames, filetype, FALSE);
		break;
	    case merge_prepend:
		/* prepend file */
		in_filenames[0] = cfile.filename;
		in_filenames[1] = file_name;
                merge_status = cf_merge_files(&tmpname, 2, in_filenames, filetype, TRUE);
		break;
	    default:
		g_assert_not_reached();
	}

	if(merge_status != CF_OK) {
	    /* merge failed */
            g_free(tmpname);
//	    if (rfcode != NULL)
//		dfilter_free(rfcode);
	    return;
	}

	cf_close(&cfile);

	/* Try to open the merged capture file. */
	if (cf_open(&cfile, tmpname, TRUE /* temporary file */, &err) != CF_OK) {
	    /* We couldn't open it; don't dismiss the open dialog box,
	       just leave it around so that the user can, after they
	       dismiss the alert box popped up for the open error,
	       try again. */
//	    if (rfcode != NULL)
//		dfilter_free(rfcode);
	    return;
	}

	/* Attach the new read filter to "cf" ("cf_open()" succeeded, so
	   it closed the previous capture file, and thus destroyed any
	   previous read filter attached to "cf"). */
//	cfile.rfcode = rfcode;

	switch (cf_read(&cfile)) {
            case CF_READ_OK:
            case CF_READ_ERROR:
                dirname = get_dirname(file_name);
                set_last_open_dir(dirname);
//                menu_name_resolution_changed(h_wnd);
                break;
            case CF_READ_ABORTED:
                break;
        }

    } else if (CommDlgExtendedError() != 0) {
        sprintf(file_name, "failed: %d/%d (%p)",  CommDlgExtendedError(), GetLastError(), h_wnd);
        MessageBox(NULL, file_name, "Eth", MB_OK);
    }
}

void
win32_export_file(HWND h_wnd, export_type_e export_type) {
    static            OPENFILENAME ofn;
    gchar             file_name[MAX_PATH] = "";
    char             *dirname;
    cf_print_status_t status;

    /* XXX - Check for version and set OPENFILENAME_SIZE_VERSION_400
       where appropriate */
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = h_wnd;
    ofn.hInstance = (HINSTANCE) GetWindowLong(h_wnd, GWL_HINSTANCE);
    ofn.lpstrFilter =
	"Plain text (*.txt)\0"		        	"*.txt\0"
	"PostScript (*.ps)\0"			        "*.ps\0"
        "CSV (Comma Separated Values summary (*.csv)\0" "*.csv\0"
	"PSML (XML packet summary) (*.psml)\0"  	"*.psml\0"
	"PDML (XML packet detail) (*.pdml)\0"   	"*.pdml\0"
	"\0";
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = export_type;
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    if (prefs.gui_fileopen_style == FO_STYLE_SPECIFIED && prefs.gui_fileopen_dir[0] != '\0') {
	ofn.lpstrInitialDir = prefs.gui_fileopen_dir;
    } else {
	ofn.lpstrInitialDir = NULL;
    }
    ofn.lpstrTitle = "Ethereal: Export";
    ofn.Flags = OFN_ENABLESIZING | OFN_ENABLETEMPLATE | OFN_EXPLORER |
	    OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY |
	    OFN_PATHMUSTEXIST | OFN_ENABLEHOOK;
    ofn.lpstrDefExt = NULL;
    ofn.lpfnHook = export_file_hook_proc;
    ofn.lpTemplateName = "ETHEREAL_EXPORTFILENAME_TEMPLATE";

    /* Fill in our print (and export) args */

    print_args.format              = PR_FMT_TEXT;
    print_args.to_file             = TRUE;
    print_args.file                = file_name;
    print_args.cmd                 = NULL;
    print_args.print_summary       = TRUE;
    print_args.print_dissections   = print_dissections_as_displayed;
    print_args.print_hex           = FALSE;
    print_args.print_formfeed      = FALSE;

    if (GetSaveFileName(&ofn)) {
	switch (ofn.nFilterIndex) {
	    case export_type_text:	/* Text */
		print_args.stream = print_stream_text_new(TRUE, print_args.file);
		if (print_args.stream == NULL) {
		    open_failure_alert_box(print_args.file, errno, TRUE);
		    return;
		}
		status = cf_print_packets(&cfile, &print_args);
		break;
	    case export_type_ps:	/* PostScript (r) */
		print_args.stream = print_stream_ps_new(TRUE, print_args.file);
		if (print_args.stream == NULL) {
		    open_failure_alert_box(print_args.file, errno, TRUE);
		    return;
		}
		status = cf_print_packets(&cfile, &print_args);
		break;
            case export_type_csv:     /* CSV */
                status = cf_write_csv_packets(&cfile, &print_args);
                break;
	    case export_type_psml:	/* PSML */
		status = cf_write_psml_packets(&cfile, &print_args);
		break;
	    case export_type_pdml:	/* PDML */
		status = cf_write_pdml_packets(&cfile, &print_args);
		break;
	    default:
		return;
	}

	switch (status) {
	    case CF_PRINT_OK:
		break;
            case CF_PRINT_OPEN_ERROR:
		open_failure_alert_box(print_args.file, errno, TRUE);
		break;
            case CF_PRINT_WRITE_ERROR:
		write_failure_alert_box(print_args.file, errno);
		break;
	}
	/* Save the directory name for future file dialogs. */
	dirname = get_dirname(file_name);  /* Overwrites cf_name */
	set_last_open_dir(dirname);
    }
}

void
win32_export_raw_file(HWND h_wnd) {
    static        OPENFILENAME ofn;
    gchar         file_name[MAX_PATH] = "";
    char         *dirname;
    const guint8 *data_p = NULL;
    const char   *file = NULL;
    int           fd;

    if (!cfile.finfo_selected) {
	/* This shouldn't happen */
	simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "No bytes were selected.");
	return;
    }

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = h_wnd;
    ofn.hInstance = (HINSTANCE) GetWindowLong(h_wnd, GWL_HINSTANCE);
    /* XXX - Grab the rest of the extension list from ethereal.nsi. */
    ofn.lpstrFilter =
	"Raw data (*.bin, *.dat, *.raw)\0"	"*.bin;*.dat;*.raw\0"
	"All Files (*.*)\0"				"*.*\0"
	"\0";
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    if (prefs.gui_fileopen_style == FO_STYLE_SPECIFIED && prefs.gui_fileopen_dir[0] != '\0') {
	ofn.lpstrInitialDir = prefs.gui_fileopen_dir;
    } else {
	ofn.lpstrInitialDir = NULL;
    }
    ofn.lpstrTitle = "Ethereal: Export Raw Data";
    ofn.Flags = OFN_ENABLESIZING | OFN_ENABLETEMPLATE | OFN_EXPLORER |
	    OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY |
	    OFN_PATHMUSTEXIST | OFN_ENABLEHOOK;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = cfile.finfo_selected->length;
    ofn.lpfnHook = export_raw_file_hook_proc;
    ofn.lpTemplateName = "ETHEREAL_EXPORTRAWFILENAME_TEMPLATE";

    /*
     * XXX - The GTK+ code uses get_byte_view_data_and_length().  We just
     * grab the info from cfile.finfo_selected.  Which is more "correct"?
     */

    if (GetSaveFileName(&ofn)) {

	data_p = tvb_get_ptr(cfile.finfo_selected->ds_tvb, 0, -1) +
		cfile.finfo_selected->start;
        fd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0666);
        if (fd == -1) {
	    open_failure_alert_box(file, errno, TRUE);
	    return;
        }
        if (write(fd, data_p, cfile.finfo_selected->length) < 0) {
	    write_failure_alert_box(file, errno);
	    close(fd);
	    return;
        }
        if (close(fd) < 0) {
	    write_failure_alert_box(file, errno);
	    return;
        }

	/* Save the directory name for future file dialogs. */
	dirname = get_dirname(file_name);  /* Overwrites cf_name */
	set_last_open_dir(dirname);
    }
}

void
win32_export_color_file(HWND h_wnd) {
    static OPENFILENAME ofn;
    gchar  file_name[MAX_PATH] = "";
    char  *dirname;

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = h_wnd;
    ofn.hInstance = (HINSTANCE) GetWindowLong(h_wnd, GWL_HINSTANCE);
    /* XXX - Grab the rest of the extension list from ethereal.nsi. */
    ofn.lpstrFilter =
	"Text Files (*.txt)\0"	"*.txt\0"
	"All Files (*.*)\0"	"*.*\0"
	"\0";
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 2;
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = "Ethereal: Export Color Filters";
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER |
	    OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY |
	    OFN_PATHMUSTEXIST | OFN_ENABLEHOOK;
    ofn.lpstrDefExt = NULL;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    filetype = cfile.cd_t;

    /* XXX - Support marked filters */
    if (GetSaveFileName(&ofn)) {
	if (!color_filters_export(file_name, FALSE))
	    return;

	/* Save the directory name for future file dialogs. */
	dirname = get_dirname(file_name);  /* Overwrites cf_name */
	set_last_open_dir(dirname);
    }
}

void
win32_import_color_file(HWND h_wnd) {
    static OPENFILENAME ofn;
    gchar  file_name[MAX_PATH] = "";
    char  *dirname;

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = h_wnd;
    ofn.hInstance = (HINSTANCE) GetWindowLong(h_wnd, GWL_HINSTANCE);
    /* XXX - Grab the rest of the extension list from ethereal.nsi. */
    ofn.lpstrFilter =
	"Text Files (*.txt)\0"	"*.txt\0"
	"All Files (*.*)\0"	"*.*\0"
	"\0";
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 2;
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = "Ethereal: Import Color Filters";
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER |
	    OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY |
	    OFN_PATHMUSTEXIST | OFN_ENABLEHOOK;
    ofn.lpstrDefExt = NULL;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    /* XXX - Support marked filters */
    if (GetOpenFileName(&ofn)) {
	if (!color_filters_import(file_name, NULL))
	    return;

	/* Save the directory name for future file dialogs. */
	dirname = get_dirname(file_name);  /* Overwrites cf_name */
	set_last_open_dir(dirname);
    }
}


/*
 * Private routines
 */
static void
format_handle_wm_initdialog(HWND dlg_hwnd, print_args_t *args) {
    HWND cur_ctrl;

    /* Set the "Packet summary" box */
    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_PKT_SUMMARY_CB);
    SendMessage(cur_ctrl, BM_SETCHECK, args->print_summary, 0);

    /* Set the "Packet details" box */
    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_PKT_DETAIL_CB);
    SendMessage(cur_ctrl, BM_SETCHECK, args->print_dissections != print_dissections_none, 0);

    /* Set the "Packet details" combo */
    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_PKT_DETAIL_COMBO);
    SendMessage(cur_ctrl, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) "All collapsed");
    SendMessage(cur_ctrl, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) "As displayed");
    SendMessage(cur_ctrl, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) "All expanded");

    switch (args->print_dissections) {
	case print_dissections_none:
	case print_dissections_collapsed:
	    SendMessage(cur_ctrl, CB_SETCURSEL, 0, 0);
	    break;
	case print_dissections_as_displayed:
	    SendMessage(cur_ctrl, CB_SETCURSEL, 1, 0);
	    break;
	case print_dissections_expanded:
	    SendMessage(cur_ctrl, CB_SETCURSEL, 2, 0);
	default:
	    g_assert_not_reached();
    }

    /* Set the "Packet bytes" box */
    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_PKT_BYTES_CB);
    SendMessage(cur_ctrl, BM_SETCHECK, args->print_hex, 0);

    /* Set the "Each packet on a new page" box */
    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_PKT_NEW_PAGE_CB);
    SendMessage(cur_ctrl, BM_SETCHECK, args->print_formfeed, 0);

    print_update_dynamic(dlg_hwnd, args);
}

static void
print_update_dynamic(HWND dlg_hwnd, print_args_t *args) {
    HWND cur_ctrl;

    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_PKT_SUMMARY_CB);
    if (SendMessage(cur_ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
	args->print_summary = TRUE;
    else
	args->print_summary = FALSE;

    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_PKT_DETAIL_CB);
    if (SendMessage(cur_ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED) {
	switch (SendMessage(cur_ctrl, CB_GETCURSEL, 0, 0)) {
	    case 0:
		args->print_dissections = print_dissections_collapsed;
		break;
	    case 1:
		args->print_dissections = print_dissections_as_displayed;
		break;
	    case 2:
		args->print_dissections = print_dissections_expanded;
		break;
	    default:
		g_assert_not_reached();
	}
	cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_PKT_DETAIL_COMBO);
	EnableWindow(cur_ctrl, TRUE);
    } else {
	args->print_dissections = print_dissections_none;
	cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_PKT_DETAIL_COMBO);
	EnableWindow(cur_ctrl, FALSE);
    }

    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_PKT_BYTES_CB);
    if (SendMessage(cur_ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
	args->print_hex = TRUE;
    else
	args->print_hex = FALSE;

    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_PKT_NEW_PAGE_CB);
    if (SendMessage(cur_ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
	args->print_formfeed = TRUE;
    else
	args->print_formfeed = FALSE;
}


/*
 * Private routines
 */

#define PREVIEW_STR_MAX      200
#define PREVIEW_TIMEOUT_SECS   3


/* If preview_file is NULL, disable the elements.  If not, enable and
 * show the preview info. */
static gboolean
preview_set_filename(HWND of_hwnd, gchar *preview_file) {
    HWND        cur_ctrl;
    int         i;
    gboolean    enable = FALSE;
    wtap       *wth;
    const struct wtap_pkthdr *phdr;
    int         err = 0;
    gchar      *err_info;
    long        data_offset;
    gchar       string_buff[PREVIEW_STR_MAX];
    guint       packet = 0;
    guint64     filesize;
    time_t      ti_time;
    struct tm  *ti_tm;
    guint       elapsed_time;
    time_t      time_preview;
    time_t      time_current;
    double      start_time = 0;
    double      stop_time = 0;
    double      cur_time;
    gboolean    is_breaked = FALSE;

    if (preview_file != NULL && strlen(preview_file) > 0) {
	enable = TRUE;
    }

    for (i = EWFD_PT_FILENAME; i <= EWFD_PTX_ELAPSED; i++) {
	cur_ctrl = GetDlgItem(of_hwnd, i);
	if (cur_ctrl) {
	    EnableWindow(cur_ctrl, enable);
	}
    }

    for (i = EWFD_PTX_FILENAME; i <= EWFD_PTX_ELAPSED; i++) {
	cur_ctrl = GetDlgItem(of_hwnd, i);
	if (cur_ctrl) {
	    SetWindowText(cur_ctrl, "-");
	}
    }

    if (enable) {
	cur_ctrl = GetDlgItem(of_hwnd, EWFD_PTX_FILENAME);
	SetWindowText(cur_ctrl, get_basename(preview_file));

	cur_ctrl = GetDlgItem(of_hwnd, EWFD_PTX_FORMAT);
	wth = wtap_open_offline(preview_file, &err, &err_info, TRUE);
	if (cur_ctrl && wth == NULL) {
	    if(err == WTAP_ERR_FILE_UNKNOWN_FORMAT) {
		SetWindowText(cur_ctrl, "unknown file format");
	    } else {
		SetWindowText(cur_ctrl, "error opening file");
	    }
	    return FALSE;
	}

	/* size */
        filesize = wtap_file_size(wth, &err);
	g_snprintf(string_buff, PREVIEW_STR_MAX, "%" PRIu64 " bytes", filesize);
	cur_ctrl = GetDlgItem(of_hwnd, EWFD_PTX_SIZE);
	SetWindowText(cur_ctrl, string_buff);

	/* type */
	g_snprintf(string_buff, PREVIEW_STR_MAX, "%s", wtap_file_type_string(wtap_file_type(wth)));
	cur_ctrl = GetDlgItem(of_hwnd, EWFD_PTX_FORMAT);
	SetWindowText(cur_ctrl, string_buff);

	time(&time_preview);
	while ( (wtap_read(wth, &err, &err_info, &data_offset)) ) {
	    phdr = wtap_phdr(wth);
            cur_time = nstime_to_sec( (const nstime_t *) &phdr->ts );
	    if(packet == 0) {
		start_time  = cur_time;
		stop_time = cur_time;
	    }
	    if (cur_time < start_time) {
		start_time = cur_time;
	    }
	    if (cur_time > stop_time){
		stop_time = cur_time;
	    }
	    packet++;
	    if(packet%100) {
		time(&time_current);
		if(time_current-time_preview >= PREVIEW_TIMEOUT_SECS) {
		    is_breaked = TRUE;
		    break;
		}
	    }
	}

	if(err != 0) {
	    g_snprintf(string_buff, PREVIEW_STR_MAX, "error after reading %u packets", packet);
	    cur_ctrl = GetDlgItem(of_hwnd, EWFD_PTX_PACKETS);
	    SetWindowText(cur_ctrl, string_buff);
	    wtap_close(wth);
	    return TRUE;
	}

	/* packet count */
	if(is_breaked) {
	    g_snprintf(string_buff, PREVIEW_STR_MAX, "more than %u packets (preview timeout)", packet);
	} else {
	    g_snprintf(string_buff, PREVIEW_STR_MAX, "%u", packet);
	}
	cur_ctrl = GetDlgItem(of_hwnd, EWFD_PTX_PACKETS);
	SetWindowText(cur_ctrl, string_buff);

	/* first packet */
	ti_time = (long)start_time;
	ti_tm = localtime( &ti_time );
	g_snprintf(string_buff, PREVIEW_STR_MAX,
		 "%04d-%02d-%02d %02d:%02d:%02d",
		 ti_tm->tm_year + 1900,
		 ti_tm->tm_mon + 1,
		 ti_tm->tm_mday,
		 ti_tm->tm_hour,
		 ti_tm->tm_min,
		 ti_tm->tm_sec);
	cur_ctrl = GetDlgItem(of_hwnd, EWFD_PTX_FIRST_PKT);
	SetWindowText(cur_ctrl, string_buff);

	/* elapsed time */
	elapsed_time = (unsigned int)(stop_time-start_time);
	if(elapsed_time/86400) {
	    g_snprintf(string_buff, PREVIEW_STR_MAX, "%02u days %02u:%02u:%02u",
	    elapsed_time/86400, elapsed_time%86400/3600, elapsed_time%3600/60, elapsed_time%60);
	} else {
	    g_snprintf(string_buff, PREVIEW_STR_MAX, "%02u:%02u:%02u",
	    elapsed_time%86400/3600, elapsed_time%3600/60, elapsed_time%60);
	}
	if(is_breaked) {
	    g_snprintf(string_buff, PREVIEW_STR_MAX, "unknown");
	}
	cur_ctrl = GetDlgItem(of_hwnd, EWFD_PTX_ELAPSED);
	SetWindowText(cur_ctrl, string_buff);

	wtap_close(wth);
    }

    return TRUE;

}

// XXX - Copied from "filter-util.c" in the ethereal-win32 branch
/* XXX - The only reason for the "filter_text" parameter is to be able to feed
 * in the "real" filter string in the case of a CBN_SELCHANGE notification message.
 */
void
filter_tb_syntax_check(HWND hwnd, gchar *filter_text) {
    gchar     *strval = NULL;
    gint       len;
    dfilter_t *dfp;

    /* If filter_text is non-NULL, use it.  Otherwise, grab the text from
     * the window */
    if (filter_text) {
        strval = g_strdup(filter_text);
        len = lstrlen(filter_text);
    } else {
        len = GetWindowTextLength(hwnd);
        if (len > 0) {
            len++;
            strval = g_malloc(len);
            GetWindowText(hwnd, strval, len);
        }
    }

    if (len == 0) {
        /* Default window background */
        SendMessage(hwnd, EM_SETBKGNDCOLOR, (WPARAM) 1, COLOR_WINDOW);
        return;
    } else if (dfilter_compile(strval, &dfp)) { /* colorize filter string entry */
        if (dfp != NULL)
            dfilter_free(dfp);
        /* Valid (light green) */
        SendMessage(hwnd, EM_SETBKGNDCOLOR, 0, 0x00afffaf);
    } else {
        /* Invalid (light red) */
        SendMessage(hwnd, EM_SETBKGNDCOLOR, 0, 0x00afafff);
    }

    if (strval) g_free(strval);
}


static UINT CALLBACK
open_file_hook_proc(HWND of_hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    HWND      cur_ctrl, parent;
    OFNOTIFY *notify = (OFNOTIFY *) l_param;
    gchar     sel_name[MAX_PATH];

    switch(msg) {
	case WM_INITDIALOG:
	    /* XXX - Retain the filter text, and fill it in. */

	    /* Fill in our resolution values */
	    cur_ctrl = GetDlgItem(of_hwnd, EWFD_MAC_NR_CB);
	    SendMessage(cur_ctrl, BM_SETCHECK, g_resolv_flags & RESOLV_MAC, 0);
	    cur_ctrl = GetDlgItem(of_hwnd, EWFD_NET_NR_CB);
	    SendMessage(cur_ctrl, BM_SETCHECK, g_resolv_flags & RESOLV_NETWORK, 0);
	    cur_ctrl = GetDlgItem(of_hwnd, EWFD_TRANS_NR_CB);
	    SendMessage(cur_ctrl, BM_SETCHECK, g_resolv_flags & RESOLV_TRANSPORT, 0);

	    preview_set_filename(of_hwnd, NULL);
	    break;
	case WM_NOTIFY:
	    switch (notify->hdr.code) {
		case CDN_FILEOK:
		    /* XXX - Fetch the read filter */
		    /* Fetch our resolution values */
		    g_resolv_flags = prefs.name_resolve & RESOLV_CONCURRENT;
		    cur_ctrl = GetDlgItem(of_hwnd, EWFD_MAC_NR_CB);
		    if (SendMessage(cur_ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
			g_resolv_flags |= RESOLV_MAC;
		    cur_ctrl = GetDlgItem(of_hwnd, EWFD_NET_NR_CB);
		    if (SendMessage(cur_ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
			g_resolv_flags |= RESOLV_NETWORK;
		    cur_ctrl = GetDlgItem(of_hwnd, EWFD_TRANS_NR_CB);
		    if (SendMessage(cur_ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
			g_resolv_flags |= RESOLV_TRANSPORT;
		    break;
		case CDN_SELCHANGE:
		    /* This _almost_ works correctly.  We need to handle directory
		       selections, etc. */
		    parent = GetParent(of_hwnd);
		    CommDlg_OpenSave_GetSpec(parent, sel_name, MAX_PATH);
		    preview_set_filename(of_hwnd, sel_name);
		    break;
		default:
		    break;
	    }
	    break;
	case WM_COMMAND:
	    cur_ctrl = (HWND) l_param;
	    switch(w_param) {
		case (EN_UPDATE << 16) | EWFD_FILTER_EDIT:
		    filter_tb_syntax_check(cur_ctrl, NULL);
		    break;
                case EWFD_FILTER_BTN:
                    /* XXX - Integrate with the filter dialog? */
                    break;
		default:
		    break;
	    }
	    break;
	default:
	    break;
    }
    return 0;
}

/* XXX - Copied verbatim from gtk/file_dlg.c.  Perhaps it
 * should be in wiretap instead?
 */

static gboolean
can_save_with_wiretap(int ft)
{
    /* To save a file with Wiretap, Wiretap has to handle that format,
     and its code to handle that format must be able to write a file
     with this file's encapsulation type. */
    return wtap_dump_can_open(ft) && wtap_dump_can_write_encap(ft, cfile.lnk_t);
}

/* Generate a list of the file types we can save this file as.

   "filetype" is the type it has now.

   "encap" is the encapsulation for its packets (which could be
   "unknown" or "per-packet").

   "filtered" is TRUE if we're to save only the packets that passed
   the display filter (in which case we have to save it using Wiretap)
   and FALSE if we're to save the entire file (in which case, if we're
   saving it in the type it has already, we can just copy it).

   The same applies for sel_curr, sel_all, sel_m_only, sel_m_range and sel_man_range
*/

static void
build_file_format_list(HWND sf_hwnd) {
    HWND  format_cb;
    int   ft;
    guint index;
    guint item_to_select;

    /* Default to the first supported file type, if the file's current
       type isn't supported. */
    item_to_select = 0;

    format_cb = GetDlgItem(sf_hwnd, EWFD_FILE_TYPE_COMBO);
    SendMessage(format_cb, CB_RESETCONTENT, 0, 0);

    /* Check all file types. */
    index = 0;
    for (ft = 0; ft < WTAP_NUM_FILE_TYPES; ft++) {
	if (!packet_range_process_all(&range) || ft != cfile.cd_t) {
	    /* not all unfiltered packets or a different file type.  We have to use Wiretap. */
	    if (!can_save_with_wiretap(ft))
		continue;       /* We can't. */
	}

	/* OK, we can write it out in this type. */
	SendMessage(format_cb, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) wtap_file_type_string(ft));
	SendMessage(format_cb, CB_SETITEMDATA, (LPARAM) index, (WPARAM) ft);
	if (ft == filetype) {
	    /* Default to the same format as the file, if it's supported. */
	    item_to_select = index;
	}
	index++;
    }

    SendMessage(format_cb, CB_SETCURSEL, (WPARAM) item_to_select, 0);
}

#define RANGE_TEXT_MAX 128
static UINT CALLBACK
save_as_file_hook_proc(HWND sf_hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    HWND           cur_ctrl;
    OFNOTIFY      *notify = (OFNOTIFY *) l_param;
    int            new_filetype, index;

    switch(msg) {
	case WM_INITDIALOG:
	    g_sf_hwnd = sf_hwnd;

	    /* Default to saving all packets, in the file's current format. */
	    filetype = cfile.cd_t;

	    /* init the packet range */
	    packet_range_init(&range);

	    /* Fill in the file format list */
	    build_file_format_list(sf_hwnd);

	    range_handle_wm_initdialog(sf_hwnd, &range);

	    break;
	case WM_COMMAND:
	    cur_ctrl = (HWND) l_param;

	    switch (w_param) {
		case (CBN_SELCHANGE << 16) | EWFD_FILE_TYPE_COMBO:
		    index = SendMessage(cur_ctrl, CB_GETCURSEL, 0, 0);
		    if (index != CB_ERR) {
			new_filetype = SendMessage(cur_ctrl, CB_GETITEMDATA, (WPARAM) index, 0);
			if (new_filetype != CB_ERR) {
			    if (filetype != new_filetype) {
				if (can_save_with_wiretap(new_filetype)) {
				    cur_ctrl = GetDlgItem(sf_hwnd, EWFD_CAPTURED_BTN);
				    EnableWindow(cur_ctrl, TRUE);
				    cur_ctrl = GetDlgItem(sf_hwnd, EWFD_DISPLAYED_BTN);
				    EnableWindow(cur_ctrl, TRUE);
				} else {
				    cur_ctrl = GetDlgItem(sf_hwnd, EWFD_CAPTURED_BTN);
				    SendMessage(cur_ctrl, BM_SETCHECK, 0, 0);
				    EnableWindow(cur_ctrl, FALSE);
				    cur_ctrl = GetDlgItem(sf_hwnd, EWFD_DISPLAYED_BTN);
				    EnableWindow(cur_ctrl, FALSE);
				}
				filetype = new_filetype;
			    }
			}
		    }
		    break;
		default:
		    range_handle_wm_command(sf_hwnd, cur_ctrl, w_param, &range);
		    break;
	    }
	    break;
	default:
	    break;
    }
    return 0;
}



/* For each range static control, fill in its value and enable/disable it. */
static void
range_update_dynamics(HWND dlg_hwnd, packet_range_t *range) {
    HWND     cur_ctrl;
    gboolean filtered_active = FALSE;
    gchar    static_val[100];
    gint     selected_num;

    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_DISPLAYED_BTN);
    if (SendMessage(cur_ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
	filtered_active = TRUE;

    /* RANGE_SELECT_ALL */
    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_ALL_PKTS_CAP);
    EnableWindow(cur_ctrl, !filtered_active);
    g_snprintf(static_val, sizeof(static_val), "%u", cfile.count);
    SetWindowText(cur_ctrl, static_val);

    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_ALL_PKTS_DISP);
    EnableWindow(cur_ctrl, filtered_active);
    g_snprintf(static_val, sizeof(static_val), "%u", range->displayed_cnt);
    SetWindowText(cur_ctrl, static_val);

    /* RANGE_SELECT_CURR */
    selected_num = (cfile.current_frame) ? cfile.current_frame->num : 0;
    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_SEL_PKT_CAP);
    EnableWindow(cur_ctrl, selected_num && !filtered_active);
    g_snprintf(static_val, sizeof(static_val), "%u", selected_num ? 1 : 0);
    SetWindowText(cur_ctrl, static_val);

    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_SEL_PKT_DISP);
    EnableWindow(cur_ctrl, selected_num && filtered_active);
    g_snprintf(static_val, sizeof(static_val), "%u", selected_num ? 1 : 0);
    SetWindowText(cur_ctrl, static_val);

    /* RANGE_SELECT_MARKED */
    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_MARKED_BTN);
    EnableWindow(cur_ctrl, cfile.marked_count);

    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_MARKED_CAP);
    EnableWindow(cur_ctrl, cfile.marked_count && !filtered_active);
    g_snprintf(static_val, sizeof(static_val), "%u", cfile.marked_count);
    SetWindowText(cur_ctrl, static_val);

    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_MARKED_DISP);
    EnableWindow(cur_ctrl, cfile.marked_count && filtered_active);
    g_snprintf(static_val, sizeof(static_val), "%u", range->displayed_marked_cnt);
    SetWindowText(cur_ctrl, static_val);

    /* RANGE_SELECT_MARKED_RANGE */
    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_FIRST_LAST_BTN);
    EnableWindow(cur_ctrl, range->mark_range_cnt);

    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_FIRST_LAST_CAP);
    EnableWindow(cur_ctrl, range->mark_range_cnt && !filtered_active);
    g_snprintf(static_val, sizeof(static_val), "%u", range->mark_range_cnt);
    SetWindowText(cur_ctrl, static_val);

    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_FIRST_LAST_DISP);
    EnableWindow(cur_ctrl, range->displayed_mark_range_cnt && filtered_active);
    g_snprintf(static_val, sizeof(static_val), "%u", range->displayed_mark_range_cnt);
    SetWindowText(cur_ctrl, static_val);

    /* RANGE_SELECT_USER */
    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_RANGE_CAP);
    EnableWindow(cur_ctrl, !filtered_active);
    g_snprintf(static_val, sizeof(static_val), "%u", range->user_range_cnt);
    SetWindowText(cur_ctrl, static_val);

    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_RANGE_DISP);
    EnableWindow(cur_ctrl, filtered_active);
    g_snprintf(static_val, sizeof(static_val), "%u", range->displayed_user_range_cnt);
    SetWindowText(cur_ctrl, static_val);
}

static void
range_handle_wm_initdialog(HWND dlg_hwnd, packet_range_t *range) {
    HWND cur_ctrl;

    /* Set the appropriate captured/displayed radio */
    if (range->process_filtered)
	cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_DISPLAYED_BTN);
    else
	cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_CAPTURED_BTN);
    SendMessage(cur_ctrl, BM_SETCHECK, TRUE, 0);

    /* dynamic values in the range frame */
    range_update_dynamics(dlg_hwnd, range);

    /* Set the appropriate range radio */
    switch(range->process) {
	case(range_process_all):
	    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_ALL_PKTS_BTN);
	    break;
	case(range_process_selected):
	    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_SEL_PKT_BTN);
	    break;
	case(range_process_marked):
	    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_MARKED_BTN);
	    break;
	case(range_process_marked_range):
	    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_FIRST_LAST_BTN);
	    break;
	case(range_process_user_range):
	    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_RANGE_BTN);
	    break;
	default:
	    g_assert_not_reached();
    }
    SendMessage(cur_ctrl, BM_SETCHECK, TRUE, 0);
}

static void
range_handle_wm_command(HWND dlg_hwnd, HWND ctrl, WPARAM w_param, packet_range_t *range) {
    HWND  cur_ctrl;
    gchar range_text[RANGE_TEXT_MAX];

    switch(w_param) {
	case (BN_CLICKED << 16) | EWFD_CAPTURED_BTN:
	case (BN_CLICKED << 16) | EWFD_DISPLAYED_BTN:
	    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_CAPTURED_BTN);
	    if (SendMessage(cur_ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
		range->process_filtered = FALSE;
	    else
		range->process_filtered = TRUE;
	    range_update_dynamics(dlg_hwnd, range);
	    break;
	case (BN_CLICKED << 16) | EWFD_ALL_PKTS_BTN:
	    if (SendMessage(ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		range->process = range_process_all;
		range_update_dynamics(dlg_hwnd, range);
	    }
	    break;
	case (BN_CLICKED << 16) | EWFD_SEL_PKT_BTN:
	    if (SendMessage(ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		range->process = range_process_selected;
		range_update_dynamics(dlg_hwnd, range);
	    }
	    break;
	case (BN_CLICKED << 16) | EWFD_MARKED_BTN:
	    if (SendMessage(ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		range->process = range_process_marked;
		range_update_dynamics(dlg_hwnd, range);
	    }
	    break;
	case (BN_CLICKED << 16) | EWFD_FIRST_LAST_BTN:
	    if (SendMessage(ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		range->process = range_process_marked_range;
		range_update_dynamics(dlg_hwnd, range);
	    }
	    break;
	case (BN_CLICKED << 16) | EWFD_RANGE_BTN:
	    if (SendMessage(ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		range->process = range_process_user_range;
		range_update_dynamics(dlg_hwnd, range);
		cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_RANGE_EDIT);
		SetFocus(cur_ctrl);
	    }
	    break;
	case (EN_SETFOCUS << 16) | EWFD_RANGE_EDIT:
	    cur_ctrl = GetDlgItem(dlg_hwnd, EWFD_RANGE_BTN);
	    SendMessage(cur_ctrl, BM_CLICK, 0, 0);
	    break;
	case (EN_CHANGE << 16) | EWFD_RANGE_EDIT:
	    SendMessage(ctrl, WM_GETTEXT, (WPARAM) RANGE_TEXT_MAX, (LPARAM) range_text);
	    packet_range_convert_str(range, range_text);
	    range_update_dynamics(dlg_hwnd, range);
	    break;
    }
}

static UINT CALLBACK
merge_file_hook_proc(HWND mf_hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    HWND      cur_ctrl, parent;
    OFNOTIFY *notify = (OFNOTIFY *) l_param;
    gchar     sel_name[MAX_PATH];

    switch(msg) {
	case WM_INITDIALOG:
	    /* XXX - Retain the filter text, and fill it in. */

	    /* Append by default */
	    cur_ctrl = GetDlgItem(mf_hwnd, EWFD_MERGE_PREPEND_BTN);
	    SendMessage(cur_ctrl, BM_SETCHECK, TRUE, 0);
	    merge_action = merge_append;

	    preview_set_filename(mf_hwnd, NULL);
	    break;
	case WM_NOTIFY:
	    switch (notify->hdr.code) {
		case CDN_FILEOK:
		    /* XXX - Fetch the read filter */

		    cur_ctrl = GetDlgItem(mf_hwnd, EWFD_MERGE_CHRONO_BTN);
		    if(SendMessage(cur_ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED) {
			merge_action = merge_chrono;
		    } else {
			cur_ctrl = GetDlgItem(mf_hwnd, EWFD_MERGE_PREPEND_BTN);
			if(SendMessage(cur_ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED) {
			    merge_action = merge_prepend;
			}
		    }

		    break;
		case CDN_SELCHANGE:
		    /* This _almost_ works correctly.  We need to handle directory
		       selections, etc. */
		    parent = GetParent(mf_hwnd);
		    CommDlg_OpenSave_GetSpec(parent, sel_name, MAX_PATH);
		    preview_set_filename(mf_hwnd, sel_name);
		    break;
		default:
		    break;
	    }
	    break;
	case WM_COMMAND:
	    cur_ctrl = (HWND) l_param;
	    switch(w_param) {
		case (EN_UPDATE << 16) | EWFD_FILTER_EDIT:
		    filter_tb_syntax_check(cur_ctrl, NULL);
		    break;
		default:
		    break;
	    }
	    break;
	default:
	    break;
    }
    return 0;
}


static UINT CALLBACK
export_file_hook_proc(HWND ef_hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    HWND           cur_ctrl;
    OFNOTIFY      *notify = (OFNOTIFY *) l_param;
    gboolean       pkt_fmt_enable;
    int            i, index;

    switch(msg) {
	case WM_INITDIALOG:
	    /* init the printing range */
	    packet_range_init(&print_args.range);
	    range_handle_wm_initdialog(ef_hwnd, &print_args.range);
	    format_handle_wm_initdialog(ef_hwnd, &print_args);

	    break;
	case WM_COMMAND:
	    cur_ctrl = (HWND) l_param;
	    switch (w_param) {
//		case (CBN_SELCHANGE << 16) | EWFD_FILE_TYPE_COMBO:
//		    break;
		default:
		    range_handle_wm_command(ef_hwnd, cur_ctrl, w_param, &print_args.range);
		    print_update_dynamic(ef_hwnd, &print_args);
		    break;
	    }
	    break;
	case WM_NOTIFY:
	    switch (notify->hdr.code) {
		case CDN_FILEOK:
		    break;
		case CDN_TYPECHANGE:
		    index = notify->lpOFN->nFilterIndex;

		    if (index == 2)	/* PostScript */
			print_args.format = PR_FMT_TEXT;
		    else
			print_args.format = PR_FMT_PS;
		    if (index == 3 || index == 4)
			pkt_fmt_enable = FALSE;
		    else
			pkt_fmt_enable = TRUE;
		    for (i = EWFD_PKT_FORMAT_GB; i <= EWFD_PKT_NEW_PAGE_CB; i++) {
			cur_ctrl = GetDlgItem(ef_hwnd, i);
			EnableWindow(cur_ctrl, pkt_fmt_enable);
		    }
		    break;
		default:
		    break;
	    }
	    break;
	default:
	    break;
    }
    return 0;
}

static UINT CALLBACK
export_raw_file_hook_proc(HWND ef_hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    HWND          cur_ctrl;
    OPENFILENAME *ofnp = (OPENFILENAME *) l_param;
    gchar         raw_msg[100];

    switch(msg) {
	case WM_INITDIALOG:
	    g_snprintf(raw_msg, sizeof(raw_msg), "%d byte%s of raw binary data will be written",
		    ofnp->lCustData, plurality(ofnp->lCustData, "", "s"));
	    cur_ctrl = GetDlgItem(ef_hwnd, EWFD_EXPORTRAW_ST);
	    SetWindowText(cur_ctrl, raw_msg);
	    break;
	default:
	    break;
    }
    return 0;
}
