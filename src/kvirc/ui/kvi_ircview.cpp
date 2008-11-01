//===========================================================================
//
//   File : kvi_ircview.cpp
//   Creation date : Tue Jul 6 1999 14:45:20 by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 1999-2008 Szymon Stefanek (pragma at kvirc dot net)
//
//   This program is FREE software. You can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your opinion) any later version.
//
//   This program is distributed in the HOPE that it will be USEFUL,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program. If not, write to the Free Software Foundation,
//   Inc. ,59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//===========================================================================


// Damn complex class ...but it works :)
// #include <brain.h>
//
// #define HOPE_THAT_IT_WILL_NEVER_NEED_TO_BE_MODIFIED :)

// 07 May 1999 ,
//      Already forgot how this damn thing works ,
//      and spent 1 hour over a stupid bug.
//      I had to recreate the whole thing in my mind......ooooouh...
//      How did I wrote it ?
//      Just take a look to paintEvent() or to calculateLineWraps()...
//      Anyway...I've solved the bug.

// 23 Nov 1999 ,
//      Well , not so bad...I seem to still remember how it works
//      So just for fun , complicated the things a little bit more.
//      Added precaclucaltion of the text blocks and word wrapping
//      and a fast scrolling mode (3 lines at once) for consecutive
//      appendText() calls.
//      Now the code becomes really not understandable...:)

// 29 Jun 2000 21:02 ,
//      Here we go again... I have to adjust this stuff for 3.0.0
//      Will I make this thingie work ?
// 01 Jul 2000 04:20 (AM!) ,
//      Yes....I got it to work just now
//      and YES , complicated the things yet more.
//      This time made some paint event code completely unreadable
//      by placing two monster macros...
//      I hope that you have a smart compiler (such as gcc is).

// 09 Dec 2000
//      This is my C-asm-optimisation-hack playground
//      Expect Bad Programming(tm) , Ugly Code(tm) , Unreadable Macros (tm)
//      and massive usage of the Evil(tm) goto.

// 25 Sep 2001
//      This stuff is going to be ported to Windoze
//      A conditionally compiled code will use only Qt calls...let's see :)
//


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Here we go... a huge set of includes
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "kvi_ircview.h"
#include "kvi_ircviewtools.h"
#include "kvi_ircviewprivate.h"
#include "kvi_debug.h"
#include "kvi_app.h"
#include "kvi_settings.h"
#include "kvi_options.h"
#include "kvi_mirccntrl.h"
#include "kvi_defaults.h"
#include "kvi_window.h"
#include "kvi_locale.h"
#include "kvi_frame.h"
#include "kvi_malloc.h"
#include "kvi_iconmanager.h"
#include "kvi_out.h"
#include "kvi_parameterlist.h"
#include "kvi_console.h"
#include "kvi_ircuserdb.h"
#include "kvi_channel.h"
#include "kvi_filedialog.h"
#include "kvi_msgbox.h"
#include "kvi_texticonmanager.h"
#include "kvi_ircconnection.h"
#include "kvi_mdimanager.h"
#include "kvi_userinput.h"
#include "kvi_qcstring.h"
#include "kvi_tal_popupmenu.h"
#include "kvi_animatedpixmap.h"

// FIXME: #warning "There should be an option to preserve control codes in copied text (clipboard) (mIrc = CTRL+Copy->with colors)"

#include <QBitmap>
#include <QPainter>
#include <QRegExp>
#include <QFontMetrics>
#include <QApplication>
#include <QMessageBox>
#include <QPaintEvent>
#include <QDateTime>
#include <QCursor>
#include <QScrollBar>
#include <QFontDialog>

// FIXME: #warning "There are problems with the selection and wrapped lines: you can select something on the first line and get the second highlighted"
// FIXME: #warning "This hack is temporary...later remove it"

#include <time.h>


#ifdef COMPILE_ON_WINDOWS
	#pragma warning(disable: 4102)
#endif

#ifdef __STRICT_ANSI__
	#ifdef COMPILE_USE_DYNAMIC_LABELS
		// incompatible with -ansi

	#endif
#endif
//#undef COMPILE_USE_DYNAMIC_LABELS

#define KVI_DEF_BACK 200

// FIXME: #warning "The scrollbar should NOT have a fixed size : the KDE styles can configure the size (sizeHint() ?)"

//
// FIXME: PgUp and PgDn scrolls a fixed number of lines!
//        Make it view height dependant
//
// FIXME: This widget is quite slow on a 300 MHz processor
//


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Globals
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Stuff declared in kvi_app.cpp and managed by KviApp class


#ifdef COMPILE_PSEUDO_TRANSPARENCY
	extern QPixmap       * g_pShadedChildGlobalDesktopBackground;
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Internal constants
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Maximum size of the internal buffer for each window
// This is the default value
//#define KVI_IRCVIEW_MAX_LINES 1024
// A little bit more than the scroll-bar...
// Qt+X have strange interactions that I can not understand when I try to move the splitter
// to the maximum on the left , Maybe the cache pixmap size becomes negative ? (I don't think so)
// Anyway , when the scroll bar position becomes negative (or the IrcView has smaller width than
// the scroll bar) X aborts with a funny
// X Error: BadDrawable (invalid Pixmap or Window parameter) 9
//   Major opcode:  55
// Program received signal SIGABRT, Aborted.
// Do not change unless you're sure that it will not happen :)
#define KVI_IRCVIEW_MINIMUM_WIDTH 22
//16+4+(2*4) * Do not change
#define KVI_IRCVIEW_PIXMAP_AND_SEPARATOR 20
#define KVI_IRCVIEW_PIXMAP_SEPARATOR_AND_DOUBLEBORDER_WIDTH 28
#define KVI_IRCVIEW_SIZEHINT_WIDTH 150
#define KVI_IRCVIEW_SIZEHINT_HEIGHT 150

#define KVI_IRCVIEW_BLOCK_SELECTION_TOTAL 0
#define KVI_IRCVIEW_BLOCK_SELECTION_LEFT 1
#define KVI_IRCVIEW_BLOCK_SELECTION_RIGHT 2
#define KVI_IRCVIEW_BLOCK_SELECTION_CENTRAL 3
#define KVI_IRCVIEW_BLOCK_SELECTION_ICON 4

#define KVI_IRCVIEW_PIXMAP_SIZE 16

#define KVI_IRCVIEW_ESCAPE_TAG_URLLINK 'u'
#define KVI_IRCVIEW_ESCAPE_TAG_NICKLINK 'n'
#define KVI_IRCVIEW_ESCAPE_TAG_SERVERLINK 's'
#define KVI_IRCVIEW_ESCAPE_TAG_HOSTLINK 'h'
#define KVI_IRCVIEW_ESCAPE_TAG_GENERICESCAPE '['

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Info about escape syntax
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// escape commands:
//
//  <cr>!<escape_command><cr><visible parameters<cr>
//
//  <escape_command> ::= u        <--- url link
//  <escape_command> ::= n        <--- nick link
//  <escape_command> ::= s        <--- server link
//  <escape_command> ::= h        <--- host link
//  <escape_command> ::= [...     <--- generic escape "rbt" | "mbt" | "dbl" | "txt"
//


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The IrcView : construct and destroy
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

KviIrcView::KviIrcView(QWidget *parent,KviFrame *pFrm,KviWindow *pWnd)
: QWidget(parent)
{
	setObjectName("irc_view");
	// Ok...here we go
	// initialize the initializable

	setAttribute(Qt::WA_NoSystemBackground); // This disables automatic qt double buffering
	setAttribute(Qt::WA_OpaquePaintEvent);
	//setAttribute(Qt::WA_PaintOnScreen); // disable qt backing store (that would force us to trigger repaint() instead of the 10 times faster paintEvent(0))

	m_iFlushTimer = 0;
	m_pToolsPopup = 0;
	m_pFirstLine               = 0;
	m_pCurLine                 = 0;
	m_pLastLine                = 0;
	m_pCursorLine              = 0;
	m_uLineMarkLineIndex       = KVI_IRCVIEW_INVALID_LINE_MARK_INDEX;
	m_bHaveUnreadedHighlightedMessages = false;
	m_bHaveUnreadedMessages = false;
	m_iNumLines                = 0;
	m_iMaxLines                = KVI_OPTION_UINT(KviOption_uintIrcViewMaxBufferSize);

	m_uNextLineIndex           = 0;

	if(m_iMaxLines < 32)
	{
		m_iMaxLines = 32;
		KVI_OPTION_UINT(KviOption_uintIrcViewMaxBufferSize) = 32;
	}

	m_bMouseIsDown               = false;

	//m_bShowImages              = KVI_OPTION_BOOL(KviOption_boolIrcViewShowImages);

	m_iSelectTimer             = 0;
	m_iMouseTimer		   = 0;
	//m_iTipTimer                = 0;
	//m_bTimestamp               = KVI_OPTION_BOOL(KviOption_boolIrcViewTimestamp);

	m_bAcceptDrops             = false;
	m_pPrivateBackgroundPixmap = 0;
	m_bSkipScrollBarRepaint    = false;
	m_pLogFile                 = 0;
	m_pKviWindow               = pWnd;
	m_pFrm                     = pFrm;

	m_iUnprocessedPaintEventRequests = 0;
	m_bPostedPaintEventPending = false;

	m_pLastLinkUnderMouse      = 0;
	m_iLastLinkRectTop         = -1;
	m_iLastLinkRectHeight      = -1;

	m_pMasterView              = 0;

	m_pToolWidget              = 0;

	m_pWrappedBlockSelectionInfo  = new KviIrcViewWrappedBlockSelectionInfo;


	m_pMessagesStoppedWhileSelecting = new KviPointerList<KviIrcViewLine>;
	m_pMessagesStoppedWhileSelecting->setAutoDelete(false);

	// say qt to avoid erasing on repaint
	setAutoFillBackground(false);

	m_pFm = 0; // will be updated in the first paint event

	m_pToolTip = new KviIrcViewToolTip(this);

	// Create the scroll bar
/*int minValue, int maxValue, int lineStep, int pageStep,
                int value, Qt::Orientation, QWidget *parent=0, const char* name = 0);
*/
	m_pScrollBar = new QScrollBar(Qt::Vertical,this);
	m_pScrollBar->setMaximum(0);
	m_pScrollBar->setMinimum(0);
	m_pScrollBar->setSingleStep(1);
	m_pScrollBar->setPageStep(10);
	m_pScrollBar->setValue(0);
	m_pScrollBar->setObjectName("irc_view_scrollbar");
	m_pScrollBar->setTracking(true);
	m_pScrollBar->show();
	m_pScrollBar->setFocusProxy(this);


	m_pToolsButton = new QToolButton(this);
	m_pToolsButton->setObjectName("btntools");

	QIcon is1(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_POPUPMENU)));
	m_pToolsButton->setAutoRaise(true);
	m_pToolsButton->setIcon(is1);

	KviTalToolTip::add(m_pToolsButton,__tr2qs("Search tools"));
	m_pToolsButton->setFocusProxy(this);

	connect(m_pToolsButton,SIGNAL(clicked()),this,SLOT(showToolsPopup()));
	m_pToolsButton->show();

	connect(m_pScrollBar,SIGNAL(valueChanged(int)),this,SLOT(scrollBarPositionChanged(int)));
	m_iLastScrollBarValue      = 0;

	// set the minimum width
	setMinimumWidth(KVI_IRCVIEW_MINIMUM_WIDTH);
	// and catch all mouse events
	setMouseTracking(true);
	// let's go!
	applyOptions();

	if(KVI_OPTION_UINT(KviOption_uintAutoFlushLogs)) //m_iFlushTimer
	{
		m_iFlushTimer = startTimer(KVI_OPTION_UINT(KviOption_uintAutoFlushLogs)*60*1000);
	}

//	if(pWnd->input()) setFocusProxy(pWnd->input());

}

static inline void delete_text_line(KviIrcViewLine * line,QHash<KviIrcViewLine*,KviAnimatedPixmap*>*  animatedSmiles)
{
	QMultiHash<KviIrcViewLine*, KviAnimatedPixmap*>::iterator it =
			animatedSmiles->find(line);
	while (it != animatedSmiles->end() && it.key() == line)
	{
		it = animatedSmiles->erase(it);
	}
	for(unsigned int i=0;i<line->uChunkCount;i++)
	{
		if((line->pChunks[i].type == KVI_TEXT_ESCAPE) || (line->pChunks[i].type == KVI_TEXT_ICON))
		{
			if( (line->pChunks[i].type == KVI_TEXT_ICON) && (line->pChunks[i].szPayload!=line->pChunks[i].szSmileId) )
				kvi_free(line->pChunks[i].szSmileId);
			kvi_free(line->pChunks[i].szPayload);
		}
	}
	kvi_free(line->pChunks);                        //free attributes data
	if(line->iBlockCount)kvi_free(line->pBlocks);
	delete line;
}

