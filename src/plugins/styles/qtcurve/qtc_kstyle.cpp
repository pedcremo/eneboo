/*
 *
 * QtCKStyle
 * Copyright (C) 2001-2002 Karol Szwed <gallium@kde.org>
 *
 * QWindowsStyle CC_ListView and style images were kindly donated by TrollTech,
 * Copyright (C) 1998-2000 TrollTech AS.
 *
 * Many thanks to Bradley T. Hughes for the 3 button scrollbar code.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "qtc_kstyle.h"

#include <qapplication.h>
#include <qbitmap.h>
#include <qcleanuphandler.h>
#include <qmap.h>
#include <qimage.h>
#include <qlistview.h>
#include <qmenubar.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpopupmenu.h>
#include <qprogressbar.h>
#include <qscrollbar.h>
#include <qsettings.h>
#include <qslider.h>
#include <qstylefactory.h>
#include <qtabbar.h>
#include <qtoolbar.h>

#ifdef Q_WS_X11
# include <X11/Xlib.h>
# ifdef HAVE_XRENDER
#  include <X11/extensions/Xrender.h> // schroder
   extern bool qt_use_xrender;
# endif
#else
#undef HAVE_XRENDER
#endif


#include <limits.h>

struct QtCKStylePrivate
{
	bool  highcolor                : 1;
	bool  useFilledFrameWorkaround : 1;
	bool  etchDisabledText         : 1;
	bool  scrollablePopupmenus     : 1;
	bool  menuAltKeyNavigation     : 1;
	bool  menuDropShadow           : 1;
	bool  sloppySubMenus           : 1;
	int   popupMenuDelay;
	float menuOpacity;

	QtCKStyle::KStyleScrollBarType  scrollbarType;
	QtCKStyle::KStyleFlags flags;
	
	//For KPE_ListViewBranch
	QBitmap *verticalLine;
	QBitmap *horizontalLine;
};

// -----------------------------------------------------------------------------


QtCKStyle::QtCKStyle( KStyleFlags flags, KStyleScrollBarType sbtype )
	: QCommonStyle(), d(new QtCKStylePrivate)
{
	d->flags = flags;
	d->useFilledFrameWorkaround = (flags & FilledFrameWorkaround);
	d->scrollbarType = sbtype;
	d->highcolor = QPixmap::defaultDepth() > 8;

	// Read style settings
	QSettings settings;
	d->popupMenuDelay       = settings.readNumEntry ("/QtCKStyle/Settings/PopupMenuDelay", 256);
	d->sloppySubMenus       = settings.readBoolEntry("/QtCKStyle/Settings/SloppySubMenus", false);
	d->etchDisabledText     = settings.readBoolEntry("/QtCKStyle/Settings/EtchDisabledText", true);
	d->menuAltKeyNavigation = settings.readBoolEntry("/QtCKStyle/Settings/MenuAltKeyNavigation", true);
	d->scrollablePopupmenus = settings.readBoolEntry("/QtCKStyle/Settings/ScrollablePopupMenus", false);
	d->menuDropShadow       = settings.readBoolEntry("/QtCKStyle/Settings/MenuDropShadow", false);
	
	d->verticalLine   = 0;
	d->horizontalLine = 0;
}


QtCKStyle::~QtCKStyle()
{
	delete d->verticalLine;
	delete d->horizontalLine;

	delete d;
}


QString QtCKStyle::defaultStyle()
{
	if (QPixmap::defaultDepth() > 8)
	   return QString("plastik");
	else
	   return QString("light, 3rd revision");
}


void QtCKStyle::polish( QWidget* widget )
{
	if ( d->useFilledFrameWorkaround )
	{
		if ( QFrame *frame = ::qt_cast< QFrame* >( widget ) ) {
			QFrame::Shape shape = frame->frameShape();
			if (shape == QFrame::ToolBarPanel || shape == QFrame::MenuBarPanel)
				widget->installEventFilter(this);
		} 
	}
}


void QtCKStyle::unPolish( QWidget* widget )
{
	if ( d->useFilledFrameWorkaround )
	{
		if ( QFrame *frame = ::qt_cast< QFrame* >( widget ) ) {
			QFrame::Shape shape = frame->frameShape();
			if (shape == QFrame::ToolBarPanel || shape == QFrame::MenuBarPanel)
				widget->removeEventFilter(this);
		} 
	}
}


// Style changes (should) always re-polish popups.
void QtCKStyle::polishPopupMenu( QPopupMenu* p )
{
	if (!p->testWState( WState_Polished ))
		p->setCheckable(true);
}


// -----------------------------------------------------------------------------
// QtCKStyle extensions
// -----------------------------------------------------------------------------

void QtCKStyle::setScrollBarType(KStyleScrollBarType sbtype)
{
	d->scrollbarType = sbtype;
}

QtCKStyle::KStyleFlags QtCKStyle::styleFlags() const
{
	return d->flags;
}

void QtCKStyle::drawKStylePrimitive( KStylePrimitive kpe,
								  QPainter* p,
								  const QWidget* widget,
								  const QRect &r,
								  const QColorGroup &cg,
								  SFlags flags,
								  const QStyleOption& /* opt */ ) const
{
	switch( kpe )
	{
		// Dock / Toolbar / General handles.
		// ---------------------------------

		case KPE_DockWindowHandle: {

			// Draws a nice DockWindow handle including the dock title.
			QWidget* wid = const_cast<QWidget*>(widget);
			bool horizontal = flags & Style_Horizontal;
			int x,y,w,h,x2,y2;

			r.rect( &x, &y, &w, &h );
			if ((w <= 2) || (h <= 2)) {
				p->fillRect(r, cg.highlight());
				return;
			}

			
			x2 = x + w - 1;
			y2 = y + h - 1;

			QFont fnt;
			fnt = QApplication::font(wid);
			fnt.setPointSize( fnt.pointSize()-2 );

			// Draw the item on an off-screen pixmap
			// to preserve Xft antialiasing for
			// vertically oriented handles.
			QPixmap pix;
			if (horizontal)
				pix.resize( h-2, w-2 );
			else
				pix.resize( w-2, h-2 );

			QString title = wid->parentWidget()->caption();
			QPainter p2;
			p2.begin(&pix);
			p2.fillRect(pix.rect(), cg.brush(QColorGroup::Highlight));
			p2.setPen(cg.highlightedText());
			p2.setFont(fnt);
			p2.drawText(pix.rect(), AlignCenter, title);
			p2.end();

			// Draw a sunken bevel
			p->setPen(cg.dark());
			p->drawLine(x, y, x2, y);
			p->drawLine(x, y, x, y2);
			p->setPen(cg.light());
			p->drawLine(x+1, y2, x2, y2);
			p->drawLine(x2, y+1, x2, y2);

			if (horizontal) {
				QWMatrix m;
				m.rotate(-90.0);
				QPixmap vpix = pix.xForm(m);
				bitBlt(wid, r.x()+1, r.y()+1, &vpix);
			} else
				bitBlt(wid, r.x()+1, r.y()+1, &pix);

			break;
		}


		/*
		 * KPE_ListViewExpander and KPE_ListViewBranch are based on code from
		 * QWindowStyle's CC_ListView, kindly donated by TrollTech.
		 * CC_ListView code is Copyright (C) 1998-2000 TrollTech AS.
		 */

		case KPE_ListViewExpander: {
			// Typical Windows style expand/collapse element.
			int radius = (r.width() - 4) / 2;
			int centerx = r.x() + r.width()/2;
			int centery = r.y() + r.height()/2;

			// Outer box
			p->setPen( cg.mid() );
			p->drawRect( r );

			// plus or minus
			p->setPen( cg.text() );
			p->drawLine( centerx - radius, centery, centerx + radius, centery );
			if ( flags & Style_On )	// Collapsed = On
				p->drawLine( centerx, centery - radius, centerx, centery + radius );
			break;
		}

		case KPE_ListViewBranch: {
			// Typical Windows style listview branch element (dotted line).

			// Create the dotline pixmaps if not already created
			if ( !d->verticalLine )
			{
				// make 128*1 and 1*128 bitmaps that can be used for
				// drawing the right sort of lines.
				d->verticalLine   = new QBitmap( 1, 129, true );
				d->horizontalLine = new QBitmap( 128, 1, true );
				QPointArray a( 64 );
				QPainter p2;
				p2.begin( d->verticalLine );

				int i;
				for( i=0; i < 64; i++ )
					a.setPoint( i, 0, i*2+1 );
				p2.setPen( color1 );
				p2.drawPoints( a );
				p2.end();
				QApplication::flushX();
				d->verticalLine->setMask( *d->verticalLine );

				p2.begin( d->horizontalLine );
				for( i=0; i < 64; i++ )
					a.setPoint( i, i*2+1, 0 );
				p2.setPen( color1 );
				p2.drawPoints( a );
				p2.end();
				QApplication::flushX();
				d->horizontalLine->setMask( *d->horizontalLine );
			}

			p->setPen( cg.text() );		// cg.dark() is bad for dark color schemes.

			if (flags & Style_Horizontal)
			{
				int point = r.x();
				int other = r.y();
				int end = r.x()+r.width();
				int thickness = r.height();

				while( point < end )
				{
					int i = 128;
					if ( i+point > end )
						i = end-point;
					p->drawPixmap( point, other, *d->horizontalLine, 0, 0, i, thickness );
					point += i;
				}

			} else {
				int point = r.y();
				int other = r.x();
				int end = r.y()+r.height();
				int thickness = r.width();
				int pixmapoffset = (flags & Style_NoChange) ? 0 : 1;	// ### Hackish

				while( point < end )
				{
					int i = 128;
					if ( i+point > end )
						i = end-point;
					p->drawPixmap( other, point, *d->verticalLine, 0, pixmapoffset, thickness, i );
					point += i;
				}
			}

			break;
		}

		// Reimplement the other primitives in your styles.
		// The current implementation just paints something visibly different.
		case KPE_ToolBarHandle:
		case KPE_GeneralHandle:
		case KPE_SliderHandle:
			p->fillRect(r, cg.light());
			break;

		case KPE_SliderGroove:
			p->fillRect(r, cg.dark());
			break;

		default:
			p->fillRect(r, Qt::yellow);	// Something really bad happened - highlight.
			break;
	}
}


