/*****************************************************************************
 * Media.h : Represents a basic container for media informations.
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
  * This file contains the Clip class declaration/definition.
  * It's used by the timeline in order to represent a subset of a media
  */

#ifndef CLIP_H__
# define CLIP_H__

#include <QObject>
#include <QUuid>

#include "Media.h"

class   Clip : public QObject
{
    Q_OBJECT

    public:
        static const float     UntilEndOfMedia = -1;

        Clip( Media* parent );
        Clip( Clip* creator, qint64 begin, qint64 end );
        virtual ~Clip();

        float           getBegin() const;
        float           getEnd() const;
        qint64          getLength() const;
        Media*          getParent();
        const QUuid&    getUuid() const;

    private:
        void        computeLength();
        void        init();

        Media*      m_parent;
        float       m_begin;
        float       m_end;
        qint64      m_length;
        QUuid       m_uuid;
};

#endif //CLIP_H__
