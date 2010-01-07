/*****************************************************************************
 * ClipWorkflow.cpp : Clip workflow. Will extract a single frame from a VLCMedia
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

#include "vlmc.h"
#include "ClipWorkflow.h"
#include "MemoryPool.hpp"
#include "LightVideoFrame.h"
#include "Clip.h"
#include "WaitCondition.hpp"

#include <QWaitCondition>
#include <QtDebug>

ClipWorkflow::ClipWorkflow( Clip::Clip* clip ) :
                m_mediaPlayer(NULL),
                m_requiredState( ClipWorkflow::None ),
                m_clip( clip ),
                m_state( ClipWorkflow::Stopped )
{
    m_stateLock = new QReadWriteLock;
    m_requiredStateLock = new QMutex;
    m_initWaitCond = new WaitCondition;
    m_pausingStateWaitCond = new WaitCondition;
    m_renderLock = new QMutex;
    m_feedingCondWait = new WaitCondition;
    m_availableBuffersMutex = new QMutex;
    m_computedBuffersMutex = new QMutex;
}

ClipWorkflow::~ClipWorkflow()
{
    delete m_renderLock;
    delete m_pausingStateWaitCond;
    delete m_initWaitCond;
    delete m_requiredStateLock;
    delete m_stateLock;
    delete m_availableBuffersMutex;
    delete m_computedBuffersMutex;
}

void    ClipWorkflow::checkStateChange()
{
    QMutexLocker    lock( m_requiredStateLock );
    QWriteLocker    lock2( m_stateLock );
    if ( m_requiredState != ClipWorkflow::None )
    {
        m_state = m_requiredState;
//        qDebug() << '[' << (void*)this << "] Applying required state change:" << m_state;
        m_requiredState = ClipWorkflow::None;
    }
}

void    ClipWorkflow::initialize()
{
    if ( m_clip->getParent()->getFileType() == Media::Image )
        m_vlcMedia = new LibVLCpp::Media( "fake:///" + m_clip->getParent()->getFileInfo()->absoluteFilePath() );
    else
        m_vlcMedia = new LibVLCpp::Media( "file:///" + m_clip->getParent()->getFileInfo()->absoluteFilePath() );
    m_currentPts = -1;
    m_previousPts = -1;
    m_pauseDuration = -1;
    initVlcOutput();
    m_mediaPlayer = MemoryPool<LibVLCpp::MediaPlayer>::getInstance()->get();
    m_mediaPlayer->setMedia( m_vlcMedia );

    connect( m_mediaPlayer, SIGNAL( playing() ), this, SLOT( loadingComplete() ), Qt::DirectConnection );
    connect( m_mediaPlayer, SIGNAL( endReached() ), this, SLOT( clipEndReached() ), Qt::DirectConnection );
    m_mediaPlayer->play();
}

void    ClipWorkflow::loadingComplete()
{
    adjustBegin();
    disconnect( m_mediaPlayer, SIGNAL( playing() ), this, SLOT( loadingComplete() ) );
    connect( m_mediaPlayer, SIGNAL( playing() ), this, SLOT( mediaPlayerUnpaused() ) );
    connect( m_mediaPlayer, SIGNAL( paused() ), this, SLOT( mediaPlayerPaused() ) );
    QMutexLocker    lock( m_initWaitCond->getMutex() );
    setState( Rendering );
    m_initWaitCond->wake();
}

void    ClipWorkflow::adjustBegin()
{
    if ( m_clip->getParent()->getFileType() == Media::Video || m_clip->getParent()->getFileType() == Media::Audio )
        m_mediaPlayer->setTime( m_clip->getBegin() / m_clip->getParent()->getFps() * 1000 );
}

bool    ClipWorkflow::isEndReached() const
{
    QReadLocker lock( m_stateLock );
    return m_state == ClipWorkflow::EndReached;
}

bool    ClipWorkflow::isStopped() const
{
    QReadLocker lock( m_stateLock );
    return m_state == ClipWorkflow::Stopped;
}

ClipWorkflow::State     ClipWorkflow::getState() const
{
    return m_state;
}

void    ClipWorkflow::clipEndReached()
{
    setState( EndReached );
}

Clip*     ClipWorkflow::getClip()
{
    return m_clip;
}

void            ClipWorkflow::stop()
{
    if ( m_mediaPlayer )
    {
        m_mediaPlayer->stop();
        disconnect( m_mediaPlayer, SIGNAL( endReached() ), this, SLOT( clipEndReached() ) );
        MemoryPool<LibVLCpp::MediaPlayer>::getInstance()->release( m_mediaPlayer );
        m_mediaPlayer = NULL;
        setState( Stopped );
        QMutexLocker    lock( m_requiredStateLock );
        m_requiredState = ClipWorkflow::None;
        delete m_vlcMedia;
    }
    else
        qDebug() << "ClipWorkflow has already been stopped";
}

void            ClipWorkflow::setTime( qint64 time )
{
    m_mediaPlayer->setTime( time );
    flushComputedBuffers();
}

bool            ClipWorkflow::isRendering() const
{
    QReadLocker lock( m_stateLock );
    return m_state == ClipWorkflow::Rendering;
}

void            ClipWorkflow::setState( State state )
{
    QWriteLocker    lock( m_stateLock );
//        qDebug() << '[' << (void*)this << "] Setting state to" << state;
    m_state = state;
}

void            ClipWorkflow::queryStateChange( State newState )
{
    QMutexLocker    lock( m_requiredStateLock );
    m_requiredState = newState;
}

QReadWriteLock* ClipWorkflow::getStateLock()
{
    return m_stateLock;
}

void        ClipWorkflow::waitForCompleteInit()
{
    if ( isRendering() == false )
    {
        QMutexLocker    lock( m_initWaitCond->getMutex() );
        m_initWaitCond->waitLocked();
    }
}

LibVLCpp::MediaPlayer*       ClipWorkflow::getMediaPlayer()
{
    return m_mediaPlayer;
}

void        ClipWorkflow::preGetOutput()
{
    QMutexLocker    lock( m_feedingCondWait->getMutex() );

    //Computed buffer mutex is already locked by underlying clipworkflow getoutput method
    if ( getNbComputedBuffers() == 0 )
    {
//        qWarning() << "Waiting for buffer to be fed";
        m_renderLock->unlock();
        m_computedBuffersMutex->unlock();
//        qDebug() << "Unlocked render lock, entering cond wait";
        m_feedingCondWait->waitLocked();
        m_computedBuffersMutex->lock();
        m_renderLock->lock();
    }
}

void        ClipWorkflow::postGetOutput()
{
    //If we're running out of computed buffers, refill our stack.
    if ( getNbComputedBuffers() < getMaxComputedBuffers() / 3 )
    {
        QWriteLocker        lock( m_stateLock );
        if ( m_state == ClipWorkflow::Paused )
        {
            qWarning() << "Unpausing media player. type:" << debugType;
            m_state = ClipWorkflow::UnpauseRequired;
//            This will act like an "unpause";
            m_mediaPlayer->pause();
        }
//        else
//            qCritical() << "Running out of computed buffers !";
    }
}

void        ClipWorkflow::commonUnlock()
{
    //Don't test using availableBuffer, as it may evolve if a buffer is required while
    //no one is available : we would spawn a new buffer, thus modifying the number of available buffers
    if ( getNbComputedBuffers() == getMaxComputedBuffers() )
    {
        qWarning() << "Pausing clip workflow. Type:" << debugType;
        setState( ClipWorkflow::PauseRequired );
        m_mediaPlayer->pause();
    }
    if ( getNbComputedBuffers() == 1 )
    {
//        qDebug() << "Waking feeding cont wait... acquiring lock. Type:" << debugType;
        QMutexLocker    lock( m_feedingCondWait->getMutex() );
//        qDebug() << "feeding cont wait mutex acquired. Type:" << debugType;
        m_feedingCondWait->wake();
    }
    checkStateChange();
}

void    ClipWorkflow::computePtsDiff( qint64 pts )
{
    if ( m_previousPts == -1 )
        m_previousPts = pts;
    if ( m_currentPts == -1 )
        m_currentPts = pts;
    if ( m_pauseDuration != -1 )
    {
        m_previousPts = m_currentPts + m_pauseDuration;
        m_pauseDuration = -1;
    }
    else
        m_previousPts = m_currentPts;
    m_currentPts = qMax( pts, m_previousPts );
}

void    ClipWorkflow::mediaPlayerPaused()
{
    qWarning() << "\n\n\n\nMedia player paused, waiting for buffers to be consumed.Type:" << debugType << "\n\n\n\n";
    setState( ClipWorkflow::Paused );
    m_beginPausePts = mdate();
//    qDebug() << "got pause pts:" << m_beginPausePts;
}

void    ClipWorkflow::mediaPlayerUnpaused()
{
    qWarning() << "Media player unpaused. Go back to rendering. Type:" << debugType;
    setState( ClipWorkflow::Rendering );
    m_pauseDuration = mdate() - m_beginPausePts;
}
