/* proto_tree.cpp
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

#include <stdio.h>

#include "proto_tree.h"
#include "monospace_font.h"

#include <epan/ftypes/ftypes.h>
#include <epan/prefs.h>

#include <QApplication>
#include <QHeaderView>
#include <QTreeWidgetItemIterator>
#include <QDesktopServices>
#include <QUrl>

QColor        expert_color_chat       ( 0x80, 0xb7, 0xf7 );        /* light blue */
QColor        expert_color_note       ( 0xa0, 0xff, 0xff );        /* bright turquoise */
QColor        expert_color_warn       ( 0xf7, 0xf2, 0x53 );        /* yellow */
QColor        expert_color_error      ( 0xff, 0x5c, 0x5c );        /* pale red */
QColor        expert_color_foreground ( 0x00, 0x00, 0x00 );        /* black */
QColor        hidden_proto_item       ( 0x44, 0x44, 0x44 );        /* gray */

/* Fill a single protocol tree item with its string value and set its color. */
static void
proto_tree_draw_node(proto_node *node, gpointer data)
{
    field_info   *fi = PNODE_FINFO(node);
    gchar         label_str[ITEM_LABEL_LENGTH];
    gchar        *label_ptr;
    gboolean      is_leaf;

    /* dissection with an invisible proto tree? */
    g_assert(fi);

    if (PROTO_ITEM_IS_HIDDEN(node) && !prefs.display_hidden_proto_items)
        return;

    // Fill in our label
    /* was a free format label produced? */
    if (fi->rep) {
        label_ptr = fi->rep->representation;
    }
    else { /* no, make a generic label */
        label_ptr = label_str;
        proto_item_fill_label(fi, label_str);
    }

    if (node->first_child != NULL) {
        is_leaf = FALSE;
        g_assert(fi->tree_type >= 0 && fi->tree_type < num_tree_types);
    }
    else {
        is_leaf = TRUE;
    }

    if (PROTO_ITEM_IS_GENERATED(node)) {
        if (PROTO_ITEM_IS_HIDDEN(node)) {
            label_ptr = g_strdup_printf("<[%s]>", label_ptr);
        } else {
            label_ptr = g_strdup_printf("[%s]", label_ptr);
        }
    } else if (PROTO_ITEM_IS_HIDDEN(node)) {
        label_ptr = g_strdup_printf("<%s>", label_ptr);
    }

    QTreeWidgetItem *parentItem = (QTreeWidgetItem *)data;
    QTreeWidgetItem *item;
    item = new QTreeWidgetItem(parentItem, 0);

    // Set our colors.
    QPalette pal = QApplication::palette();
    if (fi && fi->hfinfo) {
        if(fi->hfinfo->type == FT_PROTOCOL) {
            item->setData(0, Qt::BackgroundRole, pal.alternateBase());
        }

        if((fi->hfinfo->type == FT_FRAMENUM) ||
                (FI_GET_FLAG(fi, FI_URL) && IS_FT_STRING(fi->hfinfo->type))) {
            QFont font = item->font(0);

            item->setData(0, Qt::ForegroundRole, pal.link());
            font.setUnderline(true);
            item->setData(0, Qt::FontRole, font);
        }
    }

    // XXX - Add routines to get our severity colors.
    if(FI_GET_FLAG(fi, PI_SEVERITY_MASK)) {
        switch(FI_GET_FLAG(fi, PI_SEVERITY_MASK)) {
        case(PI_CHAT):
            item->setData(0, Qt::BackgroundRole, expert_color_chat);
            break;
        case(PI_NOTE):
            item->setData(0, Qt::BackgroundRole, expert_color_note);
            break;
        case(PI_WARN):
            item->setData(0, Qt::BackgroundRole, expert_color_warn);
            break;
        case(PI_ERROR):
            item->setData(0, Qt::BackgroundRole, expert_color_error);
            break;
        default:
            g_assert_not_reached();
        }
        item->setData(0, Qt::ForegroundRole, expert_color_foreground);
    }

    item->setText(0, label_ptr);
    item->setData(0, Qt::UserRole, qVariantFromValue(fi));

    if (PROTO_ITEM_IS_GENERATED(node) || PROTO_ITEM_IS_HIDDEN(node)) {
        g_free(label_ptr);
    }

    if (tree_is_expanded[fi->tree_type]) {
        item->setExpanded(true);
    } else {
        item->setExpanded(false);
    }

    if (!is_leaf) {
        proto_tree_children_foreach(node, proto_tree_draw_node, item);
    }
}

ProtoTree::ProtoTree(QWidget *parent) :
    QTreeWidget(parent)
{
    setAccessibleName(tr("Packet details"));
    setFont(get_monospace_font());
    setUniformRowHeights(true);

    connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(updateSelectionStatus(QTreeWidgetItem*)));
    connect(this, SIGNAL(expanded(QModelIndex)), this, SLOT(expand(QModelIndex)));
    connect(this, SIGNAL(collapsed(QModelIndex)), this, SLOT(collapse(QModelIndex)));
    connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
            this, SLOT(itemDoubleClick(QTreeWidgetItem*, int)));
}

void ProtoTree::clear() {
    updateSelectionStatus(NULL);
    QTreeWidget::clear();
}