int QtCKStyle::kPixelMetric( KStylePixelMetric kpm, const QWidget* /* widget */) const
{
	int value;
	switch(kpm)
	{
		case KPM_ListViewBranchThickness:
			value = 1;
			break;

		case KPM_MenuItemSeparatorHeight:
		case KPM_MenuItemHMargin:
		case KPM_MenuItemVMargin:
		case KPM_MenuItemHFrame:
		case KPM_MenuItemVFrame:
		case KPM_MenuItemCheckMarkHMargin:
		case KPM_MenuItemArrowHMargin:
		case KPM_MenuItemTabSpacing:
		default:
			value = 0;
	}

	return value;
}


// -----------------------------------------------------------------------------

void QtCKStyle::drawPrimitive( PrimitiveElement pe,
							QPainter* p,
							const QRect &r,
							const QColorGroup &cg,
							SFlags flags,
							const QStyleOption& opt ) const
{
	// TOOLBAR/DOCK WINDOW HANDLE
	// ------------------------------------------------------------------------
	if (pe == PE_DockWindowHandle)
	{
		// Wild workarounds are here. Beware.
		QWidget *widget, *parent;

		if (p && p->device()->devType() == QInternal::Widget) {
			widget = static_cast<QWidget*>(p->device());
			parent = widget->parentWidget();
		} else
			return;		// Don't paint on non-widgets

		// Check if we are a normal toolbar or a hidden dockwidget.
		if ( parent &&
			(parent->inherits("QToolBar") ||		// Normal toolbar
			(parent->inherits("QMainWindow")) ))	// Collapsed dock

			// Draw a toolbar handle
			drawKStylePrimitive( KPE_ToolBarHandle, p, widget, r, cg, flags, opt );

		else if ( widget->inherits("QDockWindowHandle") )

			// Draw a dock window handle
			drawKStylePrimitive( KPE_DockWindowHandle, p, widget, r, cg, flags, opt );

		else
			// General handle, probably a kicker applet handle.
			drawKStylePrimitive( KPE_GeneralHandle, p, widget, r, cg, flags, opt );

	} else
		QCommonStyle::drawPrimitive( pe, p, r, cg, flags, opt );
}



