/*****************************************************************************
 * Library.h: Multimedia library
 *****************************************************************************
 * Copyright (C) 2008-2009 the VLMC team
 *
 * Authors: Hugo Beauzee-Luyssen <hugo@vlmc.org>
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

/** \file
  * This file the library contains class declaration/definition.
  * It's the the backend part of the Library widget of vlmc.
  * It can load and unload Clips (Clip.h/Clip.cpp)
  */

#ifndef LIBRARY_H
#define LIBRARY_H

#include <QObject>
#include <QHash>
#include <QUuid>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>

#include "Media.h"
#include "Clip.h"
#include "Singleton.hpp"

class   Library : public QObject, public Singleton<Library>
{
    Q_OBJECT
    Q_DISABLE_COPY( Library );
public:
    Media*                  getMedia( const QUuid& uuid );
    Clip*                   getClip( const QUuid& uudi );

private:
    Library();
    QHash<QUuid, Media*>    m_medias;
    QHash<QUuid, Clip*>     m_clips;
    QMutex                  m_mutex;
    template <typename T>
    T                       getElementByUuid( const QHash<QUuid, T>& container , const QUuid& uuid )
    {
        typename QHash<QUuid, T>::const_iterator   it = container.find( uuid );
        if ( it == container.end() )
            return NULL;
        return *it;
    }

public slots:
    void                    newMediaLoadingAsked( const QString& filePath );
    void                    removingMediaAsked( const QUuid& uuid );

private slots:
    void                    metaDataComputed( Media* );

signals:
    void                    newClipLoaded( Clip* );
    void                    newMediaLoaded( Media* );
    void                    mediaRemoved( const QUuid& );

    friend class            Singleton<Library>;
};

#endif // LIBRARY_H