KviIrcView::~KviIrcView()
{
	// kill any pending timer
	if(m_iFlushTimer) killTimer(m_iFlushTimer);
	if(m_iSelectTimer)killTimer(m_iSelectTimer);
	if(m_iMouseTimer)killTimer(m_iMouseTimer);
	// and close the log file (flush!)
	stopLogging();
	if(m_pToolWidget)delete m_pToolWidget;
	// don't forget the bacgkround pixmap!
	if(m_pPrivateBackgroundPixmap)delete m_pPrivateBackgroundPixmap;
	// and to remove all the text lines
	emptyBuffer(false);
	// the pending ones too!
	while(KviIrcViewLine * l = m_pMessagesStoppedWhileSelecting->first())
	{
		m_pMessagesStoppedWhileSelecting->removeFirst();
		delete_text_line(l,&m_hAnimatedSmiles);
	}
	delete m_pMessagesStoppedWhileSelecting;
	if(m_pFm)delete m_pFm;
	delete m_pToolTip;
	delete m_pWrappedBlockSelectionInfo;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The IrcView : options
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void KviIrcView::setFont(const QFont &f)
{
	if(m_pFm)
	{
		// force an update to the font variables
		delete m_pFm;
		m_pFm = 0;
	}
	KviIrcViewLine * l = m_pFirstLine;
	while(l)
	{
		l->iMaxLineWidth = -1;
		l = l->pNext;
	}
	QWidget::setFont(f);
	update();
}

void KviIrcView::applyOptions()
{
	flushLog();
	setFont(KVI_OPTION_FONT(KviOption_fontIrcView));
	if(m_iFlushTimer) killTimer(m_iFlushTimer);
	if(KVI_OPTION_UINT(KviOption_uintAutoFlushLogs))
	{
		m_iFlushTimer = startTimer(KVI_OPTION_UINT(KviOption_uintAutoFlushLogs)*60*1000);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The IrcView : DnD   //2005.Resurection by Grifisx & Noldor
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void KviIrcView::enableDnd(bool bEnable)
{
	setAcceptDrops(bEnable);
	m_bAcceptDrops = bEnable;
}

void KviIrcView::clearBuffer()
{
	emptyBuffer(true);
}

bool KviIrcView::saveBuffer(const char *filename)
{
	QFile f(QString::fromUtf8(filename));
	if(!f.open(QIODevice::WriteOnly|QIODevice::Truncate))return false;
	QString tmp;
	getTextBuffer(tmp);
	KviQCString tmpx = KviQString::toUtf8(tmp);
	f.write(tmpx.data(),tmpx.length());
	f.close();
	return true;
}

void KviIrcView::prevLine(){ m_pScrollBar->triggerAction(QAbstractSlider::SliderSingleStepSub); }
void KviIrcView::nextLine(){ m_pScrollBar->triggerAction(QAbstractSlider::SliderSingleStepAdd); }
void KviIrcView::prevPage(){ m_pScrollBar->triggerAction(QAbstractSlider::SliderPageStepSub); }
void KviIrcView::nextPage(){ m_pScrollBar->triggerAction(QAbstractSlider::SliderPageStepAdd);}

void KviIrcView::setPrivateBackgroundPixmap(const QPixmap &pixmap,bool bRepaint)
{
	if(m_pPrivateBackgroundPixmap)
	{
		delete m_pPrivateBackgroundPixmap;
		m_pPrivateBackgroundPixmap=0;
	}
	if(!pixmap.isNull())m_pPrivateBackgroundPixmap = new QPixmap(pixmap);

	if(bRepaint)
		update();
}

void KviIrcView::emptyBuffer(bool bRepaint)
{
	while(m_pLastLine != 0)removeHeadLine();
	if(bRepaint)
		update();
}

void KviIrcView::clearLineMark(bool bRepaint)
{
	m_uLineMarkLineIndex = KVI_IRCVIEW_INVALID_LINE_MARK_INDEX;
	clearUnreaded();
	if(bRepaint)
		update();
}

void KviIrcView::clearUnreaded()
{
	m_bHaveUnreadedHighlightedMessages = false;
	m_bHaveUnreadedMessages = false;

	if(m_pFrm)
		if(m_pFrm->dockExtension())
			m_pFrm->dockExtension()->refresh();
}

void KviIrcView::setMaxBufferSize(int maxBufSize,bool bRepaint)
{
	if(maxBufSize < 32)maxBufSize = 32;
	m_iMaxLines = maxBufSize;
	while(m_iNumLines > m_iMaxLines)removeHeadLine();
	m_pScrollBar->setRange(0,m_iNumLines);
	if(bRepaint)
		update();
}

/*
void KviIrcView::setTimestamp(bool bTimestamp)
{
	m_bTimestamp = bTimestamp;


// STATS FOR A BUFFER FULL OF HIGHLY COLORED STRINGS , HIGHLY WRAPPED
//
// Lines = 1024 (322425 bytes - 314 KB) (avg 314 bytes per line) , well :)
// string bytes = 87745 (85 KB)
// attributes = 3576 (42912 bytes - 41 KB)
// blocks = 12226 (146712 bytes - 143 KB)
//
//	unsigned long int nAlloc = 0;
//	unsigned long int nLines = 0;
//	unsigned long int nStringBytes = 0;
//	unsigned long int nAttrBytes = 0;
//	unsigned long int nBlockBytes = 0;
//	unsigned long int nBlocks = 0;
//	unsigned long int nAttributes = 0;
//	KviIrcViewLine * l=m_pFirstLine;
//	while(l){
//		nLines++;
//		nAlloc += sizeof(KviIrcViewLine);
//		nStringBytes += l->data_len + 1;
//		nAlloc += l->data_len + 1;
//		nAlloc += (l->uChunkCount * sizeof(KviIrcViewLineChunk));
//		nAttrBytes +=(l->uChunkCount * sizeof(KviIrcViewLineChunk));
//		nAlloc += (l->iBlockCount * sizeof(KviIrcViewLineChunk));
//		nBlockBytes += (l->iBlockCount * sizeof(KviIrcViewLineChunk));
//		nBlocks += (l->iBlockCount);
//		nAttributes += (l->uChunkCount);
//		l = l->pNext;
//	}
//	debug("\n\nLines = %u (%u bytes - %u KB) (avg %u bytes per line)",nLines,nAlloc,nAlloc / 1024,nLines ? (nAlloc / nLines) : 0);
//	debug("string bytes = %u (%u KB)",nStringBytes,nStringBytes / 1024);
//	debug("attributes = %u (%u bytes - %u KB)",nAttributes,nAttrBytes,nAttrBytes / 1024);
//	debug("blocks = %u (%u bytes - %u KB)\n",nBlocks,nBlockBytes,nBlockBytes / 1024);

}
*/
void KviIrcView::scrollBarPositionChanged(int newValue)
{
	if(!m_pCurLine)return;
	int diff = 0;
	if(newValue > m_iLastScrollBarValue)
	{
		while(newValue > m_iLastScrollBarValue)
		{
			if(m_pCurLine->pNext)
			{
				m_pCurLine=m_pCurLine->pNext;
				diff++;
			}
			m_iLastScrollBarValue++;
		}
	} else {
		while(newValue < m_iLastScrollBarValue)
		{
			if(m_pCurLine->pPrev)m_pCurLine=m_pCurLine->pPrev;
			m_iLastScrollBarValue--;
		}
	}
	if(!m_bSkipScrollBarRepaint)
		repaint();
}

void KviIrcView::postUpdateEvent()
{
	// This will post a QEvent with a full repaint request
	if(!m_bPostedPaintEventPending)
	{
		m_bPostedPaintEventPending = true;
		QEvent *e = new QEvent(QEvent::User);
		g_pApp->postEvent(this,e); // queue a repaint
	}

	m_iUnprocessedPaintEventRequests++; // paintEvent() will set it to 0

	if(m_iUnprocessedPaintEventRequests == 3)
	{
		// Three unprocessed paint events...do it now
#ifdef COMPILE_PSEUDO_TRANSPARENCY
		if(! ((KVI_OPTION_PIXMAP(KviOption_pixmapIrcViewBackground).pixmap()) || m_pPrivateBackgroundPixmap || g_pShadedChildGlobalDesktopBackground))
			fastScroll(3);
#else
		if(! ((KVI_OPTION_PIXMAP(KviOption_pixmapIrcViewBackground).pixmap()) || m_pPrivateBackgroundPixmap))
			fastScroll(3);
#endif
		else
			repaint();
	}
}

void KviIrcView::appendLine(KviIrcViewLine *ptr,bool bRepaint)
{
	//This one appends a KviIrcViewLine to
	//the buffer list (at the end)
	if(m_bMouseIsDown)
	{
		// Do not move the view!
		// So we append the text line to a temp queue
		// and then we'll add it when the mouse button is released
		m_pMessagesStoppedWhileSelecting->append(ptr);
		return;
	}

	// First log the line and assign the index
	// Don't use add2log here!...we must go as fast as possible, so we avoid some push and pop calls, and also a couple of branches
	if(m_pLogFile && KVI_OPTION_BOOL(KviOption_boolStripControlCodesInLogs))
	{
		// a slave view has no log files!
		if(KVI_OPTION_MSGTYPE(ptr->iMsgType).logEnabled())
		{
			add2Log(ptr->szText,ptr->iMsgType);
			// If we fail...this has been already reported!
		}
		// mmh.. when this overflows... we have problems (find doesn't work anymore :()
		// but it overflows at 2^32 lines... 2^32 = 4.294.967.296 lines
		// to spit it out in a year you'd need to print 1360 lines per second... that's insane :D
		// a really fast but reasonable rate of printed lines might be 10 per second
		// thus 429.496.730 seconds would be needed to make this var overflow
		// that means more or less 13 years of text spitting at full rate :D
		// I think that we can safely assume that this will NOT overflow ... your cpu (or you)
		// will get mad before. Well.. it is not that dangerous after all...
		ptr->uIndex = m_uNextLineIndex;
		m_uNextLineIndex++;
	} else {
		// no log: we could have master view!
		if(m_pMasterView)
		{
			if(m_pMasterView->m_pLogFile && KVI_OPTION_BOOL(KviOption_boolStripControlCodesInLogs))
			{
				if(KVI_OPTION_MSGTYPE(ptr->iMsgType).logEnabled())
				{
					m_pMasterView->add2Log(ptr->szText,ptr->iMsgType);
				}
			}
			ptr->uIndex = m_pMasterView->m_uNextLineIndex;
			m_pMasterView->m_uNextLineIndex++;
		} else {
			ptr->uIndex = m_uNextLineIndex;
			m_uNextLineIndex++;
		}
	}

	if(m_pLastLine)
	{
		// There is at least one line in the view
		m_pLastLine->pNext=ptr;
		ptr->pPrev  =m_pLastLine;
		ptr->pNext  =0;
		m_iNumLines++;

		if(m_iNumLines > m_iMaxLines)
		{
			// Too many lines in the view...remove one
			removeHeadLine();
			if(m_pCurLine==m_pLastLine)
			{
				m_pCurLine=ptr;
				if(bRepaint)
					postUpdateEvent();
			} else {
				// the cur line remains the same
				// the scroll bar must move up one place to be in sync
				m_bSkipScrollBarRepaint = true;
				if(m_pScrollBar->value() > 0)
				{
					m_iLastScrollBarValue--;
					__range_valid(m_iLastScrollBarValue >= 0);
					m_pScrollBar->triggerAction(QAbstractSlider::SliderSingleStepSub);
				} // else will stay in sync
				m_bSkipScrollBarRepaint = false;
			}
		} else {
			// Just append
			m_pScrollBar->setRange(0,m_iNumLines);
			if(m_pCurLine==m_pLastLine)
			{
				m_bSkipScrollBarRepaint = true;
				m_pScrollBar->triggerAction(QAbstractSlider::SliderSingleStepAdd);
				m_bSkipScrollBarRepaint = false;
				if(bRepaint)
					postUpdateEvent();
			}
		}
		m_pLastLine=ptr;
	} else {
		//First line
		m_pLastLine    = ptr;
		m_pFirstLine   = ptr;
		m_pCurLine     = ptr;
		ptr->pPrev = 0;
		ptr->pNext = 0;
		m_iNumLines    = 1;
		m_pScrollBar->setRange(0,1);
		m_pScrollBar->triggerAction(QAbstractSlider::SliderSingleStepAdd);
		if(bRepaint)
			postUpdateEvent();
	}
}

//============== removeHeadLine ===============//

void KviIrcView::removeHeadLine(bool bRepaint)
{
	//Removes the first line of the text buffer
	if(!m_pLastLine)return;
	if(m_pFirstLine == m_pCursorLine)m_pCursorLine = 0;

	if(m_pFirstLine->pNext)
	{
		KviIrcViewLine *aux_ptr=m_pFirstLine->pNext;     //get the next line
		aux_ptr->pPrev=0;                                    //becomes the first
		if(m_pFirstLine==m_pCurLine)m_pCurLine=aux_ptr;      //move the cur line if necessary
		delete_text_line(m_pFirstLine,&m_hAnimatedSmiles);                   //delete the struct
		m_pFirstLine=aux_ptr;                                //set the last
		m_iNumLines--;                                       //and decrement the count
	} else { //unique line
		m_pCurLine   = 0;
		delete_text_line(m_pFirstLine,&m_hAnimatedSmiles);
		m_pFirstLine = 0;
		m_iNumLines  = 0;
		m_pLastLine  = 0;
	}
	if(bRepaint)
		repaint();
}

void KviIrcView::splitMessagesTo(KviIrcView *v)
{
	v->emptyBuffer(false);

	KviIrcViewLine * l = m_pFirstLine;
	KviIrcViewLine * tmp;
	while(l)
		switch(l->iMsgType)
	{
		case KVI_OUT_CHANPRIVMSG:
		case KVI_OUT_CHANPRIVMSGCRYPTED:
		case KVI_OUT_CHANNELNOTICE:
		case KVI_OUT_CHANNELNOTICECRYPTED:
		case KVI_OUT_ACTION:
		case KVI_OUT_OWNPRIVMSG:
		case KVI_OUT_OWNPRIVMSGCRYPTED:
		case KVI_OUT_HIGHLIGHT:
		{
			m_iNumLines--;
			v->m_iNumLines++;

			if(l->pNext)l->pNext->pPrev = l->pPrev;
			if(l->pPrev)l->pPrev->pNext = l->pNext;
			if(l == m_pFirstLine)m_pFirstLine = l->pNext;
			if(l == m_pLastLine)m_pLastLine = l->pPrev;
			if(v->m_pLastLine)
			{
				v->m_pLastLine->pNext = l;
				l->pPrev = v->m_pLastLine;
				v->m_pLastLine = l;
			} else {
				v->m_pFirstLine = l;
				l->pPrev = 0;
				v->m_pLastLine = l;
			}
			tmp = l->pNext;
			l->pNext = 0;
			l = tmp;
		}
		break;
		default:
			l = l->pNext;
		break;
	}
	v->m_pCurLine = v->m_pLastLine;
	m_pCurLine = m_pLastLine;

	v->m_pCursorLine = 0;
	m_pCursorLine = 0;

	m_iLastScrollBarValue = m_iNumLines;
	m_pScrollBar->setRange(0,m_iNumLines);
	m_pScrollBar->setValue(m_iNumLines);

	repaint();

	v->m_iLastScrollBarValue = v->m_iNumLines;
	v->m_pScrollBar->setRange(0,v->m_iNumLines);
	v->m_pScrollBar->setValue(v->m_iNumLines);
	v->repaint();

}

void KviIrcView::appendMessagesFrom(KviIrcView *v)
{
	if(!m_pLastLine)
	{
		m_pFirstLine = v->m_pFirstLine;
	} else {
		m_pLastLine->pNext = v->m_pFirstLine;
		v->m_pFirstLine->pPrev = m_pLastLine;
	}
	m_pLastLine  = v->m_pLastLine;
	m_pCurLine = m_pLastLine;
	m_pCursorLine = 0;
	v->m_pFirstLine = 0;
	v->m_pLastLine = 0;
	v->m_pCurLine = 0;
	v->m_pCursorLine = 0;
	m_iNumLines += v->m_iNumLines;
	v->m_iNumLines = 0;
//	v->m_pScrollBar->setRange(0,0);
//	v->m_pScrollBar->setValue(0);
	m_iLastScrollBarValue = m_iNumLines;
	m_pScrollBar->setRange(0,m_iNumLines);
	m_pScrollBar->setValue(m_iNumLines);

	repaint();
}

void KviIrcView::joinMessagesFrom(KviIrcView *v)
{
	KviIrcViewLine * l1 = m_pFirstLine;
	KviIrcViewLine * l2 = v->m_pFirstLine;
	KviIrcViewLine * tmp;

	while(l2)
	{
		if(l1)
		{
			if(l2->uIndex < l1->uIndex)
			{
				// the external message is older than the current internal one
				l2->pPrev = l1->pPrev;
				if(l1->pPrev)l1->pPrev->pNext = l2;
				else m_pFirstLine = l2;
				l1->pPrev = l2;
				tmp = l2->pNext;
				l2->pNext = l1;
				l2 = tmp;
			} else {
				// the external message is younger than the current internal one
				l1 = l1->pNext;
			}
		} else {
			// There is no current internal message (ran over the end)
			// merge at the end then
			if(m_pFirstLine)
			{
				m_pLastLine->pNext = l2;
				l2->pPrev = m_pLastLine;
			} else {
				m_pFirstLine = l2;
				l2->pPrev = 0;
			}
			tmp = l2->pNext;
			l2->pNext = 0;
			m_pLastLine  = l2;
			l2 = tmp;
		}
	}

	m_pCurLine = m_pLastLine;
	m_pCursorLine = 0;
	v->m_pFirstLine = 0;
	v->m_pLastLine = 0;
	v->m_pCurLine = 0;
	v->m_pCursorLine = 0;
	m_iNumLines += v->m_iNumLines;
	v->m_iNumLines = 0;
//	v->m_pScrollBar->setRange(0,0);
//	v->m_pScrollBar->setValue(0);

	m_iLastScrollBarValue = m_iNumLines;
	m_pScrollBar->setRange(0,m_iNumLines);
	m_pScrollBar->setValue(m_iNumLines);

	repaint();
}

void KviIrcView::getLinkEscapeCommand(QString &buffer,const QString &szPayload,const QString &escape_label)
{
	if(szPayload.isEmpty())return;

	int idx = szPayload.indexOf(escape_label,Qt::CaseInsensitive);
	if(idx == -1)return;
	idx += escape_label.length();

	int idx2 = szPayload.indexOf("[!",idx,Qt::CaseInsensitive);
	int len = idx2 == -1 ? szPayload.length() - idx : idx2 - idx;

	buffer = szPayload.mid(idx,len);
}

void KviIrcView::fastScroll(int lines)
{
	m_iUnprocessedPaintEventRequests = 0;

	if(!isVisible())return;

	if(!m_pFm)
	{
		// We must get the metrics from a real paint event :/
		// must do a full repaint to get them...
		repaint();
		return;
	}

	// Ok...the current line is the last one here
	// It is the only one that needs to be repainted
	int widgetWidth  = width()-m_pScrollBar->width();
	if(widgetWidth < KVI_IRCVIEW_PIXMAP_SEPARATOR_AND_DOUBLEBORDER_WIDTH+10)return; //can't show stuff here
	int widgetHeight = height();
	int maxLineWidth = widgetWidth;
	int defLeftCoord=KVI_IRCVIEW_HORIZONTAL_BORDER;
	if(KVI_OPTION_BOOL(KviOption_boolIrcViewShowImages))
	{
		maxLineWidth -= KVI_IRCVIEW_PIXMAP_SEPARATOR_AND_DOUBLEBORDER_WIDTH;
		defLeftCoord+=KVI_IRCVIEW_PIXMAP_AND_SEPARATOR;
	}


	int heightToPaint = 1;
	KviIrcViewLine * l = m_pCurLine;
	while(lines > 0)
	{
		if(l)
		{
			if(maxLineWidth != l->iMaxLineWidth)calculateLineWraps(l,maxLineWidth);
			heightToPaint += l->uLineWraps * m_iFontLineSpacing;
			heightToPaint += (m_iFontLineSpacing + m_iFontDescent);
			lines--;
			l = l->pPrev;
		} else lines = 0;
	}

	scroll(0,-(heightToPaint-1),QRect(1,1,widgetWidth-2,widgetHeight-2));

	if(m_iLastLinkRectHeight > -1)
	{
		// need to kill the last highlighted link
		m_iLastLinkRectTop -= heightToPaint;
		if(m_iLastLinkRectTop < 0)
		{
			m_iLastLinkRectHeight += m_iLastLinkRectTop;
			m_iLastLinkRectTop = 0;
		}
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The IrcView : THE paint event
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void KviIrcView::paintEvent(QPaintEvent *p)
{
	//
	// THIS FUNCTION IS A MONSTER
	//

	/*
	 * Profane description: this is ircview's most important function. It takes a lot of cpu cycles to complete, so we want to be sure
	 * it's well optimized. First, we want to skip this method everytime it's useless: it we're too short or we're covered by other windows.
	 */
	int scrollbarWidth = m_pScrollBar->width();
	int widgetWidth  = width() - scrollbarWidth;
	if(!isVisible() || (widgetWidth < 20))
	{
		m_iUnprocessedPaintEventRequests = 0; // assume a full repaint when this widget is shown...
		return; //can't show stuff here
	}

	// if the mdiManager is in SDI mode and this window is attacched but is not the toplevel one
	// then it is hidden completely behind the other windows and we can avoid to paint it :)
	if(g_pFrame->mdiManager()->isInSDIMode() &&
		(m_pKviWindow->mdiParent() != g_pFrame->mdiManager()->topChild()) &&
		(m_pKviWindow->mdiParent()))
	{
		m_iUnprocessedPaintEventRequests = 0; // assume a full repaint when this widget is shown...
		return; // totally hidden behind other windows
	}

	int widgetHeight = height();

	static QRect r; // static: avoid calling constructor and destructor every time...

	if(p)
	{
		r=p->rect(); // app triggered , or self triggered from fastScroll (in that case m_iUnprocessedPaintEventRequests is set to 0 there)
		if(r == rect())
			m_iUnprocessedPaintEventRequests = 0; // only full repaints reset
	} else {
		// A self triggered event
		m_iUnprocessedPaintEventRequests = 0; // only full repaints reset
		r = rect();
	}

	/*
	 * Profane description: we start the real paint here: set some geometry, a font, and paint the background
	 */
	int rectLeft   = r.x();
	int rectTop    = r.y();
	int rectHeight = r.height();
	int rectBottom = rectTop + rectHeight;
	int rectWidth  = r.width();
	if(rectWidth > widgetWidth)rectWidth = widgetWidth;

	QPainter pa(this);

	SET_ANTI_ALIASING(pa);

	pa.setFont(font());
	if(!m_pFm)
	{
		// note that QFontMetrics(pa.font()) may be not the same as QFontMetrics(font())
		// because the painter might effectively use an approximation of the QFont specified
		// by font().
		recalcFontVariables(QFontMetrics(pa.font()),pa.fontInfo());
	}

#ifdef COMPILE_PSEUDO_TRANSPARENCY
	if(g_pShadedChildGlobalDesktopBackground)
	{
		QPoint pnt = mapToGlobal(QPoint(rectLeft,rectTop));
		pa.drawTiledPixmap(rectLeft,rectTop,rectWidth,rectHeight,*g_pShadedChildGlobalDesktopBackground,pnt.x(),pnt.y());
	} else {
#endif
		QPixmap * pix = m_pPrivateBackgroundPixmap;

		if(!pix)
			pix = KVI_OPTION_PIXMAP(KviOption_pixmapIrcViewBackground).pixmap();

		pa.fillRect(rectLeft,rectTop,rectWidth,rectHeight,KVI_OPTION_COLOR(KviOption_colorIrcViewBackground));
		if(pix)
			KviPixmapUtils::drawPixmapWithPainter(&pa,pix,KVI_OPTION_UINT(KviOption_uintIrcViewPixmapAlign),r,widgetWidth,widgetHeight);
#ifdef COMPILE_PSEUDO_TRANSPARENCY
	}
#endif

	/*
	 * Profane description: after the background, start to paint the contents (a list of text lines with "dynamic contents", correctly
	 * wrapped at the right edge of this control).
	 */

	//Have lines visible
	int curBottomCoord = widgetHeight - KVI_IRCVIEW_VERTICAL_BORDER;
	int maxLineWidth   = widgetWidth;
	int defLeftCoord   = KVI_IRCVIEW_HORIZONTAL_BORDER;
	int lineWrapsHeight;

	// if we draw an icon as a line preamble, we have to change borders geometry accordingly
	if(KVI_OPTION_BOOL(KviOption_boolIrcViewShowImages))
	{
		maxLineWidth -= KVI_IRCVIEW_PIXMAP_SEPARATOR_AND_DOUBLEBORDER_WIDTH;
		defLeftCoord += KVI_IRCVIEW_PIXMAP_AND_SEPARATOR;
	}

	KviIrcViewLine *pCurTextLine = m_pCurLine;

	if(m_bMouseIsDown)
	{
		m_szLastSelectionLine = "";
		m_szLastSelection = "";
	}

	//Make sure that we have enough space to paint something...
	if(maxLineWidth < m_iMinimumPaintWidth)pCurTextLine=0;

	bool bLineMarkPainted = !KVI_OPTION_BOOL(KviOption_boolTrackLastReadTextViewLine);


	//And loop thru lines until we not run over the upper bound of the view
	while((curBottomCoord >= KVI_IRCVIEW_VERTICAL_BORDER) && pCurTextLine)
	{
		//Paint pCurTextLine
		if(maxLineWidth != pCurTextLine->iMaxLineWidth)
		{
			// Width of the widget or the font has been changed
			// from the last time that this line was painted
			calculateLineWraps(pCurTextLine,maxLineWidth);
		}

		// the evil multiplication
		// in an i486 it can get up to 42 clock cycles
		lineWrapsHeight  = (pCurTextLine->uLineWraps) * m_iFontLineSpacing;
		curBottomCoord  -= lineWrapsHeight;

		if((curBottomCoord - m_iFontLineSpacing) > rectBottom)
		{
			// not in update rect... skip
			curBottomCoord -= (m_iFontLineSpacing + m_iFontDescent);
			pCurTextLine = pCurTextLine->pPrev;
			continue;
		}

		if(KVI_OPTION_BOOL(KviOption_boolIrcViewShowImages))
		{
			//Paint the pixmap first
			//Calculate the position of the image
			//imageYPos = curBottomCoord - (pixmapHeight(16) + ((m_iFontLineSpacing - 16)/2) );
			int imageYPos = curBottomCoord - m_iRelativePixmapY;
			//Set the mask if needed
			int iPixId = KVI_OPTION_MSGTYPE(pCurTextLine->iMsgType).pixId();
			if(iPixId > 0)
				pa.drawPixmap(KVI_IRCVIEW_HORIZONTAL_BORDER,imageYPos,*(g_pIconManager->getSmallIcon(iPixId)));
		}

		if(m_pToolWidget)
		{
			if(!m_pToolWidget->messageEnabled(pCurTextLine->iMsgType))
			{
				// not in update rect... skip
				curBottomCoord -= (m_iFontLineSpacing + m_iFontDescent);
				pCurTextLine = pCurTextLine->pPrev;
				continue;
			}
		}

		// Initialize for drawing this line of text
		// The first block is always an attribute block
		char defaultBack  = pCurTextLine->pBlocks->pChunk->colors.back;
		char defaultFore  = pCurTextLine->pBlocks->pChunk->colors.fore;
		bool curBold      = false;
		bool curUnderline = false;
		char foreBeforeEscape= KVI_BLACK;
		bool curLink      = false;
		bool bacWasTransp = false;
		char curFore      = defaultFore;
		char curBack      = defaultBack;
		int  curLeftCoord = defLeftCoord;
		curBottomCoord   -= m_iFontDescent; //rise up the text...

		//
		// Single text line loop (paint all text blocks)
		// (May correspond to more physical lines on the display if the text is wrapped)
		//

		for(int i=0;i < pCurTextLine->iBlockCount;i++)
		{
			register KviIrcViewWrappedBlock * block = &(pCurTextLine->pBlocks[i]);

			// Play with the attributes
			if(block->pChunk)
			{
				//normal block
				switch(block->pChunk->type)
				{
					case KVI_TEXT_COLOR:
						if(block->pChunk->colors.fore != KVI_NOCHANGE)
						{
							curFore = block->pChunk->colors.fore;
							if(block->pChunk->colors.back != KVI_NOCHANGE)
								curBack = block->pChunk->colors.back;
						} else {
							// only a CTRL+K... reset
							curFore = defaultFore;
							curBack = defaultBack;
						}
						// Begin Edit by GMC-jimmy: Added
						// When KVIrc encounters a CTRL+K code without any trailing numbers, we then use KVIrc's default color value
						// defined by the user in the Options dialog.
						// This is to allow KVIrc to handle mIRC color codes in a similar fashion to most other modern irc clients.
						// See also kvi_input.cpp

						// Pragma: optimized: moved the code above (avoided duplicate if())
						// Pragma(05.03.2003): fixed again: reset ONLY if CTRL+K without numbers
						// otherwise leave the background unchanged

						//if(block->pChunk->colors.fore == KVI_NOCHANGE)
						//	curFore = defaultFore;
						//if(block->pChunk->colors.back == KVI_NOCHANGE)
						//	curBack = defaultBack;
						// End Edit
						break;
					case KVI_TEXT_ESCAPE:
						foreBeforeEscape      = curFore;
						if(block->pChunk->colors.fore != KVI_NOCHANGE)
							curFore = block->pChunk->colors.fore;
						if(m_pLastLinkUnderMouse == block)curLink = true;
						break;
					case KVI_TEXT_UNESCAPE:
						curLink            = false;
						curFore            = foreBeforeEscape;
						break;
					case KVI_TEXT_BOLD:
						curBold            = !curBold;
						break;
					case KVI_TEXT_UNDERLINE:
						curUnderline       = !curUnderline;
						break;
					case KVI_TEXT_RESET:
						curBold            = false;
						curUnderline       = false;
						curFore            = defaultFore;
						curBack            = defaultBack;
						break;
					case KVI_TEXT_REVERSE: //Huh ?
						char aux       = curBack;
						if(bacWasTransp == true)
						{
							curBack = KVI_TRANSPARENT;
						} else {
							curBack = curFore;
						}
						if(aux == KVI_TRANSPARENT)
						{
							curFore = (char)KVI_DEF_BACK;
						} else {
							curFore = aux;
						}
						bacWasTransp = (aux == KVI_TRANSPARENT);
/*						if(curBack != KVI_TRANSPARENT)
						{
							char aux       =curFore;
							curFore        = curBack;
							curBack        = aux;
						} else {
							curBack = curFore;
							switch(curBack)
							{
								case KVI_WHITE:
								case KVI_ORANGE:
								case KVI_YELLOW:
								case KVI_LIGHTGREEN:
								case KVI_BLUEMARINE:
								case KVI_LIGHTBLUE:
								case KVI_LIGHTVIOLET:
								case KVI_LIGHTGRAY:
									curFore=KVI_BLACK;
									break;
								default: //transparent too
									curFore=KVI_LIGHTGREEN;
									break;
							}
						}
*/
						break;
					//case KVI_TEXT_ICON:
					//case KVI_TEXT_UNICON:
						// does nothing
						//debug("Have a block with ICON/UNICON attr");
						//break;
				}

			} else {
				// no attributes , it is a line wrap
				curLeftCoord = defLeftCoord;
				if(KVI_OPTION_BOOL(KviOption_boolIrcViewWrapMargin))curLeftCoord+=m_iWrapMargin;
				curBottomCoord += m_iFontLineSpacing;
			}

//
// Here we run really out of bounds :)))))
// A couple of macros that could work well as functions...
// but since there are really many params to be passed
// and push & pop calls take clock cycles
// my paranoic mind decided to go for the macro way.
// This is NOT good programming
//

#define SET_PEN(_color,_custom)\
	if( ((unsigned char)_color) < 16 )\
	{\
		pa.setPen(KVI_OPTION_MIRCCOLOR((unsigned char)_color));\
	} else {\
		switch((unsigned char)_color)\
		{\
			case KVI_COLOR_EXT_USER_OP:\
				pa.setPen(KVI_OPTION_COLOR(KviOption_colorUserListViewOpForeground));\
				break;\
			case KVI_COLOR_EXT_USER_HALFOP:\
				pa.setPen(KVI_OPTION_COLOR(KviOption_colorUserListViewHalfOpForeground));\
				break;\
			case KVI_COLOR_EXT_USER_ADMIN:\
				pa.setPen(KVI_OPTION_COLOR(KviOption_colorUserListViewChanAdminForeground));\
				break;\
			case KVI_COLOR_EXT_USER_OWNER:\
				pa.setPen(KVI_OPTION_COLOR(KviOption_colorUserListViewChanOwnerForeground));\
				break;\
			case KVI_COLOR_EXT_USER_VOICE:\
				pa.setPen(KVI_OPTION_COLOR(KviOption_colorUserListViewVoiceForeground));\
				break;\
			case KVI_COLOR_EXT_USER_USEROP:\
				pa.setPen(KVI_OPTION_COLOR(KviOption_colorUserListViewUserOpForeground));\
				break;\
			case KVI_COLOR_EXT_USER_NORMAL:\
				pa.setPen(KVI_OPTION_COLOR(KviOption_colorUserListViewNormalForeground));\
				break;\
			case KVI_DEF_BACK :\
				pa.setPen(KVI_OPTION_COLOR(KviOption_colorIrcViewBackground));\
				break;\
			case KVI_COLOR_CUSTOM :\
				pa.setPen(_custom);\
				break;\
			case KVI_COLOR_OWN :\
				pa.setPen(KVI_OPTION_COLOR(KviOption_colorUserListViewOwnForeground));\
				break;\
		}\
	}

#define DRAW_SELECTED_TEXT(_text_str,_text_idx,_text_len,_text_width) \
	SET_PEN(KVI_OPTION_MSGTYPE(KVI_OUT_SELECT).fore(),block->pChunk ? block->pChunk->customFore : QColor()); \
	{ \
		int theWdth = _text_width; \
		if(theWdth < 0) \
			theWdth=width()-(curLeftCoord+KVI_IRCVIEW_HORIZONTAL_BORDER+scrollbarWidth); \
		pa.fillRect(curLeftCoord,curBottomCoord - m_iFontLineSpacing + m_iFontDescent,theWdth,m_iFontLineSpacing,KVI_OPTION_MIRCCOLOR(KVI_OPTION_MSGTYPE(KVI_OUT_SELECT).back())); \
	} \
	pa.drawText(curLeftCoord,curBottomCoord,_text_str.mid(_text_idx,_text_len)); \
	m_szLastSelectionLine.append(_text_str.mid(_text_idx,_text_len)); \
	curLeftCoord += _text_width;

#define DRAW_NORMAL_TEXT(_text_str,_text_idx,_text_len,_text_width) \
	SET_PEN(curFore,block->pChunk ? block->pChunk->customFore : QColor()); \
	if(curBack != KVI_TRANSPARENT){ \
		int theWdth = _text_width; \
		if(theWdth < 0) \
			theWdth=width()-(curLeftCoord+KVI_IRCVIEW_HORIZONTAL_BORDER+scrollbarWidth); \
		pa.fillRect(curLeftCoord,curBottomCoord - m_iFontLineSpacing + m_iFontDescent,theWdth,m_iFontLineSpacing,KVI_OPTION_MIRCCOLOR((unsigned char)curBack)); \
	} \
	pa.drawText(curLeftCoord,curBottomCoord,_text_str.mid(_text_idx,_text_len)); \
	if(curBold)pa.drawText(curLeftCoord+1,curBottomCoord,_text_str.mid(_text_idx,_text_len)); \
	if(curUnderline){ \
		int theWdth = _text_width; \
		if(theWdth < 0) \
			theWdth=width()-(curLeftCoord+KVI_IRCVIEW_HORIZONTAL_BORDER+scrollbarWidth); \
		pa.drawLine(curLeftCoord,curBottomCoord+2,curLeftCoord+theWdth,curBottomCoord+2); \
	} \
	curLeftCoord += _text_width;


// EOF macro declarations

			if(m_bMouseIsDown)
			{
				//Check if the block or a part of it is selected
				if(checkSelectionBlock(pCurTextLine,curLeftCoord,curBottomCoord,i))
				{
					//Selected in some way
					//__range_valid(g_pOptions->m_cViewOutSeleFore != KVI_TRANSPARENT);
					//__range_valid(g_pOptions->m_cViewOutSeleBack != KVI_TRANSPARENT);

					if(m_bShiftPressed && i && block->pChunk &&
						((m_pWrappedBlockSelectionInfo->selection_type == KVI_IRCVIEW_BLOCK_SELECTION_TOTAL) ||
						(m_pWrappedBlockSelectionInfo->selection_type == KVI_IRCVIEW_BLOCK_SELECTION_LEFT))
					)
					{
						switch(block->pChunk->type)
						{
							case KVI_TEXT_BOLD:
							case KVI_TEXT_UNDERLINE:
							case KVI_TEXT_REVERSE:
							case KVI_TEXT_RESET:
								m_szLastSelectionLine.append(QChar(block->pChunk->type));
							break;
							case KVI_TEXT_COLOR:
								m_szLastSelectionLine.append(QChar(block->pChunk->type));
								if((block->pChunk->colors.fore != KVI_NOCHANGE) && (block->pChunk->colors.fore != KVI_TRANSPARENT))
								{
									if(curFore > 9)m_szLastSelectionLine.append(QChar('1'));
									m_szLastSelectionLine.append(QChar((curFore%10)+'0'));
								}
								if((block->pChunk->colors.back != KVI_NOCHANGE) && (block->pChunk->colors.back != KVI_TRANSPARENT) )
								{
									m_szLastSelectionLine.append(QChar(','));
									if(curBack > 9)m_szLastSelectionLine.append(QChar('1'));
									m_szLastSelectionLine.append(QChar((curBack%10)+'0'));
								}
							break;
						}
					}

					switch(m_pWrappedBlockSelectionInfo->selection_type)
					{
						case KVI_IRCVIEW_BLOCK_SELECTION_TOTAL:
							DRAW_SELECTED_TEXT(pCurTextLine->szText,block->block_start,
								block->block_len,block->block_width)
						break;
						case KVI_IRCVIEW_BLOCK_SELECTION_LEFT:
							DRAW_SELECTED_TEXT(pCurTextLine->szText,block->block_start,
								m_pWrappedBlockSelectionInfo->part_1_length,
								m_pWrappedBlockSelectionInfo->part_1_width)
							DRAW_NORMAL_TEXT(pCurTextLine->szText,block->block_start+m_pWrappedBlockSelectionInfo->part_1_length,
								m_pWrappedBlockSelectionInfo->part_2_length,
								m_pWrappedBlockSelectionInfo->part_2_width)
						break;
						case KVI_IRCVIEW_BLOCK_SELECTION_RIGHT:
							DRAW_NORMAL_TEXT(pCurTextLine->szText,block->block_start,
								m_pWrappedBlockSelectionInfo->part_1_length,
								m_pWrappedBlockSelectionInfo->part_1_width)
							DRAW_SELECTED_TEXT(pCurTextLine->szText,block->block_start+m_pWrappedBlockSelectionInfo->part_1_length,
								m_pWrappedBlockSelectionInfo->part_2_length,
								m_pWrappedBlockSelectionInfo->part_2_width)
						break;
						case KVI_IRCVIEW_BLOCK_SELECTION_CENTRAL:
							DRAW_NORMAL_TEXT(pCurTextLine->szText,block->block_start,
								m_pWrappedBlockSelectionInfo->part_1_length,
								m_pWrappedBlockSelectionInfo->part_1_width)
							DRAW_SELECTED_TEXT(pCurTextLine->szText,block->block_start+m_pWrappedBlockSelectionInfo->part_1_length,
								m_pWrappedBlockSelectionInfo->part_2_length,
								m_pWrappedBlockSelectionInfo->part_2_width)
							DRAW_NORMAL_TEXT(pCurTextLine->szText,block->block_start+m_pWrappedBlockSelectionInfo->part_1_length+m_pWrappedBlockSelectionInfo->part_2_length,
								m_pWrappedBlockSelectionInfo->part_3_length,
								m_pWrappedBlockSelectionInfo->part_3_width)
						break;
						case KVI_IRCVIEW_BLOCK_SELECTION_ICON:
						{
							int theWdth = block->block_width;
							if(theWdth < 0)theWdth=width()-(curLeftCoord+KVI_IRCVIEW_HORIZONTAL_BORDER+scrollbarWidth);
							pa.fillRect(curLeftCoord,curBottomCoord - m_iFontLineSpacing + m_iFontDescent,theWdth,m_iFontLineSpacing,KVI_OPTION_MIRCCOLOR(KVI_OPTION_MSGTYPE(KVI_OUT_SELECT).back()));
							kvi_wslen_t the_len = kvi_wstrlen(block->pChunk->szPayload);
							m_szLastSelectionLine.append(QChar(block->pChunk->type));
							QString tmp;
							tmp.setUtf16(block->pChunk->szPayload,the_len);
							m_szLastSelectionLine.append(tmp);
							goto no_selection_paint;
						}
						break;
					}
				} else {
					if(block->pChunk && block->pChunk->type == KVI_TEXT_ICON)goto no_selection_paint;
					int wdth = block->block_width;
					if(wdth == 0)
					{
						// Last block before a word wrap , or a zero characters attribute block ?
						if(i < (pCurTextLine->iBlockCount - 1))
						{
							// There is another block...
							// Check if it is a wrap...
							if(pCurTextLine->pBlocks[i+1].pChunk == 0)wdth = widgetWidth-(curLeftCoord+KVI_IRCVIEW_HORIZONTAL_BORDER);
							// else simply a zero characters block
						}
						// else simply a zero characters block
					}
					DRAW_NORMAL_TEXT(pCurTextLine->szText,block->block_start,block->block_len,wdth)
				}
			} else {
				//No selection ...go fast!
no_selection_paint:
				if(block->pChunk && block->pChunk->type == KVI_TEXT_ICON)
				{
					int wdth = block->block_width;
					if(wdth < 0)wdth = widgetWidth - (curLeftCoord + KVI_IRCVIEW_HORIZONTAL_BORDER);
					int imageYPos = curBottomCoord - m_iRelativePixmapY;
					//Set the mask if needed
					if(curBack != KVI_TRANSPARENT && curBack < 16)
					{
						pa.fillRect(curLeftCoord,curBottomCoord - m_iFontLineSpacing + m_iFontDescent,wdth,m_iFontLineSpacing,KVI_OPTION_MIRCCOLOR((unsigned char)curBack));
					}
					QString tmpQ;
					tmpQ.setUtf16(block->pChunk->szSmileId,kvi_wstrlen(block->pChunk->szSmileId));
					QPixmap * daIcon =0;
					KviTextIcon* pIcon = g_pTextIconManager->lookupTextIcon(tmpQ);
					if(pIcon)
					{
						daIcon = pIcon->animatedPixmap() ? pIcon->animatedPixmap()->pixmap() : pIcon->pixmap();
					}
					if(!daIcon)
					{
						// this should never happen since we do a check
						// when building the text icon block , but.. better safe than sorry:
						// so... we lost some icons ? wrong associations ?
						// recover it by displaying the "question mark" icon
						daIcon = g_pIconManager->getSmallIcon(KVI_SMALLICON_HELP); // must be there, eventually null pixmap :D
					}
					int moredown = 1; //used to center imager vertically (pixels which the image is moved more down)
					moredown += ((m_iFontLineSpacing - daIcon->height()) / 2);
					pa.drawPixmap(curLeftCoord + m_iIconSideSpacing,imageYPos + moredown,*(daIcon));

					//debug("SHifting by %d",block->block_width);
					curLeftCoord += block->block_width;
				} else {

					int wdth = block->block_width;
					if(wdth < 0)wdth = widgetWidth - (curLeftCoord + KVI_IRCVIEW_HORIZONTAL_BORDER);

					// FIXME: We could avoid this XSetForeground if the curFore was not changed....

					SET_PEN(curFore,block->pChunk ? block->pChunk->customFore : QColor());

					if(curBack != KVI_TRANSPARENT && curBack < 16 )
					{
						pa.fillRect(curLeftCoord,curBottomCoord - m_iFontLineSpacing + m_iFontDescent,wdth,m_iFontLineSpacing,KVI_OPTION_MIRCCOLOR((unsigned char)curBack));
					}

					if(curLink)
					{
						SET_PEN(KVI_OPTION_MSGTYPE(KVI_OUT_LINK).fore(),block->pChunk ? block->pChunk->customFore : QColor());
						pa.drawText(curLeftCoord,curBottomCoord,pCurTextLine->szText.mid(block->block_start,block->block_len));
						pa.drawText(curLeftCoord+1,curBottomCoord,pCurTextLine->szText.mid(block->block_start,block->block_len));
						pa.drawLine(curLeftCoord,curBottomCoord+2,curLeftCoord+wdth,curBottomCoord+2);
					} else if(curBold) {
						//Draw doubled font (simulate bold)
						pa.drawText(curLeftCoord,curBottomCoord,pCurTextLine->szText.mid(block->block_start,block->block_len));
						pa.drawText(curLeftCoord + 1,curBottomCoord,pCurTextLine->szText.mid(block->block_start,block->block_len));
					} else {
						pa.drawText(curLeftCoord,curBottomCoord,pCurTextLine->szText.mid(block->block_start,block->block_len));
					}

					if(curUnderline)
					{
						//Draw a line under the text block....
						pa.drawLine(curLeftCoord,curBottomCoord+2,curLeftCoord+wdth,curBottomCoord+2);
					}
					curLeftCoord += block->block_width;
				}
			}
		}

		if(pCurTextLine == m_pCursorLine)
		{
			// paint the cursor line
			int iH = lineWrapsHeight + m_iFontLineSpacing;

			// workaround to fix "Warning:QPainter::setCompositionMode: PorterDuff modes not supported on device on win"
			#if defined(COMPILE_ON_WINDOWS) || defined(COMPILE_ON_MINGW)
			pa.fillRect(0,curBottomCoord - iH,widgetWidth,iH + (m_iFontDescent << 1),QBrush(QColor(0,0,0,200)));
			#else
			pa.setCompositionMode(QPainter::CompositionMode_SourceOut);
			pa.fillRect(0,curBottomCoord - iH,widgetWidth,iH + (m_iFontDescent << 1),QBrush(QColor(0,0,0,127)));
			pa.setCompositionMode(QPainter::CompositionMode_SourceOver);
			#endif
		}

		if(m_bMouseIsDown)
		{
			if(!m_szLastSelectionLine.isEmpty())
			{
				if(!m_szLastSelection.isEmpty())m_szLastSelection.prepend("\n");
				m_szLastSelection.prepend(m_szLastSelectionLine);
				m_szLastSelectionLine = "";
			}
		}

		curBottomCoord -= (lineWrapsHeight + m_iFontLineSpacing);

		if(pCurTextLine->uIndex == m_uLineMarkLineIndex)
		{
			if((curBottomCoord >= KVI_IRCVIEW_VERTICAL_BORDER) && !bLineMarkPainted)
			{
				// visible!
				bLineMarkPainted = true;
				//pa.setRasterOp(NotROP);

				// Pen setup for marker line
				QPen pen(KVI_OPTION_COLOR(KviOption_colorIrcViewMarkLine),KVI_OPTION_UINT(KviOption_uintIrcViewMarkerSize));

				switch(KVI_OPTION_UINT(KviOption_uintIrcViewMarkerStyle))
				{
					case 1:
						pen.setStyle(Qt::DashLine);
						break;
					case 2:
						pen.setStyle(Qt::SolidLine);
						break;
					case 3:
						pen.setStyle(Qt::DashDotLine);
						break;
					case 4:
						pen.setStyle(Qt::DashDotDotLine);
						break;
					default:
						pen.setStyle(Qt::DotLine);
				}

				pa.setPen(pen);
				pa.drawLine(0,curBottomCoord,widgetWidth,curBottomCoord);
				//pa.setRasterOp(CopyROP);
			} // else was partially visible only
		}

		pCurTextLine    = pCurTextLine->pPrev;
	}

	if(!bLineMarkPainted && pCurTextLine && (rectTop <= (KVI_IRCVIEW_VERTICAL_BORDER + 5)))
	{
		// the line mark hasn't been painted yet
		// need to find out if the mark is above the display
		// the mark might be somewhere before the current text line
		// find the first line that can't be painted in the view at all
		while((curBottomCoord >= KVI_IRCVIEW_VERTICAL_BORDER) && pCurTextLine)
		{
			// the line wraps for the visible lines MUST have been already calculated
			// for this view width
			lineWrapsHeight  = (pCurTextLine->uLineWraps) * m_iFontLineSpacing;
			curBottomCoord  -= lineWrapsHeight + m_iFontLineSpacing + m_iFontDescent;
			pCurTextLine = pCurTextLine->pPrev;
		}

		if(pCurTextLine)
		{
			// this is the first NOT visible
			// so pCurTextLine->pNext is the last visible one
			if(pCurTextLine->pNext)
			{
				if(pCurTextLine->pNext->uIndex >= m_uLineMarkLineIndex)
					bLineMarkPainted = true; // yes, its somewhere before or on this line
			} else {
				// no next line ? hm... compare to the not visible one.. but this should never happen
				if(pCurTextLine->uIndex >= m_uLineMarkLineIndex)
					bLineMarkPainted = true; // yes, its somewhere before or on this line
			}
			if(bLineMarkPainted)
			{
				// need to mark it!
				//pa.setRasterOp(NotROP);
				pa.setPen(QPen(KVI_OPTION_COLOR(KviOption_colorIrcViewMarkLine),1,Qt::DotLine));

				// Marker icon
				// 16(width) + 5(border) = 21
				int x = widgetWidth - 21;
				int y = KVI_IRCVIEW_VERTICAL_BORDER;
				/*
				* Old icon... what a lame code :D
				* pa.drawLine(x,y,x,y);
				* y++; pa.drawLine(x-1,y,x+1,y);
				* y++; pa.drawLine(x-2,y,x+2,y);
				* y++; pa.drawLine(x-3,y,x+3,y);
				* y++; pa.drawLine(x-4,y,x+4,y);
				*/
				QPixmap * pIcon = g_pIconManager->getSmallIcon(KVI_SMALLICON_UNREADTEXT);
				pa.drawPixmap(x,y,16,16,*pIcon);
				//pa.setRasterOp(CopyROP);
			}
		}
	}

	//Need to draw the sunken rect around the view now...
	pa.setPen(palette().dark().color());
	pa.drawLine(0,0,widgetWidth,0);
	pa.drawLine(0,0,0,widgetHeight);
	pa.setPen(palette().light().color());
	widgetWidth--;
	pa.drawLine(1,widgetHeight-1,widgetWidth,widgetHeight-1);
	pa.drawLine(widgetWidth,1,widgetWidth,widgetHeight);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The IrcView : calculate line wraps
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define IRCVIEW_WCHARWIDTH(__c) (((__c).unicode() < 0xff) ? m_iFontCharacterWidth[(__c).unicode()] : m_pFm->width(__c))

void KviIrcView::calculateLineWraps(KviIrcViewLine *ptr,int maxWidth)
{
	//
	// Another monster
	//

	if(maxWidth<=0) return;

	if(ptr->iBlockCount != 0)kvi_free(ptr->pBlocks); // free any previous wrap blocks
	ptr->pBlocks      = (KviIrcViewWrappedBlock *)kvi_malloc(sizeof(KviIrcViewWrappedBlock)); // alloc one block
	ptr->iMaxLineWidth       = maxWidth; // calculus for this width
	ptr->iBlockCount      = 0;        // it will be ++
	ptr->uLineWraps           = 0;        // no line wraps yet

	unsigned int curAttrBlock = 0;        // Current attribute block
	int curLineWidth          = 0;

	// init the first block
	ptr->pBlocks->block_start = 0;
	ptr->pBlocks->block_len   = 0;
	ptr->pBlocks->block_width = 0;
	ptr->pBlocks->pChunk  = &(ptr->pChunks[0]); // always an attribute block

	int maxBlockLen = ptr->pChunks->iTextLen; // ptr->pChunks[0].iTextLen

	const QChar * unicode = ptr->szText.unicode();

	for(;;)
	{
		//Calculate the block_width
		register const QChar * p = unicode + ptr->pBlocks[ptr->iBlockCount].block_start;
		int curBlockLen   = 0;
		int curBlockWidth = 0;

		if(ptr->pChunks[curAttrBlock].type == KVI_TEXT_ICON)
		{
			curBlockWidth = m_iIconWidth;
		} else {
			while(curBlockLen < maxBlockLen)
			{
				// FIXME: this is ugly :/
				curBlockWidth += IRCVIEW_WCHARWIDTH(*p);
				curBlockLen++;
				p++;
			}
		}
		//Check the length
		curLineWidth += curBlockWidth;

		if(curLineWidth < maxWidth)
		{
			//debug("Block of %d pix and len %d with type %d",ptr->pBlocks[ptr->iBlockCount].block_width,ptr->pBlocks[ptr->iBlockCount].block_len,ptr->pChunks[curAttrBlock].type);
			//Ok....proceed to next block
			ptr->pBlocks[ptr->iBlockCount].block_len   = curBlockLen;
			ptr->pBlocks[ptr->iBlockCount].block_width = curBlockWidth;
			curAttrBlock++;
			ptr->iBlockCount++;
			//Process the next block of data in the next loop or return if have no more blocks
			if(curAttrBlock < ptr->uChunkCount)
			{
				ptr->pBlocks = (KviIrcViewWrappedBlock *)kvi_realloc(ptr->pBlocks,(ptr->iBlockCount + 1) * sizeof(KviIrcViewWrappedBlock));
				ptr->pBlocks[ptr->iBlockCount].block_start = ptr->pChunks[curAttrBlock].iTextStart;
				ptr->pBlocks[ptr->iBlockCount].block_len = 0;
				ptr->pBlocks[ptr->iBlockCount].block_width = 0;
				ptr->pBlocks[ptr->iBlockCount].pChunk  = &(ptr->pChunks[curAttrBlock]);
				maxBlockLen = ptr->pBlocks[ptr->iBlockCount].pChunk->iTextLen;
			} else return;
		} else {
			//Need word wrap
			//First go back to an admissible width
			while((curLineWidth >= maxWidth) && curBlockLen)
			{
				p--;
				curBlockLen--;
				curLineWidth-=IRCVIEW_WCHARWIDTH(*p);
			}
			//Now look for a space
			while((*p != ' ') && curBlockLen)
			{
				p--;
				curBlockLen--;
				curLineWidth-=IRCVIEW_WCHARWIDTH(*p);
			}

			//If we ran up to the beginning of the block....
			if(curBlockLen == 0)
			{
				if(ptr->pChunks[curAttrBlock].type == KVI_TEXT_ICON)
				{
					// This is an icon block: needs to be wrapped differently:
					// The wrap block goes BEFORE the icon itself
					ptr->pBlocks[ptr->iBlockCount].pChunk  = 0;
					ptr->pBlocks[ptr->iBlockCount].block_width = 0;
					ptr->iBlockCount++;
					ptr->pBlocks = (KviIrcViewWrappedBlock *)kvi_realloc(ptr->pBlocks,(ptr->iBlockCount + 1) * sizeof(KviIrcViewWrappedBlock));
					ptr->pBlocks[ptr->iBlockCount].block_start = p - unicode;
					ptr->pBlocks[ptr->iBlockCount].block_len   = 0;
					ptr->pBlocks[ptr->iBlockCount].block_width = 0;
					ptr->pBlocks[ptr->iBlockCount].pChunk  = &(ptr->pChunks[curAttrBlock]);
					goto wrap_line;
				}
				//Don't like it....forced wrap here...
				//Go ahead up to the biggest possible string
				if(maxBlockLen > 0)
				{
					do
					{
						curBlockLen++;
						p++;
						curLineWidth+=IRCVIEW_WCHARWIDTH(*p);
					} while((curLineWidth < maxWidth) && (curBlockLen < maxBlockLen));
					//Now overrunned , go back 1 char
					p--;
					curBlockLen--;
				}
				//K...wrap
			} else {
				//found a space...
				//include it in the first block
				p++;
				curBlockLen++;
			}

			ptr->pBlocks[ptr->iBlockCount].block_len = curBlockLen;
			ptr->pBlocks[ptr->iBlockCount].block_width = -1; // word wrap --> negative block_width
			maxBlockLen-=curBlockLen;
			ptr->iBlockCount++;
			ptr->pBlocks = (KviIrcViewWrappedBlock *)kvi_realloc(ptr->pBlocks,(ptr->iBlockCount + 1) * sizeof(KviIrcViewWrappedBlock));
			ptr->pBlocks[ptr->iBlockCount].block_start = p - unicode;
			ptr->pBlocks[ptr->iBlockCount].block_len   = 0;
			ptr->pBlocks[ptr->iBlockCount].block_width = 0;
			ptr->pBlocks[ptr->iBlockCount].pChunk  = 0;
wrap_line:
			curLineWidth = 0;
			ptr->uLineWraps++;
			if(ptr->uLineWraps == 1)
			{
				if(KVI_OPTION_BOOL(KviOption_boolIrcViewWrapMargin))maxWidth-=m_iWrapMargin;
				if(maxWidth<=0) return;
			}
		}
	}

	ptr->iBlockCount++;
}

//================= calculateSelectionBounds ==================//

void KviIrcView::calculateSelectionBounds()
{
	if(m_mousePressPos.y() < m_mouseCurrentPos.y())
	{
		m_iSelectionTop     = m_mousePressPos.y();
		m_iSelectionBottom  = m_mouseCurrentPos.y();
		m_iSelectionBegin   = m_mousePressPos.x();
		m_iSelectionEnd     = m_mouseCurrentPos.x();
	} else {
		m_iSelectionTop     = m_mouseCurrentPos.y();
		m_iSelectionBottom  = m_mousePressPos.y();
		m_iSelectionBegin   = m_mouseCurrentPos.x();
		m_iSelectionEnd     = m_mousePressPos.x();
	}

	if(m_iSelectionBegin < m_iSelectionEnd)
	{
		m_iSelectionLeft    = m_iSelectionBegin;
		m_iSelectionRight   = m_iSelectionEnd;
	} else {
		m_iSelectionLeft    = m_iSelectionEnd;
		m_iSelectionRight   = m_iSelectionBegin;
	}
}


//=============== checkSelectionBlock ===============//

bool KviIrcView::checkSelectionBlock(KviIrcViewLine * line,int left,int bottom,int bufIndex)
{
	//
	// Yahoo!!!!
	//
	const QChar * unicode = line->szText.unicode();
	register const QChar * p = unicode + line->pBlocks[bufIndex].block_start;

	int top = bottom-m_iFontLineSpacing;
	int right  = ((line->pBlocks[bufIndex].block_width >= 0) ? \
					left+line->pBlocks[bufIndex].block_width : width()-(KVI_IRCVIEW_HORIZONTAL_BORDER + m_pScrollBar->width()));
	if(bottom < m_iSelectionTop)return false; //The selection starts under this line
	if(top > m_iSelectionBottom)return false; //The selection ends over this line
	if((top >= m_iSelectionTop)&&(bottom < m_iSelectionBottom))
	{
		//Whole line selected
		if(line->pBlocks[bufIndex].pChunk && line->pBlocks[bufIndex].pChunk->type == KVI_TEXT_ICON)
			m_pWrappedBlockSelectionInfo->selection_type = KVI_IRCVIEW_BLOCK_SELECTION_ICON;
		else
			m_pWrappedBlockSelectionInfo->selection_type = KVI_IRCVIEW_BLOCK_SELECTION_TOTAL;
		return true;
	}
	if((top < m_iSelectionTop) && (bottom >= m_iSelectionBottom))
	{
		//Selection begins and ends in this line
		if(right < m_iSelectionLeft)return false;
		if(left  > m_iSelectionRight)return false;
		if(line->pBlocks[bufIndex].pChunk && line->pBlocks[bufIndex].pChunk->type == KVI_TEXT_ICON)
		{
			m_pWrappedBlockSelectionInfo->selection_type = KVI_IRCVIEW_BLOCK_SELECTION_ICON;
			return true;
		}
		if((right <= m_iSelectionRight) && (left > m_iSelectionLeft))
		{
			//Whole line selected
			m_pWrappedBlockSelectionInfo->selection_type = KVI_IRCVIEW_BLOCK_SELECTION_TOTAL;
			return true;
		}
		if((right > m_iSelectionRight) && (left <= m_iSelectionLeft))
		{
			//Selection ends and begins in THIS BLOCK!
			m_pWrappedBlockSelectionInfo->selection_type = KVI_IRCVIEW_BLOCK_SELECTION_CENTRAL;
			m_pWrappedBlockSelectionInfo->part_1_length = 0;
			m_pWrappedBlockSelectionInfo->part_1_width  = 0;
			while((left <= m_iSelectionLeft) && (m_pWrappedBlockSelectionInfo->part_1_length < line->pBlocks[bufIndex].block_len)){
				int www = IRCVIEW_WCHARWIDTH(*p);
				left += www;
				m_pWrappedBlockSelectionInfo->part_1_width += www;
				p++;
				m_pWrappedBlockSelectionInfo->part_1_length++;
			}
			//Need to include the first character
			if(m_pWrappedBlockSelectionInfo->part_1_length > 0)
			{
				m_pWrappedBlockSelectionInfo->part_1_length--;
				p--;
				int www = IRCVIEW_WCHARWIDTH(*p);
				left -= www;
				m_pWrappedBlockSelectionInfo->part_1_width -= www;
			}
			int maxLenNow = line->pBlocks[bufIndex].block_len-m_pWrappedBlockSelectionInfo->part_1_length;
			int maxWidthNow = line->pBlocks[bufIndex].block_width-m_pWrappedBlockSelectionInfo->part_1_width;
			m_pWrappedBlockSelectionInfo->part_2_length = 0;
			m_pWrappedBlockSelectionInfo->part_2_width  = 0;
			while((left < m_iSelectionRight) && (m_pWrappedBlockSelectionInfo->part_2_length < maxLenNow))
			{
				int www = IRCVIEW_WCHARWIDTH(*p);
				left += www;
				m_pWrappedBlockSelectionInfo->part_2_width += www;
				p++;
				m_pWrappedBlockSelectionInfo->part_2_length++;
			}
			m_pWrappedBlockSelectionInfo->part_3_length = maxLenNow-m_pWrappedBlockSelectionInfo->part_2_length;
			m_pWrappedBlockSelectionInfo->part_3_width  = maxWidthNow-m_pWrappedBlockSelectionInfo->part_2_width;
			return true;
		}
		if(right > m_iSelectionRight)
		{
			//Selection ends in THIS BLOCK!
			m_pWrappedBlockSelectionInfo->selection_type = KVI_IRCVIEW_BLOCK_SELECTION_LEFT;
			m_pWrappedBlockSelectionInfo->part_1_length = 0;
			m_pWrappedBlockSelectionInfo->part_1_width  = 0;
			while((left < m_iSelectionRight) && (m_pWrappedBlockSelectionInfo->part_1_length < line->pBlocks[bufIndex].block_len))
			{
				int www = IRCVIEW_WCHARWIDTH(*p);
				left += www;
				m_pWrappedBlockSelectionInfo->part_1_width += www;
				p++;
				m_pWrappedBlockSelectionInfo->part_1_length++;
			}
			m_pWrappedBlockSelectionInfo->part_2_length = line->pBlocks[bufIndex].block_len-m_pWrappedBlockSelectionInfo->part_1_length;
			m_pWrappedBlockSelectionInfo->part_2_width  = line->pBlocks[bufIndex].block_width-m_pWrappedBlockSelectionInfo->part_1_width;
			//debug("%d",m_pWrappedBlockSelectionInfo->part_2_width);
			return true;
		}
		//Selection begins in THIS BLOCK!
		m_pWrappedBlockSelectionInfo->selection_type = KVI_IRCVIEW_BLOCK_SELECTION_RIGHT;
		m_pWrappedBlockSelectionInfo->part_1_length = 0;
		m_pWrappedBlockSelectionInfo->part_1_width  = 0;
		while((left <= m_iSelectionLeft) && (m_pWrappedBlockSelectionInfo->part_1_length < line->pBlocks[bufIndex].block_len))
		{
				int www = IRCVIEW_WCHARWIDTH(*p);
				left += www;
				m_pWrappedBlockSelectionInfo->part_1_width += www;
			p++;
			m_pWrappedBlockSelectionInfo->part_1_length++;
		}
		//Need to include the first character
		if(m_pWrappedBlockSelectionInfo->part_1_length > 0)
		{
			m_pWrappedBlockSelectionInfo->part_1_length--;
			p--;
			int www = IRCVIEW_WCHARWIDTH(*p);
			left -= www;
			m_pWrappedBlockSelectionInfo->part_1_width -= www;
		}
		m_pWrappedBlockSelectionInfo->part_2_length = line->pBlocks[bufIndex].block_len-m_pWrappedBlockSelectionInfo->part_1_length;
		m_pWrappedBlockSelectionInfo->part_2_width  = line->pBlocks[bufIndex].block_width-m_pWrappedBlockSelectionInfo->part_1_width;
		return true;
	}

	if(top < m_iSelectionTop)
	{
		//Selection starts in this line
		if(right < m_iSelectionBegin)return false;
		if(line->pBlocks[bufIndex].pChunk && line->pBlocks[bufIndex].pChunk->type == KVI_TEXT_ICON)
		{
			m_pWrappedBlockSelectionInfo->selection_type = KVI_IRCVIEW_BLOCK_SELECTION_ICON;
			return true;
		}
		if(left > m_iSelectionBegin)
		{
			//Whole block selected
			m_pWrappedBlockSelectionInfo->selection_type = KVI_IRCVIEW_BLOCK_SELECTION_TOTAL;
			return true;
		}
		//Selection begins in THIS BLOCK!
		m_pWrappedBlockSelectionInfo->selection_type = KVI_IRCVIEW_BLOCK_SELECTION_RIGHT;
		m_pWrappedBlockSelectionInfo->part_1_length = 0;
		m_pWrappedBlockSelectionInfo->part_1_width  = 0;
		while((left <= m_iSelectionBegin) && (m_pWrappedBlockSelectionInfo->part_1_length < line->pBlocks[bufIndex].block_len))
		{
			int www = IRCVIEW_WCHARWIDTH(*p);
			left += www;
			m_pWrappedBlockSelectionInfo->part_1_width += www;
			p++;
			m_pWrappedBlockSelectionInfo->part_1_length++;
		}
		//Need to include the first character
		if(m_pWrappedBlockSelectionInfo->part_1_length > 0)
		{
			m_pWrappedBlockSelectionInfo->part_1_length--;
			p--;
			int www = IRCVIEW_WCHARWIDTH(*p);
			left -= www;
			m_pWrappedBlockSelectionInfo->part_1_width -= www;
		}
		m_pWrappedBlockSelectionInfo->part_2_length = line->pBlocks[bufIndex].block_len-m_pWrappedBlockSelectionInfo->part_1_length;
		m_pWrappedBlockSelectionInfo->part_2_width  = line->pBlocks[bufIndex].block_width-m_pWrappedBlockSelectionInfo->part_1_width;
		return true;
	}
	//Selection ends in this line
	if(left  > m_iSelectionEnd)return false;
	if(line->pBlocks[bufIndex].pChunk && line->pBlocks[bufIndex].pChunk->type == KVI_TEXT_ICON)
	{
		m_pWrappedBlockSelectionInfo->selection_type = KVI_IRCVIEW_BLOCK_SELECTION_ICON;
		return true;
	}
	if(right < m_iSelectionEnd)
	{
		//Whole block selected
		m_pWrappedBlockSelectionInfo->selection_type = KVI_IRCVIEW_BLOCK_SELECTION_TOTAL;
		return true;
	}
	//Selection ends in THIS BLOCK!
	m_pWrappedBlockSelectionInfo->selection_type = KVI_IRCVIEW_BLOCK_SELECTION_LEFT;
	m_pWrappedBlockSelectionInfo->part_1_length = 0;
	m_pWrappedBlockSelectionInfo->part_1_width  = 0;
	while((left < m_iSelectionEnd) && (m_pWrappedBlockSelectionInfo->part_1_length < line->pBlocks[bufIndex].block_len))
	{
		int www = IRCVIEW_WCHARWIDTH(*p);
		left += www;
		m_pWrappedBlockSelectionInfo->part_1_width += www;
		p++;
		m_pWrappedBlockSelectionInfo->part_1_length++;
	}
	m_pWrappedBlockSelectionInfo->part_2_length = line->pBlocks[bufIndex].block_len-m_pWrappedBlockSelectionInfo->part_1_length;
	m_pWrappedBlockSelectionInfo->part_2_width  = line->pBlocks[bufIndex].block_width-m_pWrappedBlockSelectionInfo->part_1_width;
	return true;
}

//============ recalcFontVariables ==============//

void KviIrcView::recalcFontVariables(const QFontMetrics &fm,const QFontInfo &fi)
{
	// FIXME: #warning "OPTIMIZE THIS: GLOBAL VARIABLES"
	if(m_pFm)delete m_pFm;
	m_pFm = new QFontMetrics(fm);
	m_iFontLineSpacing = m_pFm->lineSpacing();
	if(m_iFontLineSpacing < KVI_IRCVIEW_PIXMAP_SIZE && KVI_OPTION_BOOL(KviOption_boolIrcViewShowImages))
	{
		m_iFontLineSpacing = KVI_IRCVIEW_PIXMAP_SIZE;
	}
	m_iFontDescent     =m_pFm->descent();
	m_iFontLineWidth   =m_pFm->lineWidth();
	// cache the first 256 characters
	for(unsigned short i=0;i<256;i++)
	{
		m_iFontCharacterWidth[i]=m_pFm->width(QChar(i));
	}
	if(m_iFontLineWidth==0)m_iFontLineWidth=1;
	m_iWrapMargin = m_pFm->width("wwww");
	//for(int i=0;i<256;i++)m_iFontCharacterWidth[i]=fm.width((char)i);
	m_iMinimumPaintWidth = (m_pFm->width('w') << 1)+m_iWrapMargin;
	m_iRelativePixmapY = (m_iFontLineSpacing + KVI_IRCVIEW_PIXMAP_SIZE) >> 1;
	m_iIconWidth = m_pFm->width("w");

	if(fi.fixedPitch() && (m_iIconWidth > 0))
	{
		while(m_iIconWidth < 18)m_iIconWidth += m_iIconWidth;
		m_iIconSideSpacing = (m_iIconWidth - 16) >> 1;
	} else {
		m_iIconWidth = 18;
		m_iIconSideSpacing = 1;
	}
}

//================ resizeEvent ===============//

void KviIrcView::resizeEvent(QResizeEvent *)
{
	int iScr = m_pScrollBar->sizeHint().width();
	int iLeft = width()-iScr;
	m_pToolsButton->setGeometry(iLeft,0,iScr,iScr);
	m_pScrollBar->setGeometry(iLeft,iScr,iScr,height() - iScr);

	if(m_pToolWidget)
	{
		if( ((m_pToolWidget->x() + m_pToolWidget->width()) > (iLeft - 1)) ||
			((m_pToolWidget->y() + m_pToolWidget->height()) > (height() - 1)))
		{
			m_pToolWidget->move(10,10);
		}
	}
}

QSize KviIrcView::sizeHint() const
{
	QSize ret(KVI_IRCVIEW_SIZEHINT_WIDTH,KVI_IRCVIEW_SIZEHINT_HEIGHT);
	return ret;
}

void KviIrcView::showToolsPopup()
{
	if(!m_pToolsPopup)
		m_pToolsPopup = new KviTalPopupMenu(this);

	m_pToolsPopup->clear();

	if(m_pToolWidget)
		m_pToolsPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_SEARCH)),__tr2qs("Hide Find Window"),this,SLOT(toggleToolWidget()));
	else
		m_pToolsPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_SEARCH)),__tr2qs("Show Find Window"),this,SLOT(toggleToolWidget()));
	m_pToolsPopup->insertSeparator();
	m_pToolsPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_PLUS)),__tr2qs("Zoom In"),this,SLOT(increaseFontSize()));
	m_pToolsPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_MINUS)),__tr2qs("Zoom Out"),this,SLOT(decreaseFontSize()));
	m_pToolsPopup->insertItem(__tr2qs("Choose Temporary Font..."),this,SLOT(chooseFont()));
	m_pToolsPopup->insertItem(__tr2qs("Choose Temporary Background..."),this,SLOT(chooseBackground()));
	int id = m_pToolsPopup->insertItem(__tr2qs("Reset Temporary Background"),this,SLOT(resetBackground()));
	m_pToolsPopup->setItemEnabled(id,m_pPrivateBackgroundPixmap != 0);
	m_pToolsPopup->insertSeparator();
	m_pToolsPopup->insertItem(__tr2qs("Clear Buffer"),this,SLOT(clearBuffer()));

	QSize s = m_pToolsPopup->sizeHint();

	m_pToolsPopup->popup(m_pToolsButton->mapToGlobal(QPoint(m_pToolsButton->width() - s.width(),m_pToolsButton->height())));
}