void QtCKStyle::drawControl( ControlElement element,
						  QPainter* p,
						  const QWidget* widget,
						  const QRect &r,
						  const QColorGroup &cg,
						  SFlags flags,
						  const QStyleOption &opt ) const
{
	switch (element)
	{
		// TABS
		// ------------------------------------------------------------------------
		case CE_TabBarTab: {
			const QTabBar* tb  = (const QTabBar*) widget;
			QTabBar::Shape tbs = tb->shape();
			bool selected      = flags & Style_Selected;
			int x = r.x(), y=r.y(), bottom=r.bottom(), right=r.right();

			switch (tbs) {

				case QTabBar::RoundedAbove: {
					if (!selected)
						p->translate(0,1);
					p->setPen(selected ? cg.light() : cg.shadow());
					p->drawLine(x, y+4, x, bottom);
					p->drawLine(x, y+4, x+4, y);
					p->drawLine(x+4, y, right-1, y);
					if (selected)
						p->setPen(cg.shadow());
					p->drawLine(right, y+1, right, bottom);

					p->setPen(cg.midlight());
					p->drawLine(x+1, y+4, x+1, bottom);
					p->drawLine(x+1, y+4, x+4, y+1);
					p->drawLine(x+5, y+1, right-2, y+1);

					if (selected) {
						p->setPen(cg.mid());
						p->drawLine(right-1, y+1, right-1, bottom);
					} else {
						p->setPen(cg.mid());
						p->drawPoint(right-1, y+1);
						p->drawLine(x+4, y+2, right-1, y+2);
						p->drawLine(x+3, y+3, right-1, y+3);
						p->fillRect(x+2, y+4, r.width()-3, r.height()-6, cg.mid());

						p->setPen(cg.light());
						p->drawLine(x, bottom-1, right, bottom-1);
						p->translate(0,-1);
					}
					break;
				}

				case QTabBar::RoundedBelow: {
					if (!selected)
						p->translate(0,-1);
					p->setPen(selected ? cg.light() : cg.shadow());
					p->drawLine(x, bottom-4, x, y);
					if (selected)
						p->setPen(cg.mid());
					p->drawLine(x, bottom-4, x+4, bottom);
					if (selected)
						p->setPen(cg.shadow());
					p->drawLine(x+4, bottom, right-1, bottom);
					p->drawLine(right, bottom-1, right, y);

					p->setPen(cg.midlight());
					p->drawLine(x+1, bottom-4, x+1, y);
					p->drawLine(x+1, bottom-4, x+4, bottom-1);
					p->drawLine(x+5, bottom-1, right-2, bottom-1);

					if (selected) {
						p->setPen(cg.mid());
						p->drawLine(right-1, y, right-1, bottom-1);
					} else {
						p->setPen(cg.mid());
						p->drawPoint(right-1, bottom-1);
						p->drawLine(x+4, bottom-2, right-1, bottom-2);
						p->drawLine(x+3, bottom-3, right-1, bottom-3);
						p->fillRect(x+2, y+2, r.width()-3, r.height()-6, cg.mid());
						p->translate(0,1);
						p->setPen(cg.dark());
						p->drawLine(x, y, right, y);
					}
					break;
				}

				case QTabBar::TriangularAbove: {
					if (!selected)
						p->translate(0,1);
					p->setPen(selected ? cg.light() : cg.shadow());
					p->drawLine(x, bottom, x, y+6);
					p->drawLine(x, y+6, x+6, y);
					p->drawLine(x+6, y, right-6, y);
					if (selected)
						p->setPen(cg.mid());
					p->drawLine(right-5, y+1, right-1, y+5);
					p->setPen(cg.shadow());
					p->drawLine(right, y+6, right, bottom);

					p->setPen(cg.midlight());
					p->drawLine(x+1, bottom, x+1, y+6);
					p->drawLine(x+1, y+6, x+6, y+1);
					p->drawLine(x+6, y+1, right-6, y+1);
					p->drawLine(right-5, y+2, right-2, y+5);
					p->setPen(cg.mid());
					p->drawLine(right-1, y+6, right-1, bottom);

					QPointArray a(6);
					a.setPoint(0, x+2, bottom);
					a.setPoint(1, x+2, y+7);
					a.setPoint(2, x+7, y+2);
					a.setPoint(3, right-7, y+2);
					a.setPoint(4, right-2, y+7);
					a.setPoint(5, right-2, bottom);
					p->setPen  (selected ? cg.background() : cg.mid());
					p->setBrush(selected ? cg.background() : cg.mid());
					p->drawPolygon(a);
					p->setBrush(NoBrush);
					if (!selected) {
						p->translate(0,-1);
						p->setPen(cg.light());
						p->drawLine(x, bottom, right, bottom);
					}
					break;
				}

				default: { // QTabBar::TriangularBelow
					if (!selected)
						p->translate(0,-1);
					p->setPen(selected ? cg.light() : cg.shadow());
					p->drawLine(x, y, x, bottom-6);
					if (selected)
						p->setPen(cg.mid());
					p->drawLine(x, bottom-6, x+6, bottom);
					if (selected)
						p->setPen(cg.shadow());
					p->drawLine(x+6, bottom, right-6, bottom);
					p->drawLine(right-5, bottom-1, right-1, bottom-5);
					if (!selected)
						p->setPen(cg.shadow());
					p->drawLine(right, bottom-6, right, y);

					p->setPen(cg.midlight());
					p->drawLine(x+1, y, x+1, bottom-6);
					p->drawLine(x+1, bottom-6, x+6, bottom-1);
					p->drawLine(x+6, bottom-1, right-6, bottom-1);
					p->drawLine(right-5, bottom-2, right-2, bottom-5);
					p->setPen(cg.mid());
					p->drawLine(right-1, bottom-6, right-1, y);

					QPointArray a(6);
					a.setPoint(0, x+2, y);
					a.setPoint(1, x+2, bottom-7);
					a.setPoint(2, x+7, bottom-2);
					a.setPoint(3, right-7, bottom-2);
					a.setPoint(4, right-2, bottom-7);
					a.setPoint(5, right-2, y);
					p->setPen  (selected ? cg.background() : cg.mid());
					p->setBrush(selected ? cg.background() : cg.mid());
					p->drawPolygon(a);
					p->setBrush(NoBrush);
					if (!selected) {
						p->translate(0,1);
						p->setPen(cg.dark());
						p->drawLine(x, y, right, y);
					}
					break;
				}
			};

			break;
		}
		
		// Popup menu scroller
		// ------------------------------------------------------------------------
		case CE_PopupMenuScroller: {
			p->fillRect(r, cg.background());
			drawPrimitive(PE_ButtonTool, p, r, cg, Style_Enabled);
			drawPrimitive((flags & Style_Up) ? PE_ArrowUp : PE_ArrowDown, p, r, cg, Style_Enabled);
			break;
		}


		// PROGRESSBAR
		// ------------------------------------------------------------------------
		case CE_ProgressBarGroove: {
			QRect fr = subRect(SR_ProgressBarGroove, widget);
			drawPrimitive(PE_Panel, p, fr, cg, Style_Sunken, QStyleOption::Default);
			break;
		}

		case CE_ProgressBarContents: {
			// ### Take into account totalSteps() for busy indicator
			const QProgressBar* pb = (const QProgressBar*)widget;
			QRect cr = subRect(SR_ProgressBarContents, widget);
			double progress = pb->progress();
			bool reverse = QApplication::reverseLayout();
			int steps = pb->totalSteps();

			if (!cr.isValid())
				return;

			// Draw progress bar
			if (progress > 0 || steps == 0) {
				double pg = (steps == 0) ? 0.1 : progress / steps;
				int width = QMIN(cr.width(), (int)(pg * cr.width()));
				if (steps == 0) { //Busy indicator

					if (width < 1) width = 1; //A busy indicator with width 0 is kind of useless

					int remWidth = cr.width() - width; //Never disappear completely
					if (remWidth <= 0) remWidth = 1; //Do something non-crashy when too small...

					int pstep =  int(progress) % ( 2 *  remWidth );

					if ( pstep > remWidth ) {
						//Bounce about.. We're remWidth + some delta, we want to be remWidth - delta...
						// - ( (remWidth + some delta) - 2* remWidth )  = - (some deleta - remWidth) = remWidth - some delta..
						pstep = - (pstep - 2 * remWidth );
					}

					if (reverse)
						p->fillRect(cr.x() + cr.width() - width - pstep, cr.y(), width, cr.height(),
									cg.brush(QColorGroup::Highlight));
					else
						p->fillRect(cr.x() + pstep, cr.y(), width, cr.height(),
									cg.brush(QColorGroup::Highlight));

					return;
				}


				// Do fancy gradient for highcolor displays
// 				if (d->highcolor) {
// 					QColor c(cg.highlight());
// 					KPixmap pix;
// 					pix.resize(cr.width(), cr.height());
// 					KPixmapEffect::gradient(pix, reverse ? c.light(150) : c.dark(150),
// 											reverse ? c.dark(150) : c.light(150),
// 											KPixmapEffect::HorizontalGradient);
// 					if (reverse)
// 						p->drawPixmap(cr.x()+(cr.width()-width), cr.y(), pix,
// 									  cr.width()-width, 0, width, cr.height());
// 					else
// 						p->drawPixmap(cr.x(), cr.y(), pix, 0, 0, width, cr.height());
// 				} else
					if (reverse)
						p->fillRect(cr.x()+(cr.width()-width), cr.y(), width, cr.height(),
									cg.brush(QColorGroup::Highlight));
					else
						p->fillRect(cr.x(), cr.y(), width, cr.height(),
									cg.brush(QColorGroup::Highlight));
			}
			break;
		}

		case CE_ProgressBarLabel: {
			const QProgressBar* pb = (const QProgressBar*)widget;
			QRect cr = subRect(SR_ProgressBarContents, widget);
			double progress = pb->progress();
			bool reverse = QApplication::reverseLayout();
			int steps = pb->totalSteps();

			if (!cr.isValid())
				return;

			QFont font = p->font();
			font.setBold(true);
			p->setFont(font);

			// Draw label
			if (progress > 0 || steps == 0) {
				double pg = (steps == 0) ? 1.0 : progress / steps;
				int width = QMIN(cr.width(), (int)(pg * cr.width()));
				QRect crect;
				if (reverse)
					crect.setRect(cr.x()+(cr.width()-width), cr.y(), cr.width(), cr.height());
				else
					crect.setRect(cr.x()+width, cr.y(), cr.width(), cr.height());
					
				p->save();
				p->setPen(pb->isEnabled() ? (reverse ? cg.text() : cg.highlightedText()) : cg.text());
				p->drawText(r, AlignCenter, pb->progressString());
				p->setClipRect(crect);
				p->setPen(reverse ? cg.highlightedText() : cg.text());
				p->drawText(r, AlignCenter, pb->progressString());
				p->restore();

			} else {
				p->setPen(cg.text());
				p->drawText(r, AlignCenter, pb->progressString());
			}

			break;
		}

		default:
			QCommonStyle::drawControl(element, p, widget, r, cg, flags, opt);
	}
}


