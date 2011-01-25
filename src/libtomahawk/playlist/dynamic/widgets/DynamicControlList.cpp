/****************************************************************************************
 * Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DynamicControlList.h"

#include <QLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPushButton>
#include <QToolButton>
#include <QPainter>
#include <QGridLayout>

#include "DynamicControlWrapper.h"
#include "dynamic/GeneratorInterface.h"
#include "tomahawk/tomahawkapp.h"
#include <QHBoxLayout>

using namespace Tomahawk;

DynamicControlList::DynamicControlList( QWidget* parent )
    : QWidget( parent )
    , m_layout( new QGridLayout )
    , m_summaryWidget( 0 )
{
    init();
}

DynamicControlList::DynamicControlList( const geninterface_ptr& generator, const QList< dyncontrol_ptr >& controls, bool isLocal, QWidget* parent )
    : QWidget( parent )
    , m_generator( generator )
    , m_layout( new QGridLayout )
    , m_summaryWidget( 0 )
    , m_isLocal( isLocal )
{
    init();
    setControls(  generator, controls, m_isLocal );
}

DynamicControlList::~DynamicControlList()
{

}

void 
DynamicControlList::init()
{
    qDebug() << "GRIDLAYOUT: " << m_layout->rowCount();
    setContentsMargins( 0, 0, 0, 0 );
    setLayout( m_layout );
    m_layout->setColumnStretch( 2, 1 );
    m_layout->setMargin( 0 );
    m_layout->setVerticalSpacing( 0 );
    m_layout->setContentsMargins( 0, 0, 0, 0 );
    m_layout->setSizeConstraint( QLayout::SetMinimumSize );
    
    m_collapseLayout = new QHBoxLayout();
    m_collapseLayout->setContentsMargins( 0, 0, 0, 0 );
    m_collapseLayout->setMargin( 0 );
    m_collapseLayout->setSpacing( 0 );
    m_collapse = new QPushButton( tr( "Click to collapse" ), this );
    m_collapseLayout->addWidget( m_collapse );
    m_addControl = new QToolButton( this );
    m_addControl->setIcon( QIcon( RESPATH "images/list-add.png" ) );
    m_addControl->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    m_addControl->setIconSize( QSize( 16, 16 ) );
    m_addControl->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_addControl->setAutoRaise( true );
    m_addControl->setContentsMargins( 0, 0, 0, 0 );
    m_collapseLayout->addWidget( m_addControl );
    m_collapse->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    
    connect( m_collapse, SIGNAL( clicked() ), this, SIGNAL( toggleCollapse() ) );
    connect( m_addControl, SIGNAL( clicked() ), this, SLOT( addNewControl() ) );
    
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
}

void 
DynamicControlList::setControls( const geninterface_ptr& generator, const QList< dyncontrol_ptr >& controls, bool isLocal )
{
    if( m_controls.size() == controls.size() && controls.size() > 0 ) { // check if we're setting the same controls we already have, and exit if we are
        bool different = false;
        for( int i = 0; i < m_controls.size(); i++ ) {
            if( m_controls.value( i )->control().data() != controls.value( i ).data() ) {
                different = true;
                break;
            }
        }
        if( !different ) { // no work to do
            return;
        }
    }
    
    if( !m_controls.isEmpty() ) {
        qDeleteAll( m_controls );
        m_controls.clear();
    }
    
    m_layout->removeItem( m_collapseLayout );
    
    m_isLocal = isLocal;
    m_generator = generator;
    if( controls.isEmpty() ) {
        qDebug() << "CREATING DEFAULT CONTROL";
        DynamicControlWrapper* ctrlW = new DynamicControlWrapper( generator->createControl(), m_layout, m_controls.size(), isLocal, this );
        connect( ctrlW, SIGNAL( removeControl() ), this, SLOT( removeControl() ) );
        connect( ctrlW, SIGNAL( changed() ), this, SLOT( controlChanged() ) );
        m_controls << ctrlW;
    } else 
    {
        foreach( const dyncontrol_ptr& control, controls ) {
            DynamicControlWrapper* ctrlW = new DynamicControlWrapper( control, m_layout, m_controls.size(), isLocal, this );
            connect( ctrlW, SIGNAL( removeControl() ), this, SLOT( removeControl() ) );
            connect( ctrlW, SIGNAL( changed() ), this, SLOT( controlChanged() ) );
            
            m_controls << ctrlW;
        }
    }
    m_layout->addItem( m_collapseLayout, m_layout->rowCount(), 0, 1, 4, Qt::AlignCenter );
    
}

void DynamicControlList::addNewControl()
{
    m_layout->removeItem( m_collapseLayout );
    
    dyncontrol_ptr control = m_generator->createControl();
    m_controls.append( new DynamicControlWrapper( control, m_layout, m_controls.size(), m_isLocal, this ) );
    connect( m_controls.last(), SIGNAL( removeControl() ), this, SLOT( removeControl() ) );
    connect( m_controls.last(), SIGNAL( changed() ), this, SLOT( controlChanged() ) );
    
    m_layout->addItem( m_collapseLayout, m_layout->rowCount(), 0, 1, 4, Qt::AlignCenter );
    emit controlsChanged();
}

void DynamicControlList::removeControl()
{
    DynamicControlWrapper* w = qobject_cast<DynamicControlWrapper*>( sender() );
    w->removeFromLayout();
    m_controls.removeAll( w );
    
    m_generator->removeControl( w->control() );
    delete w;
    
    emit controlsChanged();
}

void DynamicControlList::controlChanged()
{
    Q_ASSERT( sender() && qobject_cast<DynamicControlWrapper*>(sender()) );
    DynamicControlWrapper* widget = qobject_cast<DynamicControlWrapper*>(sender());
    
    qDebug() << "control changed!";
    foreach( DynamicControlWrapper* c, m_controls )
        qDebug() << c->control()->id() << c->control()->selectedType() << c->control()->match() << c->control()->input();
    emit controlChanged( widget->control() );
}
