/*****************************************************************************
 * ListViewController.h:
 *****************************************************************************
 * Copyright (C) 2008-2010 VideoLAN
 *
 * Authors: Thomas Boquet <thomas.boquet@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef LISTVIEWCONTROLLER_H
#define LISTVIEWCONTROLLER_H

#include <QWidget>
#include <QListView>
#include <QAbstractItemModel>
#include <QScrollArea>
#include <QUuid>

#include "ViewController.h"
#include "StackViewController.h"
#include "Clip.h"


class ListViewController : public ViewController
{
    Q_OBJECT

public:
    ListViewController() {}
    ListViewController( StackViewController* nav );
    ~ListViewController();

    QWidget*        view() const;
    const QString&  title() const;
    void            addCell( QWidget* cell );
    void            removeCell( QWidget* cell );

protected:
    QVBoxLayout*                m_layout;

private:
    QString*                    m_title;
    QScrollArea*                m_scrollArea;
    QWidget*                    m_container;

    StackViewController*        m_nav;
};

#endif // LISTVIEWCONTROLLER_H