QRect QtCKStyle::subRect(SubRect r, const QWidget* widget) const
{
	switch(r)
	{
		// KDE2 look smooth progress bar
		// ------------------------------------------------------------------------
		case SR_ProgressBarGroove:
			return widget->rect();

		case SR_ProgressBarContents:
		case SR_ProgressBarLabel: {
			// ### take into account indicatorFollowsStyle()
			QRect rt = widget->rect();
			return QRect(rt.x()+2, rt.y()+2, rt.width()-4, rt.height()-4);
		}

		default:
			return QCommonStyle::subRect(r, widget);
	}
}


int QtCKStyle::pixelMetric(PixelMetric m, const QWidget* widget) const
{
	switch(m)
	{
		// BUTTONS
		// ------------------------------------------------------------------------
		case PM_ButtonShiftHorizontal:		// Offset by 1
		case PM_ButtonShiftVertical:		// ### Make configurable
			return 1;

		case PM_DockWindowHandleExtent:
		{
			QWidget* parent = 0;
			// Check that we are not a normal toolbar or a hidden dockwidget,
			// in which case we need to adjust the height for font size
			if (widget && (parent = widget->parentWidget() )
				&& !parent->inherits("QToolBar")
				&& !parent->inherits("QMainWindow")
				&& widget->inherits("QDockWindowHandle") )
					return widget->fontMetrics().lineSpacing();
			else
				return QCommonStyle::pixelMetric(m, widget);
		}

		// TABS
		// ------------------------------------------------------------------------
		case PM_TabBarTabHSpace:
			return 24;

		case PM_TabBarTabVSpace: {
			const QTabBar * tb = (const QTabBar *) widget;
			if ( tb->shape() == QTabBar::RoundedAbove ||
				 tb->shape() == QTabBar::RoundedBelow )
				return 10;
			else
				return 4;
		}

		case PM_TabBarTabOverlap: {
			const QTabBar* tb = (const QTabBar*)widget;
			QTabBar::Shape tbs = tb->shape();

			if ( (tbs == QTabBar::RoundedAbove) ||
				 (tbs == QTabBar::RoundedBelow) )
				return 0;
			else
				return 2;
		}

		// SLIDER
		// ------------------------------------------------------------------------
		case PM_SliderLength:
			return 18;

		case PM_SliderThickness:
			return 24;

		// Determines how much space to leave for the actual non-tickmark
		// portion of the slider.
		case PM_SliderControlThickness: {
			const QSlider* slider   = (const QSlider*)widget;
			QSlider::TickSetting ts = slider->tickmarks();
			int thickness = (slider->orientation() == Horizontal) ?
							 slider->height() : slider->width();
			switch (ts) {
				case QSlider::NoMarks:				// Use total area.
					break;
				case QSlider::Both:
					thickness = (thickness/2) + 3;	// Use approx. 1/2 of area.
					break;
				default:							// Use approx. 2/3 of area
					thickness = ((thickness*2)/3) + 3;
					break;
			};
			return thickness;
		}

		// SPLITTER
		// ------------------------------------------------------------------------
		case PM_SplitterWidth:
			if (widget && widget->inherits("QDockWindowResizeHandle"))
				return 8;	// ### why do we need 2pix extra?
			else
				return 6;

		// FRAMES
		// ------------------------------------------------------------------------
		case PM_MenuBarFrameWidth:
			return 1;

		case PM_DockWindowFrameWidth:
			return 1;

		// GENERAL
		// ------------------------------------------------------------------------
		case PM_MaximumDragDistance:
			return -1;

		case PM_MenuBarItemSpacing:
			return 5;

		case PM_ToolBarItemSpacing:
			return 0;

		case PM_PopupMenuScrollerHeight:
			return pixelMetric( PM_ScrollBarExtent, 0);

		default:
			return QCommonStyle::pixelMetric( m, widget );
	}
}

//Helper to find the next sibling that's not hidden
static QListViewItem* nextVisibleSibling(QListViewItem* item)
{
    QListViewItem* sibling = item;
    do
    {
        sibling = sibling->nextSibling();
    }
    while (sibling && !sibling->isVisible());
    
    return sibling;
}

