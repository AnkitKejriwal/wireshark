/* main_window.h
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <stdio.h>

#include "config.h"

#include <glib.h>

#include "file.h"

#include <QMainWindow>
#include <QSplitter>
#include "main_welcome.h"
#include "packet_list.h"
#include "display_filter_combo.h"
#include "progress_bar.h"

class QAction;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    Ui::MainWindow *ui;
    QMenu *openRecentMenu;
    QSplitter *splitterV;
    MainWelcome *mainWelcome;
    DisplayFilterCombo *dfComboBox;
    capture_file *capFile;
    PacketList *m_packetList;

signals:
    void showProgress(progdlg_t **dlg_p, bool animate, const QString message, bool terminate_is_stop, bool *stop_flag, float pct);

public slots:
    void captureFileReadStarted(const capture_file *cf);
    void captureFileReadFinished(const capture_file *cf);
    void captureFileClosing(const capture_file *cf);
    void captureFileClosed(const capture_file *cf);

private slots:
    void closeCaptureFile();
    void updateRecentFiles();
    void openCaptureFile(QString& cfPath = *new QString());
    void recentActionTriggered();
    void on_actionGoNextPacket_triggered();
    void on_actionGoPreviousPacket_triggered();
    void on_actionGoFirstPacket_triggered();
    void on_actionGoLastPacket_triggered();
    void on_actionHelpWebsite_triggered();
    void on_actionHelpFAQ_triggered();
    void on_actionHelpAsk_triggered();
    void on_actionHelpDownloads_triggered();
    void on_actionHelpWiki_triggered();
    void on_actionHelpSampleCaptures_triggered();
};


#endif // MAINWINDOW_H