void ProtoTree::fillProtocolTree(proto_tree *protocol_tree) {
    // Clear out previous tree
    clear();

    proto_tree_children_foreach(protocol_tree, proto_tree_draw_node, invisibleRootItem());
}

void ProtoTree::updateSelectionStatus(QTreeWidgetItem* item) {

    if (item) {
        field_info *fi;
        QString itemInfo;
        int finfo_length;

        fi = item->data(0, Qt::UserRole).value<field_info *>();
        if (!fi || !fi->hfinfo) return;

        if (fi->hfinfo->blurb != NULL && fi->hfinfo->blurb[0] != '\0') {
            itemInfo.append(QString().fromUtf8(fi->hfinfo->blurb));
        } else {
            itemInfo.append(QString().fromUtf8(fi->hfinfo->name));
        }

        if (!itemInfo.isEmpty()) {
            itemInfo.append(" (" + QString().fromUtf8(fi->hfinfo->abbrev) + ")");

            finfo_length = fi->length + fi->appendix_length;
            if (finfo_length == 1) {
                itemInfo.append(tr(", 1 byte"));
            } else if (finfo_length > 1) {
                itemInfo.append(QString(tr(", %1 bytes")).arg(finfo_length));
            }

            emit protoItemSelected(new QString());
            emit protoItemSelected(false);
            emit protoItemSelected(itemInfo);
            emit protoItemSelected(true);
        } // else the GTK+ version pushes an empty string as described below.
        /*
         * Don't show anything if the field name is zero-length;
         * the pseudo-field for "proto_tree_add_text()" is such
         * a field, and we don't want "Text (text)" showing up
         * on the status line if you've selected such a field.
         *
         * XXX - there are zero-length fields for which we *do*
         * want to show the field name.
         *
         * XXX - perhaps the name and abbrev field should be null
         * pointers rather than null strings for that pseudo-field,
         * but we'd have to add checks for null pointers in some
         * places if we did that.
         *
         * Or perhaps protocol tree items added with
         * "proto_tree_add_text()" should have -1 as the field index,
         * with no pseudo-field being used, but that might also
         * require special checks for -1 to be added.
         */

    } else {
        emit protoItemSelected(new QString());
        emit protoItemSelected(false);
    }
}

void ProtoTree::expand(const QModelIndex & index) {
    field_info *fi;

    fi = index.data(Qt::UserRole).value<field_info *>();
    g_assert(fi);

    if(prefs.gui_auto_scroll_on_expand) {
        ScrollHint scroll_hint = PositionAtTop;
        if (prefs.gui_auto_scroll_percentage > 66) {
            scroll_hint = PositionAtBottom;
        } else if (prefs.gui_auto_scroll_percentage >= 33) {
            scroll_hint = PositionAtCenter;
        }
        scrollTo(index, scroll_hint);
    }

    /*
     * Nodes with "finfo->tree_type" of -1 have no ett_ value, and
     * are thus presumably leaf nodes and cannot be expanded.
     */
    if (fi->tree_type != -1) {
        g_assert(fi->tree_type >= 0 &&
                 fi->tree_type < num_tree_types);
        tree_is_expanded[fi->tree_type] = TRUE;
    }
}

void ProtoTree::collapse(const QModelIndex & index) {
    field_info *fi;

    fi = index.data(Qt::UserRole).value<field_info *>();
    g_assert(fi);

    /*
     * Nodes with "finfo->tree_type" of -1 have no ett_ value, and
     * are thus presumably leaf nodes and cannot be collapsed.
     */
    if (fi->tree_type != -1) {
        g_assert(fi->tree_type >= 0 &&
                 fi->tree_type < num_tree_types);
        tree_is_expanded[fi->tree_type] = FALSE;
    }
}

void ProtoTree::expandSubtrees()
{
    QTreeWidgetItem *topSel;

    if (selectedItems().length() < 1) {
        return;
    }

    topSel = selectedItems()[0];

    if (!topSel) {
        return;
    }

    while (topSel->parent()) {
        topSel = topSel->parent();
    }

    QTreeWidgetItemIterator iter(topSel);
    while (*iter) {
        if ((*iter) != topSel && (*iter)->parent() == NULL) {
            // We found the next top-level item
            break;
        }
        (*iter)->setExpanded(true);
        iter++;
    }
}

void ProtoTree::expandAll()
{
    int i;
    for(i=0; i < num_tree_types; i++) {
        tree_is_expanded[i] = TRUE;
    }
    QTreeWidget::expandAll();
}

void ProtoTree::collapseAll()
{
    int i;
    for(i=0; i < num_tree_types; i++) {
        tree_is_expanded[i] = FALSE;
    }
    QTreeWidget::collapseAll();
}

void ProtoTree::itemDoubleClick(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);

    gchar *url;
    field_info *fi;

    fi = item->data(0, Qt::UserRole).value<field_info *>();

    if(fi->hfinfo->type == FT_FRAMENUM) {
        emit goToFrame(fi->value.value.uinteger);
    }

    if(FI_GET_FLAG(fi, FI_URL) && IS_FT_STRING(fi->hfinfo->type)) {
        url = fvalue_to_string_repr(&fi->value, FTREPR_DISPLAY, NULL);
        if(url){
//            browser_open_url(url);
            QDesktopServices::openUrl(QUrl(url));
            g_free(url);
        }
    }
}

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