void QtCKStyle::drawComplexControl( ComplexControl control,
								 QPainter* p,
								 const QWidget* widget,
								 const QRect &r,
								 const QColorGroup &cg,
								 SFlags flags,
								 SCFlags controls,
								 SCFlags active,
								 const QStyleOption &opt ) const
{
	switch(control)
	{
		// 3 BUTTON SCROLLBAR
		// ------------------------------------------------------------------------
		case CC_ScrollBar: {
			// Many thanks to Brad Hughes for contributing this code.
			bool useThreeButtonScrollBar = (d->scrollbarType & ThreeButtonScrollBar);

			const QScrollBar *sb = (const QScrollBar*)widget;
			bool   maxedOut   = (sb->minValue()    == sb->maxValue());
			bool   horizontal = (sb->orientation() == Qt::Horizontal);
			SFlags sflags     = ((horizontal ? Style_Horizontal : Style_Default) |
								 (maxedOut   ? Style_Default : Style_Enabled));

			QRect  addline, subline, subline2, addpage, subpage, slider, first, last;
			subline = querySubControlMetrics(control, widget, SC_ScrollBarSubLine, opt);
			addline = querySubControlMetrics(control, widget, SC_ScrollBarAddLine, opt);
			subpage = querySubControlMetrics(control, widget, SC_ScrollBarSubPage, opt);
			addpage = querySubControlMetrics(control, widget, SC_ScrollBarAddPage, opt);
			slider  = querySubControlMetrics(control, widget, SC_ScrollBarSlider,  opt);
			first   = querySubControlMetrics(control, widget, SC_ScrollBarFirst,   opt);
			last    = querySubControlMetrics(control, widget, SC_ScrollBarLast,    opt);
			subline2 = addline;

			if ( useThreeButtonScrollBar )
				if (horizontal)
					subline2.moveBy(-addline.width(), 0);
				else
					subline2.moveBy(0, -addline.height());

			// Draw the up/left button set
			if ((controls & SC_ScrollBarSubLine) && subline.isValid()) {
				drawPrimitive(PE_ScrollBarSubLine, p, subline, cg,
							sflags | (active == SC_ScrollBarSubLine ?
								Style_Down : Style_Default));

				if (useThreeButtonScrollBar && subline2.isValid())
					drawPrimitive(PE_ScrollBarSubLine, p, subline2, cg,
							sflags | (active == SC_ScrollBarSubLine ?
								Style_Down : Style_Default));
			}

			if ((controls & SC_ScrollBarAddLine) && addline.isValid())
				drawPrimitive(PE_ScrollBarAddLine, p, addline, cg,
							sflags | ((active == SC_ScrollBarAddLine) ?
										Style_Down : Style_Default));

			if ((controls & SC_ScrollBarSubPage) && subpage.isValid())
				drawPrimitive(PE_ScrollBarSubPage, p, subpage, cg,
							sflags | ((active == SC_ScrollBarSubPage) ?
										Style_Down : Style_Default));

			if ((controls & SC_ScrollBarAddPage) && addpage.isValid())
				drawPrimitive(PE_ScrollBarAddPage, p, addpage, cg,
							sflags | ((active == SC_ScrollBarAddPage) ?
										Style_Down : Style_Default));

			if ((controls & SC_ScrollBarFirst) && first.isValid())
				drawPrimitive(PE_ScrollBarFirst, p, first, cg,
							sflags | ((active == SC_ScrollBarFirst) ?
										Style_Down : Style_Default));

			if ((controls & SC_ScrollBarLast) && last.isValid())
				drawPrimitive(PE_ScrollBarLast, p, last, cg,
							sflags | ((active == SC_ScrollBarLast) ?
										Style_Down : Style_Default));

			if ((controls & SC_ScrollBarSlider) && slider.isValid()) {
				drawPrimitive(PE_ScrollBarSlider, p, slider, cg,
							sflags | ((active == SC_ScrollBarSlider) ?
										Style_Down : Style_Default));
				// Draw focus rect
				if (sb->hasFocus()) {
					QRect fr(slider.x() + 2, slider.y() + 2,
							 slider.width() - 5, slider.height() - 5);
					drawPrimitive(PE_FocusRect, p, fr, cg, Style_Default);
				}
			}
			break;
		}


		// SLIDER
		// -------------------------------------------------------------------
		case CC_Slider: {
			const QSlider* slider = (const QSlider*)widget;
			QRect groove = querySubControlMetrics(CC_Slider, widget, SC_SliderGroove, opt);
			QRect handle = querySubControlMetrics(CC_Slider, widget, SC_SliderHandle, opt);

			// Double-buffer slider for no flicker
			QPixmap pix(widget->size());
			QPainter p2;
			p2.begin(&pix);

			if ( slider->parentWidget() &&
				 slider->parentWidget()->backgroundPixmap() &&
				 !slider->parentWidget()->backgroundPixmap()->isNull() ) {
				QPixmap pixmap = *(slider->parentWidget()->backgroundPixmap());
				p2.drawTiledPixmap(r, pixmap, slider->pos());
			} else
				pix.fill(cg.background());

			// Draw slider groove
			if ((controls & SC_SliderGroove) && groove.isValid()) {
				drawKStylePrimitive( KPE_SliderGroove, &p2, widget, groove, cg, flags, opt );

				// Draw the focus rect around the groove
				if (slider->hasFocus())
					drawPrimitive(PE_FocusRect, &p2, groove, cg);
			}

			// Draw the tickmarks
			if (controls & SC_SliderTickmarks)
				QCommonStyle::drawComplexControl(control, &p2, widget,
						r, cg, flags, SC_SliderTickmarks, active, opt);

			// Draw the slider handle
			if ((controls & SC_SliderHandle) && handle.isValid()) {
				if (active == SC_SliderHandle)
					flags |= Style_Active;
				drawKStylePrimitive( KPE_SliderHandle, &p2, widget, handle, cg, flags, opt );
			}

			p2.end();
			bitBlt((QWidget*)widget, r.x(), r.y(), &pix);
			break;
		}

		// LISTVIEW
		// -------------------------------------------------------------------
		case CC_ListView: {

			/*
			 * Many thanks to TrollTech AS for donating CC_ListView from QWindowsStyle.
			 * CC_ListView code is Copyright (C) 1998-2000 TrollTech AS.
			 */

			// Paint the icon and text.
			if ( controls & SC_ListView )
				QCommonStyle::drawComplexControl( control, p, widget, r, cg, flags, controls, active, opt );

			// If we're have a branch or are expanded...
			if ( controls & (SC_ListViewBranch | SC_ListViewExpand) )
			{
				// If no list view item was supplied, break
				if (opt.isDefault())
					break;

				QListViewItem *item  = opt.listViewItem();
				QListViewItem *child = item->firstChild();

				int y = r.y();
				int c;	// dotline vertice count
				int dotoffset = 0;
				QPointArray dotlines;

				if ( active == SC_All && controls == SC_ListViewExpand ) {
					// We only need to draw a vertical line
					c = 2;
					dotlines.resize(2);
					dotlines[0] = QPoint( r.right(), r.top() );
					dotlines[1] = QPoint( r.right(), r.bottom() );

				} else {

					int linetop = 0, linebot = 0;
					// each branch needs at most two lines, ie. four end points
					dotoffset = (item->itemPos() + item->height() - y) % 2;
					dotlines.resize( item->childCount() * 4 );
					c = 0;

					// skip the stuff above the exposed rectangle
					while ( child && y + child->height() <= 0 )
					{
						y += child->totalHeight();
						child = nextVisibleSibling(child);
					}

					int bx = r.width() / 2;

					// paint stuff in the magical area
					QListView* v = item->listView();
					int lh = QMAX( p->fontMetrics().height() + 2 * v->itemMargin(),
								   QApplication::globalStrut().height() );
					if ( lh % 2 > 0 )
						lh++;

					// Draw all the expand/close boxes...
					QRect boxrect;
					QStyle::StyleFlags boxflags;
					while ( child && y < r.height() )
					{
						linebot = y + lh/2;
						if ( (child->isExpandable() || child->childCount()) &&
							 (child->height() > 0) )
						{
							// The primitive requires a rect.
							boxrect = QRect( bx-4, linebot-4, 9, 9 );
							boxflags = child->isOpen() ? QStyle::Style_Off : QStyle::Style_On;

							// QtCKStyle extension: Draw the box and expand/collapse indicator
							drawKStylePrimitive( KPE_ListViewExpander, p, NULL, boxrect, cg, boxflags, opt );

							// dotlinery
							p->setPen( cg.mid() );
							dotlines[c++] = QPoint( bx, linetop );
							dotlines[c++] = QPoint( bx, linebot - 5 );
							dotlines[c++] = QPoint( bx + 5, linebot );
							dotlines[c++] = QPoint( r.width(), linebot );
							linetop = linebot + 5;
						} else {
							// just dotlinery
							dotlines[c++] = QPoint( bx+1, linebot );
							dotlines[c++] = QPoint( r.width(), linebot );
						}

						y += child->totalHeight();
						child = nextVisibleSibling(child);
					}

					if ( child ) // there's a child to draw, so move linebot to edge of rectangle
						linebot = r.height();

					if ( linetop < linebot )
					{
						dotlines[c++] = QPoint( bx, linetop );
						dotlines[c++] = QPoint( bx, linebot );
					}
				}

				// Draw all the branches...
				static int thickness = kPixelMetric( KPM_ListViewBranchThickness );
				int line; // index into dotlines
				QRect branchrect;
				QStyle::StyleFlags branchflags;
				for( line = 0; line < c; line += 2 )
				{
					// assumptions here: lines are horizontal or vertical.
					// lines always start with the numerically lowest
					// coordinate.

					// point ... relevant coordinate of current point
					// end ..... same coordinate of the end of the current line
					// other ... the other coordinate of the current point/line
					if ( dotlines[line].y() == dotlines[line+1].y() )
					{
						// Horizontal branch
						int end = dotlines[line+1].x();
						int point = dotlines[line].x();
						int other = dotlines[line].y();

						branchrect  = QRect( point, other-(thickness/2), end-point, thickness );
						branchflags = QStyle::Style_Horizontal;

						// QtCKStyle extension: Draw the horizontal branch
						drawKStylePrimitive( KPE_ListViewBranch, p, NULL, branchrect, cg, branchflags, opt );

					} else {
						// Vertical branch
						int end = dotlines[line+1].y();
						int point = dotlines[line].y();
						int other = dotlines[line].x();
						int pixmapoffset = ((point & 1) != dotoffset ) ? 1 : 0;

						branchrect  = QRect( other-(thickness/2), point, thickness, end-point );
						if (!pixmapoffset)	// ### Hackish - used to hint the offset
							branchflags = QStyle::Style_NoChange;
						else
							branchflags = QStyle::Style_Default;

						// QtCKStyle extension: Draw the vertical branch
						drawKStylePrimitive( KPE_ListViewBranch, p, NULL, branchrect, cg, branchflags, opt );
					}
				}
			}
			break;
		}

		default:
			QCommonStyle::drawComplexControl( control, p, widget, r, cg,
											  flags, controls, active, opt );
			break;
	}
}