void KviIrcView::increaseFontSize()
{
	QFont f = font();
	f.setPointSize(f.pointSize() + 1);
	setFont(f);
}

void KviIrcView::decreaseFontSize()
{
	QFont f = font();
	int p = f.pointSize();
	if(p > 2)p--;
	f.setPointSize(p);
	setFont(f);
}

void KviIrcView::chooseFont()
{
	bool bOk;
	QFont f = QFontDialog::getFont(&bOk,font(),this);
	if(!bOk)return;
	setFont(f);
}

void KviIrcView::chooseBackground()
{
	QString f;
	if(!KviFileDialog::askForOpenFileName(f,__tr2qs("Choose the background image...")))return;
	if(f.isEmpty())return;
	QPixmap p(f);
	if(p.isNull())
	{
		QMessageBox::information(this,__tr2qs("Invalid image"),__tr2qs("Failed to load the selected image"),__tr2qs("Ok"));
		return;
	}
	setPrivateBackgroundPixmap(p);
}

void KviIrcView::resetBackground()
{
	setPrivateBackgroundPixmap(0);
}

void KviIrcView::toggleToolWidget()
{
	if(m_pToolWidget)
	{
		delete m_pToolWidget;
		m_pToolWidget = 0;
		m_pCursorLine = 0;
		repaint();

	} else {
		m_pToolWidget = new KviIrcViewToolWidget(this);
		int w = m_pToolWidget->sizeHint().width();
		m_pToolWidget->move(width() - (w + 40),10);
		m_pToolWidget->show();
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The IrcView : find
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void KviIrcView::setCursorLine(KviIrcViewLine * l)
{
	m_pCursorLine = l;
	if(m_pCursorLine == m_pCurLine)
	{

	repaint();

		return;
	}
	int sc = m_pScrollBar->value();
	l = m_pCurLine;
	if(m_pCursorLine->uIndex > m_pCurLine->uIndex)
	{
		// The cursor line is below the current line
		while(l && (l != m_pCursorLine))
		{
			l = l->pNext;
			sc++;
		}
		if(!l)return;
		if(sc != m_pScrollBar->value())
		{
			m_pCurLine = m_pCursorLine;
			m_iLastScrollBarValue = sc;
			m_pScrollBar->setValue(sc);
		} else {
			repaint();
		}
	} else {
		// The cursor line is over the current line
		// Here we're in trouble :D
		int curBottomCoord = height() - KVI_IRCVIEW_VERTICAL_BORDER;
		int maxLineWidth   = width();
		if(KVI_OPTION_BOOL(KviOption_boolIrcViewShowImages))maxLineWidth -= KVI_IRCVIEW_PIXMAP_SEPARATOR_AND_DOUBLEBORDER_WIDTH;
		//Make sure that we have enough space to paint something...
		if(maxLineWidth < m_iMinimumPaintWidth)return; // ugh
		//And loop thru lines until we not run over the upper bound of the view
		KviIrcViewLine * curLine = m_pCurLine;
		while(l)
		{
			if(maxLineWidth != l->iMaxLineWidth)calculateLineWraps(l,maxLineWidth);
			curBottomCoord -= (l->uLineWraps + 1) * m_iFontLineSpacing;
			while(curLine && (curBottomCoord < KVI_IRCVIEW_VERTICAL_BORDER))
			{
				if(curLine->iMaxLineWidth != maxLineWidth)calculateLineWraps(curLine,maxLineWidth);
				curBottomCoord += ((curLine->uLineWraps + 1) * m_iFontLineSpacing) + m_iFontDescent;
				curLine = curLine->pPrev;
				sc--;
			}
			if(l == m_pCursorLine)break;
			curBottomCoord -= m_iFontDescent;
			l = l->pPrev;
		}
		if(!curLine)return;
		if(sc != m_pScrollBar->value())
		{
			m_pCurLine = curLine;
			m_iLastScrollBarValue = sc;
			m_pScrollBar->setValue(sc);
		} else {
			repaint();
		}
	}
}

void KviIrcView::findNext(const QString& szText,bool bCaseS,bool bRegExp,bool bExtended)
{
	KviIrcViewLine * l = m_pCursorLine;
	if(!l)l = m_pCurLine;
	if(l)
	{
		l = l->pNext;
		if(!l)l = m_pFirstLine;
		KviIrcViewLine * start = l;

		int idx = -1;

		do{
			if(m_pToolWidget)
			{
				if(!(m_pToolWidget->messageEnabled(l->iMsgType)))goto do_pNext;
			}

			if(bRegExp)
			{
				QRegExp re(szText,bCaseS? Qt::CaseSensitive : Qt::CaseInsensitive,bExtended?QRegExp::RegExp : QRegExp::Wildcard);
				idx = re.indexIn(l->szText,0);
			} else {
				QString tmp = l->szText;
				idx = tmp.indexOf(szText,0,bCaseS?Qt::CaseSensitive:Qt::CaseInsensitive);
			}

			if(idx != -1)
			{
				setCursorLine(l);
				if(m_pToolWidget)
				{
					QString tmp;
					KviQString::sprintf(tmp,__tr2qs("Pos %d"),idx);
					m_pToolWidget->setFindResult(tmp);
				}
				return;
			}

do_pNext:

			l = l->pNext;
			if(!l)l = m_pFirstLine;

		} while(l != start);
	}
	m_pCursorLine = 0;
	repaint();
	if(m_pToolWidget)m_pToolWidget->setFindResult(__tr2qs("Not found"));
}


void KviIrcView::findPrev(const QString& szText,bool bCaseS,bool bRegExp,bool bExtended)
{
	KviIrcViewLine * l = m_pCursorLine;
	if(!l)l = m_pCurLine;
	if(l)
	{
		l = l->pPrev;
		if(!l)l = m_pLastLine;
		KviIrcViewLine * start = l;

		int idx = -1;

		do{

			if(m_pToolWidget)
			{
				if(!(m_pToolWidget->messageEnabled(l->iMsgType)))goto do_pPrev;
			}

			if(bRegExp)
			{
				QRegExp re(szText,bCaseS? Qt::CaseSensitive : Qt::CaseInsensitive,bExtended?QRegExp::RegExp : QRegExp::Wildcard);
				idx = re.indexIn(l->szText,0);
			} else {
				QString tmp = l->szText;
				idx = tmp.indexOf(szText,0,bCaseS?Qt::CaseSensitive:Qt::CaseInsensitive);
			}

			if(idx != -1)
			{
				setCursorLine(l);
				if(m_pToolWidget)
				{
					QString tmp;
					KviQString::sprintf(tmp,__tr2qs("Pos %d"),idx);
					m_pToolWidget->setFindResult(tmp);
				}
				return;
			}

do_pPrev:

			l = l->pPrev;
			if(!l)l = m_pLastLine;

		} while(l != start);
	}
	m_pCursorLine = 0;

	repaint();
	if(m_pToolWidget)m_pToolWidget->setFindResult(__tr2qs("Not found"));
}

/*
void KviIrcView::findClosestPositionInText(int xCursorPos,int yCursorPos,KviIrcViewPositionInText &pos)
{
	pos.pLine = getVisibleLineAt(xCursorPos,uCursorPos);
}
*/


KviIrcViewLine * KviIrcView::getVisibleLineAt(int,int yPos)
{
	KviIrcViewLine * l = m_pCurLine;
	int iTop = height() + m_iFontDescent - KVI_IRCVIEW_VERTICAL_BORDER;

	while(iTop > yPos)
	{
		if(l)
		{
			iTop -= ((l->uLineWraps + 1) * m_iFontLineSpacing) + m_iFontDescent;
			if(iTop <= yPos)return l;
			l = l->pPrev;
		} else return 0;
	}
	return 0;
}

KviIrcViewWrappedBlock * KviIrcView::getLinkUnderMouse(int xPos,int yPos,QRect * pRect,QString * linkCmd,QString * linkText)
{
	/*
	 * Profane description: this functions sums up most of the complications involved in the ircview. We got a mouse position and have
	 * to identify if there's a link inside the KviIrcViewLine at that position.
	 * l contains the current KviIrcViewLine we're checking, iTop is the y coordinate of the
	 * that line. We go from the bottom to the top: l is the last line and iTop is the y coordinate of the end of that line (imagine it
	 * as the beginning of the "next" line that have to come.
	 */

	KviIrcViewLine * l = m_pCurLine;
	int iTop = height() + m_iFontDescent - KVI_IRCVIEW_VERTICAL_BORDER;

	// our current line begins after the mouse position... go on
	while(iTop > yPos)
	{
		//no lines, go away
		if(!l)return 0;

		//subtract from iTop the height of the current line (aka go to the end of the previous / start of the current point)
		iTop -= ((l->uLineWraps + 1) * m_iFontLineSpacing) + m_iFontDescent;

		//we're still below the mouse position.. go on
		if(iTop > yPos)
		{
			// next round, try with the previous line
			l = l->pPrev;
			continue;
		}

		/*
		 * Profane description: if we are here we have found the right line where our mouse is over; l is the KviIrcViewLine *,
		 * iTop is the line start y coordinate. Now we have to go through this line's text and find the exact text under the mouse.
		 * The line start x posistion is iLeft; we save iTop to firstRowTop (many rows can be part of this lingle line of text)
		 */
		int iLeft = KVI_IRCVIEW_HORIZONTAL_BORDER;
		if(KVI_OPTION_BOOL(KviOption_boolIrcViewShowImages))iLeft += KVI_IRCVIEW_PIXMAP_AND_SEPARATOR;
		int firstRowTop = iTop;
		int i = 0;

		int iLastEscapeBlock = -1;
		int iLastEscapeBlockTop = -1;

		for(;;)
		{
			// if the mouse position is > start_of_this_row + row_height, move on to the next row of this line
			if(yPos > iTop + m_iFontLineSpacing)
			{
				// run until a word wrap block (aka a new line); move at least one block forward
				i++;
				while(i < l->iBlockCount)
				{
					if(l->pBlocks[i].pChunk == 0)
					{
						//word wrap found
						break;
					} else {
						//still ok to run right, but check if we find an url
						if(i >= l->iBlockCount) break;
						//we try to save the position of the last "text escape" tag we find
						if(l->pBlocks[i].pChunk)
							if(l->pBlocks[i].pChunk->type == KVI_TEXT_ESCAPE)
							{
								iLastEscapeBlock=i;
								iLastEscapeBlockTop=iTop;
							}
						//we reset the position of the last "text escape" tag if we find a "unescape"
						if(l->pBlocks[i].pChunk)
							if(l->pBlocks[i].pChunk->type == KVI_TEXT_UNESCAPE) iLastEscapeBlock=-1;

						i++;
					}
				}
				if(i >= l->iBlockCount) return 0; //we reached the last chunk... there's something wrong, return
				else iTop += m_iFontLineSpacing; //we found a word wrap, check the next row.
			} else {
			/*
			 * Profane description: Once we get here, we know the correct line l, the correct row top coordinate iTop and
			 * the index of the first chunk in this line i.
			 * Calculate the left border of this row: if this is not the first one, add any margin.
			 * Note: iLeft will contain the right border position of the current chunk.
			 */
				int iBlockWidth = 0;

				// this is not the first row of this line and the margin option is enabled?
				if(iTop != firstRowTop)
					if(KVI_OPTION_BOOL(KviOption_boolIrcViewWrapMargin))iLeft+=m_iWrapMargin;

				if(xPos < iLeft) return 0; // Mouse is out of this row boundaries
				for(;;)
				{
					int iLastLeft = iLeft;
					//we've run till the end of the line, go away
					if(i >= l->iBlockCount)return 0;
					//we try to save the position of the last "text escape" tag we find
					if(l->pBlocks[i].pChunk)
						if(l->pBlocks[i].pChunk->type == KVI_TEXT_ESCAPE)
						{
							iLastEscapeBlock=i;
							iLastEscapeBlockTop=iTop;
						}
					//we reset the position of the last "text escape" tag if we find a "unescape"
					if(l->pBlocks[i].pChunk)
						if(l->pBlocks[i].pChunk->type == KVI_TEXT_UNESCAPE) iLastEscapeBlock=-1;
					// if the block width is > 0, update iLeft
					if(l->pBlocks[i].block_width > 0)
					{
						iBlockWidth = l->pBlocks[i].block_width;
						iLeft += iBlockWidth;
					} else {
						if(i < (l->iBlockCount - 1))
						{
							// There is another block, check if it is a wrap (we reached the end of the row)
							if(l->pBlocks[i+1].pChunk == 0)
							{
								iBlockWidth = width() - iLastLeft;
								iLeft = width();
							}
							// else simply a zero characters block
						}
					}
					/*
					 * Profane description: mouse was not under the last chunk, try with this one..
					 */
					if(xPos < iLeft)
					{
						// Got it!
						// link ?
						bool bHadWordWraps = false;
						while(l->pBlocks[i].pChunk == 0)
						{
							// word wrap ?
							if(i >= 0)
							{
								i--;
								bHadWordWraps = true;
							} else return 0; // all word wraps ?!!!
						}
						if(iLastEscapeBlock != -1)
						{
 							int iLeftBorder=iLeft;
							int k;
							for(k = i ; k>=iLastEscapeBlock ; k--)
								iLeftBorder-=l->pBlocks[k].block_width;
							int iRightBorder=0;
							unsigned int uLineWraps = 0;
							for(k = iLastEscapeBlock;; k++)
							{
								if(l->pBlocks[k].pChunk)
									if(l->pBlocks[k].pChunk->type != KVI_TEXT_UNESCAPE)
										iRightBorder+=l->pBlocks[k].block_width;
									else
										break;
								else
								{
									uLineWraps++;
									bHadWordWraps=1;
								}
							}
							if(pRect)
							{
								*pRect = QRect(iLeftBorder,
										bHadWordWraps ? iLastEscapeBlockTop : iTop,
										iRightBorder,
										((uLineWraps + 1) * m_iFontLineSpacing) + m_iFontDescent);
							}
							if(linkCmd)
							{
								linkCmd->setUtf16(l->pBlocks[iLastEscapeBlock].pChunk->szPayload,kvi_wstrlen(l->pBlocks[iLastEscapeBlock].pChunk->szPayload));
								linkCmd->trimmed();
								if((*linkCmd)=="nc") (*linkCmd)="n";
							}
							if(linkText)
							{
								QString szLink;
								int iEndOfLInk = iLastEscapeBlock;
								while(1)
								{
									if(l->pBlocks[iEndOfLInk].pChunk)
										if(l->pBlocks[iEndOfLInk].pChunk->type != KVI_TEXT_UNESCAPE)
										{
											switch(l->pBlocks[iEndOfLInk].pChunk->type)
											{
												case KVI_TEXT_BOLD:
												case KVI_TEXT_UNDERLINE:
												case KVI_TEXT_REVERSE:
												case KVI_TEXT_RESET:
													szLink.append(QChar(l->pBlocks[iEndOfLInk].pChunk->type));
												break;
												case KVI_TEXT_COLOR:
													szLink.append(QChar(KVI_TEXT_COLOR));
													if(l->pBlocks[iEndOfLInk].pChunk->colors.fore != KVI_NOCHANGE)
													{
														szLink.append(QString("%1").arg((int)(l->pBlocks[iEndOfLInk].pChunk->colors.fore)));
													}
													if(l->pBlocks[iEndOfLInk].pChunk->colors.back != KVI_NOCHANGE)
													{
														szLink.append(QChar(','));
														szLink.append(QString("%1").arg((int)(l->pBlocks[iEndOfLInk].pChunk->colors.back)));
													}
												break;
											}
											szLink.append(l->szText.mid(l->pBlocks[iEndOfLInk].block_start,l->pBlocks[iEndOfLInk].block_len));
										} else
											break;
									iEndOfLInk++;

								}
								*linkText=szLink;
								// grab the rest of the link visible string
								// Continue while we do not find a non word wrap block block
								for(int bufIndex = (i + 1);bufIndex < l->iBlockCount;bufIndex++)
								{
									if(l->pBlocks[bufIndex].pChunk ) break; //finished : not a word wrap
									else {
										linkText->append(l->szText.mid(l->pBlocks[bufIndex].block_start,l->pBlocks[bufIndex].block_len));
									}
								}
							}
							return &(l->pBlocks[iLastEscapeBlock]);
						}
						if(l->pBlocks[i].pChunk->type == KVI_TEXT_ICON)
						{
							if(pRect)
							{
								*pRect = QRect(iLastLeft,
										bHadWordWraps ? firstRowTop : iTop,
										iBlockWidth,
										((l->uLineWraps + 1) * m_iFontLineSpacing) + m_iFontDescent);
							}
							if(linkCmd)
							{
								*linkCmd = "[!txt]";
								QString tmp;
								tmp.setUtf16(l->pBlocks[i].pChunk->szPayload,kvi_wstrlen(l->pBlocks[i].pChunk->szPayload));
								linkCmd->append(tmp);
								linkCmd->trimmed();
							}
							if(linkText)
							{
								*linkText = "";
							}
							return &(l->pBlocks[i]);
						}
						return 0;
					}
					i++;
				}
			}
		}
	}
	return 0;
}

KviConsole * KviIrcView::console()
{
	return m_pKviWindow->console();
}

bool KviIrcView::checkMarkerArea(const QRect & area, const QPoint & mousePos)
{
	return (area.contains(mousePos)) ? true : false;
}

void KviIrcView::animatedIconChange()
{
	update();
	//static int i = 0;
	//debug("animation %i",i);
	//i++;


	/*KviAnimatedPixmap* targetPixmap = (KviAnimatedPixmap*) sender();
	KviIrcViewLine   * targetLine = m_hAnimatedSmiles[targetPixmap];
	if(targetLine)
	{
		QRect repaintRect();
	}

	uint uWraps = 0;

	KviIrcViewLine * l = m_pCurLine;
	if(!l)return;

	int iTop = height() + m_iFontDescent - KVI_IRCVIEW_VERTICAL_BORDER;

	// our current line begins after the mouse position... go on
	while(iTop > KVI_IRCVIEW_VERTICAL_BORDER)
	{
		//subtract from iTop the height of the current line (aka go to the end of the previous / start of the current point)
		iTop -= ((l->uLineWraps + 1) * m_iFontLineSpacing) + m_iFontDescent;

		l = l->pPrev;
	}

	if(l)
	{
		update(QRect(0,iTop,width()-m_pScrollBar->width(),((l->uLineWraps + 1) * m_iFontLineSpacing)));
	}*/

}

#ifndef COMPILE_USE_STANDALONE_MOC_SOURCES
#include "kvi_ircview.moc"
#endif //!COMPILE_USE_STANDALONE_MOC_SOURCES