QStyle::SubControl QtCKStyle::querySubControl( ComplexControl control,
											const QWidget* widget,
											const QPoint &pos,
											const QStyleOption &opt ) const
{
	QStyle::SubControl ret = QCommonStyle::querySubControl(control, widget, pos, opt);

	if (d->scrollbarType == ThreeButtonScrollBar) {
		// Enable third button
		if (control == CC_ScrollBar && ret == SC_None)
			ret = SC_ScrollBarSubLine;
	}
	return ret;
}


QRect QtCKStyle::querySubControlMetrics( ComplexControl control,
									  const QWidget* widget,
									  SubControl sc,
									  const QStyleOption &opt ) const
{
    QRect ret;

	if (control == CC_ScrollBar)
	{
		bool threeButtonScrollBar = d->scrollbarType & ThreeButtonScrollBar;
		bool platinumScrollBar    = d->scrollbarType & PlatinumStyleScrollBar;
		bool nextScrollBar        = d->scrollbarType & NextStyleScrollBar;

		const QScrollBar *sb = (const QScrollBar*)widget;
		bool horizontal = sb->orientation() == Qt::Horizontal;
		int sliderstart = sb->sliderStart();
		int sbextent    = pixelMetric(PM_ScrollBarExtent, widget);
		int maxlen      = (horizontal ? sb->width() : sb->height())
						  - (sbextent * (threeButtonScrollBar ? 3 : 2));
		int sliderlen;

		// calculate slider length
		if (sb->maxValue() != sb->minValue())
		{
			uint range = sb->maxValue() - sb->minValue();
			sliderlen = (sb->pageStep() * maxlen) /	(range + sb->pageStep());

			int slidermin = pixelMetric( PM_ScrollBarSliderMin, widget );
			if ( sliderlen < slidermin || range > INT_MAX / 2 )
				sliderlen = slidermin;
			if ( sliderlen > maxlen )
				sliderlen = maxlen;
		} else
			sliderlen = maxlen;

		// Subcontrols
		switch (sc)
		{
			case SC_ScrollBarSubLine: {
				// top/left button
				if (platinumScrollBar) {
					if (horizontal)
						ret.setRect(sb->width() - 2 * sbextent, 0, sbextent, sbextent);
					else
						ret.setRect(0, sb->height() - 2 * sbextent, sbextent, sbextent);
				} else
					ret.setRect(0, 0, sbextent, sbextent);
				break;
			}

			case SC_ScrollBarAddLine: {
				// bottom/right button
				if (nextScrollBar) {
					if (horizontal)
						ret.setRect(sbextent, 0, sbextent, sbextent);
					else
						ret.setRect(0, sbextent, sbextent, sbextent);
				} else {
					if (horizontal)
						ret.setRect(sb->width() - sbextent, 0, sbextent, sbextent);
					else
						ret.setRect(0, sb->height() - sbextent, sbextent, sbextent);
				}
				break;
			}

			case SC_ScrollBarSubPage: {
				// between top/left button and slider
				if (platinumScrollBar) {
					if (horizontal)
						ret.setRect(0, 0, sliderstart, sbextent);
					else
						ret.setRect(0, 0, sbextent, sliderstart);
				} else if (nextScrollBar) {
					if (horizontal)
						ret.setRect(sbextent*2, 0, sliderstart-2*sbextent, sbextent);
					else
						ret.setRect(0, sbextent*2, sbextent, sliderstart-2*sbextent);
				} else {
					if (horizontal)
						ret.setRect(sbextent, 0, sliderstart - sbextent, sbextent);
					else
						ret.setRect(0, sbextent, sbextent, sliderstart - sbextent);
				}
				break;
			}

			case SC_ScrollBarAddPage: {
				// between bottom/right button and slider
				int fudge;

				if (platinumScrollBar)
					fudge = 0;
				else if (nextScrollBar)
					fudge = 2*sbextent;
				else
					fudge = sbextent;

				if (horizontal)
					ret.setRect(sliderstart + sliderlen, 0,
							maxlen - sliderstart - sliderlen + fudge, sbextent);
				else
					ret.setRect(0, sliderstart + sliderlen, sbextent,
							maxlen - sliderstart - sliderlen + fudge);
				break;
			}

			case SC_ScrollBarGroove: {
				int multi = threeButtonScrollBar ? 3 : 2;
				int fudge;

				if (platinumScrollBar)
					fudge = 0;
				else if (nextScrollBar)
					fudge = 2*sbextent;
				else
					fudge = sbextent;

				if (horizontal)
					ret.setRect(fudge, 0, sb->width() - sbextent * multi, sb->height());
				else
					ret.setRect(0, fudge, sb->width(), sb->height() - sbextent * multi);
				break;
			}

			case SC_ScrollBarSlider: {
				if (horizontal)
					ret.setRect(sliderstart, 0, sliderlen, sbextent);
				else
					ret.setRect(0, sliderstart, sbextent, sliderlen);
				break;
			}

			default:
				ret = QCommonStyle::querySubControlMetrics(control, widget, sc, opt);
				break;
		}
	} else
		ret = QCommonStyle::querySubControlMetrics(control, widget, sc, opt);

	return ret;
}

static const char * const kstyle_close_xpm[] = {
"12 12 2 1",
"# c #000000",
". c None",
"............",
"............",
"..##....##..",
"...##..##...",
"....####....",
".....##.....",
"....####....",
"...##..##...",
"..##....##..",
"............",
"............",
"............"};

static const char * const kstyle_maximize_xpm[]={
"12 12 2 1",
"# c #000000",
". c None",
"............",
"............",
".##########.",
".##########.",
".#........#.",
".#........#.",
".#........#.",
".#........#.",
".#........#.",
".#........#.",
".##########.",
"............"};


static const char * const kstyle_minimize_xpm[] = {
"12 12 2 1",
"# c #000000",
". c None",
"............",
"............",
"............",
"............",
"............",
"............",
"............",
"...######...",
"...######...",
"............",
"............",
"............"};

static const char * const kstyle_normalizeup_xpm[] = {
"12 12 2 1",
"# c #000000",
". c None",
"............",
"...#######..",
"...#######..",
"...#.....#..",
".#######.#..",
".#######.#..",
".#.....#.#..",
".#.....###..",
".#.....#....",
".#.....#....",
".#######....",
"............"};


static const char * const kstyle_shade_xpm[] = {
"12 12 2 1",
"# c #000000",
". c None",
"............",
"............",
"............",
"............",
"............",
".....#......",
"....###.....",
"...#####....",
"..#######...",
"............",
"............",
"............"};

static const char * const kstyle_unshade_xpm[] = {
"12 12 2 1",
"# c #000000",
". c None",
"............",
"............",
"............",
"............",
"..#######...",
"...#####....",
"....###.....",
".....#......",
"............",
"............",
"............",
"............"};

static const char * const dock_window_close_xpm[] = {
"8 8 2 1",
"# c #000000",
". c None",
"##....##",
".##..##.",
"..####..",
"...##...",
"..####..",
".##..##.",
"##....##",
"........"};

// Message box icons, from page 210 of the Windows style guide.

// Hand-drawn to resemble Microsoft's icons, but in the Mac/Netscape
// palette.  The "question mark" icon, which Microsoft recommends not
// using but a lot of people still use, is left out.

/* XPM */
static const char * const information_xpm[]={
"32 32 5 1",
". c None",
"c c #000000",
"* c #999999",
"a c #ffffff",
"b c #0000ff",
"...........********.............",
"........***aaaaaaaa***..........",
"......**aaaaaaaaaaaaaa**........",
".....*aaaaaaaaaaaaaaaaaa*.......",
"....*aaaaaaaabbbbaaaaaaaac......",
"...*aaaaaaaabbbbbbaaaaaaaac.....",
"..*aaaaaaaaabbbbbbaaaaaaaaac....",
".*aaaaaaaaaaabbbbaaaaaaaaaaac...",
".*aaaaaaaaaaaaaaaaaaaaaaaaaac*..",
"*aaaaaaaaaaaaaaaaaaaaaaaaaaaac*.",
"*aaaaaaaaaabbbbbbbaaaaaaaaaaac*.",
"*aaaaaaaaaaaabbbbbaaaaaaaaaaac**",
"*aaaaaaaaaaaabbbbbaaaaaaaaaaac**",
"*aaaaaaaaaaaabbbbbaaaaaaaaaaac**",
"*aaaaaaaaaaaabbbbbaaaaaaaaaaac**",
"*aaaaaaaaaaaabbbbbaaaaaaaaaaac**",
".*aaaaaaaaaaabbbbbaaaaaaaaaac***",
".*aaaaaaaaaaabbbbbaaaaaaaaaac***",
"..*aaaaaaaaaabbbbbaaaaaaaaac***.",
"...caaaaaaabbbbbbbbbaaaaaac****.",
"....caaaaaaaaaaaaaaaaaaaac****..",
".....caaaaaaaaaaaaaaaaaac****...",
"......ccaaaaaaaaaaaaaacc****....",
".......*cccaaaaaaaaccc*****.....",
"........***cccaaaac*******......",
"..........****caaac*****........",
".............*caaac**...........",
"...............caac**...........",
"................cac**...........",
".................cc**...........",
"..................***...........",
"...................**..........."};
/* XPM */
static const char* const warning_xpm[]={
"32 32 4 1",
". c None",
"a c #ffff00",
"* c #000000",
"b c #999999",
".............***................",
"............*aaa*...............",
"...........*aaaaa*b.............",
"...........*aaaaa*bb............",
"..........*aaaaaaa*bb...........",
"..........*aaaaaaa*bb...........",
".........*aaaaaaaaa*bb..........",
".........*aaaaaaaaa*bb..........",
"........*aaaaaaaaaaa*bb.........",
"........*aaaa***aaaa*bb.........",
".......*aaaa*****aaaa*bb........",
".......*aaaa*****aaaa*bb........",
"......*aaaaa*****aaaaa*bb.......",
"......*aaaaa*****aaaaa*bb.......",
".....*aaaaaa*****aaaaaa*bb......",
".....*aaaaaa*****aaaaaa*bb......",
"....*aaaaaaaa***aaaaaaaa*bb.....",
"....*aaaaaaaa***aaaaaaaa*bb.....",
"...*aaaaaaaaa***aaaaaaaaa*bb....",
"...*aaaaaaaaaa*aaaaaaaaaa*bb....",
"..*aaaaaaaaaaa*aaaaaaaaaaa*bb...",
"..*aaaaaaaaaaaaaaaaaaaaaaa*bb...",
".*aaaaaaaaaaaa**aaaaaaaaaaa*bb..",
".*aaaaaaaaaaa****aaaaaaaaaa*bb..",
"*aaaaaaaaaaaa****aaaaaaaaaaa*bb.",
"*aaaaaaaaaaaaa**aaaaaaaaaaaa*bb.",
"*aaaaaaaaaaaaaaaaaaaaaaaaaaa*bbb",
"*aaaaaaaaaaaaaaaaaaaaaaaaaaa*bbb",
".*aaaaaaaaaaaaaaaaaaaaaaaaa*bbbb",
"..*************************bbbbb",
"....bbbbbbbbbbbbbbbbbbbbbbbbbbb.",
".....bbbbbbbbbbbbbbbbbbbbbbbbb.."};
/* XPM */
static const char* const critical_xpm[]={
"32 32 4 1",
". c None",
"a c #999999",
"* c #ff0000",
"b c #ffffff",
"...........********.............",
".........************...........",
".......****************.........",
"......******************........",
".....********************a......",
"....**********************a.....",
"...************************a....",
"..*******b**********b*******a...",
"..******bbb********bbb******a...",
".******bbbbb******bbbbb******a..",
".*******bbbbb****bbbbb*******a..",
"*********bbbbb**bbbbb*********a.",
"**********bbbbbbbbbb**********a.",
"***********bbbbbbbb***********aa",
"************bbbbbb************aa",
"************bbbbbb************aa",
"***********bbbbbbbb***********aa",
"**********bbbbbbbbbb**********aa",
"*********bbbbb**bbbbb*********aa",
".*******bbbbb****bbbbb*******aa.",
".******bbbbb******bbbbb******aa.",
"..******bbb********bbb******aaa.",
"..*******b**********b*******aa..",
"...************************aaa..",
"....**********************aaa...",
"....a********************aaa....",
".....a******************aaa.....",
"......a****************aaa......",
".......aa************aaaa.......",
".........aa********aaaaa........",
"...........aaaaaaaaaaa..........",
".............aaaaaaa............"};

QPixmap QtCKStyle::stylePixmap( StylePixmap stylepixmap,
						  const QWidget* widget,
						  const QStyleOption& opt) const
{
	switch (stylepixmap) {
		case SP_TitleBarShadeButton:
			return QPixmap(const_cast<const char**>(kstyle_shade_xpm));
		case SP_TitleBarUnshadeButton:
			return QPixmap(const_cast<const char**>(kstyle_unshade_xpm));
		case SP_TitleBarNormalButton:
			return QPixmap(const_cast<const char**>(kstyle_normalizeup_xpm));
		case SP_TitleBarMinButton:
			return QPixmap(const_cast<const char**>(kstyle_minimize_xpm));
		case SP_TitleBarMaxButton:
			return QPixmap(const_cast<const char**>(kstyle_maximize_xpm));
		case SP_TitleBarCloseButton:
			return QPixmap(const_cast<const char**>(kstyle_close_xpm));
		case SP_DockWindowCloseButton:
			return QPixmap(const_cast<const char**>(dock_window_close_xpm ));
		case SP_MessageBoxInformation:
			return QPixmap(const_cast<const char**>(information_xpm));
		case SP_MessageBoxWarning:
			return QPixmap(const_cast<const char**>(warning_xpm));
		case SP_MessageBoxCritical:
			return QPixmap(const_cast<const char**>(critical_xpm));
		default:
			break;
    }
    return QCommonStyle::stylePixmap(stylepixmap, widget, opt);
}


int QtCKStyle::styleHint( StyleHint sh, const QWidget* w,
					   const QStyleOption &opt, QStyleHintReturn* shr) const
{
	switch (sh)
	{
		case SH_EtchDisabledText:
			return d->etchDisabledText ? 1 : 0;

		case SH_PopupMenu_Scrollable:
			return d->scrollablePopupmenus ? 1 : 0;

		case SH_MenuBar_AltKeyNavigation:
			return d->menuAltKeyNavigation ? 1 : 0;

		case SH_PopupMenu_SubMenuPopupDelay:
			if ( styleHint( SH_PopupMenu_SloppySubMenus, w ) )
				return QMIN( 100, d->popupMenuDelay );
			else
				return d->popupMenuDelay;

		case SH_PopupMenu_SloppySubMenus:
			return d->sloppySubMenus;

		case SH_ItemView_ChangeHighlightOnFocus:
		case SH_Slider_SloppyKeyEvents:
		case SH_MainWindow_SpaceBelowMenuBar:
		case SH_PopupMenu_AllowActiveAndDisabled:
			return 0;

		case SH_Slider_SnapToValue:
		case SH_PrintDialog_RightAlignButtons:
		case SH_FontDialog_SelectAssociatedText:
		case SH_MenuBar_MouseTracking:
		case SH_PopupMenu_MouseTracking:
		case SH_ComboBox_ListMouseTracking:
		case SH_ScrollBar_MiddleClickAbsolutePosition:
			return 1;
		case SH_LineEdit_PasswordCharacter:
		{
			if (w) {
				const QFontMetrics &fm = w->fontMetrics();
				if (fm.inFont(QChar(0x25CF))) {
					return 0x25CF;
				} else if (fm.inFont(QChar(0x2022))) {
					return 0x2022;
				}
			}
			return '*';
		}

		default:
			return QCommonStyle::styleHint(sh, w, opt, shr);
	}
}


bool QtCKStyle::eventFilter( QObject* object, QEvent* event )
{
	if ( d->useFilledFrameWorkaround )
	{
		// Make the QMenuBar/QToolBar paintEvent() cover a larger area to
		// ensure that the filled frame contents are properly painted.
		// We essentially modify the paintEvent's rect to include the
		// panel border, which also paints the widget's interior.
		// This is nasty, but I see no other way to properly repaint
		// filled frames in all QMenuBars and QToolBars.
		// -- Karol.
		QFrame *frame = 0;
		if ( event->type() == QEvent::Paint
				&& (frame = ::qt_cast<QFrame*>(object)) )
		{
			if (frame->frameShape() != QFrame::ToolBarPanel && frame->frameShape() != QFrame::MenuBarPanel)
				return false;
				
			bool horizontal = true;
			QPaintEvent* pe = (QPaintEvent*)event;
			QToolBar *toolbar = ::qt_cast< QToolBar *>( frame );
			QRect r = pe->rect();

			if (toolbar && toolbar->orientation() == Qt::Vertical)
				horizontal = false;

			if (horizontal) {
				if ( r.height() == frame->height() )
					return false;	// Let QFrame handle the painting now.

				// Else, send a new paint event with an updated paint rect.
				QPaintEvent dummyPE( QRect( r.x(), 0, r.width(), frame->height()) );
				QApplication::sendEvent( frame, &dummyPE );
			}
			else {	// Vertical
				if ( r.width() == frame->width() )
					return false;

				QPaintEvent dummyPE( QRect( 0, r.y(), frame->width(), r.height()) );
				QApplication::sendEvent( frame, &dummyPE );
			}

			// Discard this event as we sent a new paintEvent.
			return true;
		}
	}

	return false;
}

void QtCKStyle::virtual_hook( int, void* )
{ /*BASE::virtual_hook( id, data );*/ }

// vim: set noet ts=4 sw=4:
// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;

//#include "qtc_kstyle.moc"
