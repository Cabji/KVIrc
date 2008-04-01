//=============================================================================
//
//   File : mp_xmmsinterface.cpp
//   Created on Fri 25 Mar 2005 20:04:54 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC client distribution
//   Copyright (C) 2005-2007 Szymon Stefanek <pragma at kvirc dot net>
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
//   Thnx to Alexander Stillich <torque at pltn dot org> for Audacious
//   media player interface hints :)
//
//=============================================================================

#include "mp_xmmsinterface.h"

#if (!defined(COMPILE_ON_WINDOWS) && !defined(Q_OS_MACX))

#include "kvi_locale.h"

MP_IMPLEMENT_DESCRIPTOR(
	KviXmmsInterface,
	"xmms",
	__tr2qs_ctx(
		"An interface to the popular UNIX xmms media player.\n" \
		"Download it from http://www.xmms.org\n"
		,
		"mediaplayer"
	)
)

MP_IMPLEMENT_DESCRIPTOR(
	KviAudaciousClassicInterface,
	"audacious classic",
	__tr2qs_ctx(
		"An interface to the popular UNIX audacious media player.\n" \
		"Download it from http://audacious-media-player.org\n"
		,
		"mediaplayer"
	)
)

static const char *xmms_lib_names[] =
{
	"libxmms.so",
	"libxmms.so.1",
	"/usr/lib/libxmms.so",
	"/usr/lib/libxmms.so.1",
	"/usr/local/lib/libxmms.so",
	"/usr/local/lib/libxmms.so.1",
	0
};

static const char *audacious_lib_names[] =
{
	"libaudacious.so",
	"libaudacious.so.4",
	"/usr/lib/libaudacious.so",
	"/usr/lib/libaudacious.so.4",
	"/usr/local/lib/libaudacious.so",
	"/usr/local/lib/libaudacious.so.4",
	0
};



KviXmmsInterface::KviXmmsInterface()
: KviMediaPlayerInterface()
{
	m_pPlayerLibrary = 0;
	m_szPlayerLibraryName = "libxmms.so";
	m_pLibraryPaths = xmms_lib_names;
}

KviXmmsInterface::~KviXmmsInterface()
{
	if(m_pPlayerLibrary)
	{
		kvi_library_close(m_pPlayerLibrary);
		m_pPlayerLibrary = 0;
	}
}

KviAudaciousClassicInterface::KviAudaciousClassicInterface()
: KviXmmsInterface()
{
	m_szPlayerLibraryName = "libaudacious.so";
	m_pLibraryPaths = audacious_lib_names;
}

KviAudaciousClassicInterface::~KviAudaciousClassicInterface()
{
}

bool KviXmmsInterface::loadPlayerLibrary()
{
	if(m_pPlayerLibrary)return true;

	const char **lib_name = m_pLibraryPaths;
	while(*lib_name)
	{
		m_pPlayerLibrary = kvi_library_load(*lib_name);
		if(m_pPlayerLibrary)
		{
			m_szPlayerLibraryName = *lib_name;
			break;
		}
		lib_name++;
	}
	return true;
}

void * KviXmmsInterface::lookupSymbol(const char * szSymbolName)
{
	if(!m_pPlayerLibrary)
	{
		if(!loadPlayerLibrary())
		{
			QString tmp;
			KviQString::sprintf(tmp,__tr2qs_ctx("Can't load the player library (%Q)","mediaplayer"),&m_szPlayerLibraryName);
			setLastError(tmp);
			return 0;
		}
	}
	void * symptr = kvi_library_symbol(m_pPlayerLibrary,szSymbolName);
	if(!symptr)
	{
		QString tmp;
		KviQString::sprintf(tmp,__tr2qs_ctx("Can't find symbol %s in %Q","mediaplayer"),szSymbolName,&m_szPlayerLibraryName);
		setLastError(tmp);
	}
	return symptr;
}


int KviXmmsInterface::detect(bool bStart)
{
	void * sym = lookupSymbol("xmms_remote_play");
	return sym ? 80 : 0;
}

#define XMMS_SIMPLE_CALL(__symname) \
	void (*sym)(int) = (void (*)(int))lookupSymbol(__symname); \
	if(!sym)return false; \
	sym(0); \
	return true;

bool KviXmmsInterface::prev()
{
	XMMS_SIMPLE_CALL("xmms_remote_playlist_prev")
}

bool KviXmmsInterface::next()
{
	XMMS_SIMPLE_CALL("xmms_remote_playlist_next")
}

bool KviXmmsInterface::play()
{
	XMMS_SIMPLE_CALL("xmms_remote_play")
}

bool KviXmmsInterface::stop()
{
	XMMS_SIMPLE_CALL("xmms_remote_stop")
}

bool KviXmmsInterface::pause()
{
	XMMS_SIMPLE_CALL("xmms_remote_pause")
}

bool KviXmmsInterface::quit()
{
	XMMS_SIMPLE_CALL("xmms_remote_quit")
}

bool KviXmmsInterface::jumpTo(kvs_int_t &iPos)
{
	void (*sym)(int,int) = (void (*)(int,int))lookupSymbol("xmms_remote_jump_to_time");
	if(!sym)return false;
	sym(0,iPos);
	return true;
}

bool KviXmmsInterface::setVol(kvs_int_t &iVol)
{
	void (*sym)(int,int) = (void (*)(int,int))lookupSymbol("xmms_remote_set_main_volume");
	if(!sym)return false;
	sym(0,100*iVol/255);
	return true;
}

int KviXmmsInterface::getVol()
{
	int (*sym)(int) = (int (*)(int))lookupSymbol("xmms_remote_get_main_volume");
	if(!sym)return -1;
	int iVol = sym(0);
	return iVol * 255 /100;
}

bool KviXmmsInterface::getRepeat()
{
	bool (*sym)(int) = (bool (*)(int))lookupSymbol("xmms_remote_is_repeat");
	if(!sym)return false;
	bool ret = sym(0);
	return ret;
}

bool KviXmmsInterface::setRepeat(bool &bVal)
{
	bool (*sym1)(int) = (bool (*)(int))lookupSymbol("xmms_remote_is_repeat");
	if(!sym1)return false;
	bool bNow = sym1(0);
	if(bNow != bVal)
	{
		void (*sym2)(int) = (void (*)(int))lookupSymbol("xmms_remote_toggle_repeat");
		if(!sym2)return false;
		sym2(0);
	}
	return true;
}

bool KviXmmsInterface::getShuffle()
{
	bool (*sym)(int) = (bool (*)(int))lookupSymbol("xmms_remote_is_shuffle");
	if(!sym)return false;
	bool ret = sym(0);
	return ret;
}

bool KviXmmsInterface::setShuffle(bool &bVal)
{
	bool (*sym1)(int) = (bool (*)(int))lookupSymbol("xmms_remote_is_shuffle");
	if(!sym1)return false;
	bool bNow = sym1(0);
	if(bNow != bVal)
	{
		void (*sym2)(int) = (void (*)(int))lookupSymbol("xmms_remote_toggle_shuffle");
		if(!sym2)return false;
		sym2(0);
	}
	return true;
}

KviMediaPlayerInterface::PlayerStatus KviXmmsInterface::status()
{
	bool (*sym1)(int) = (bool (*)(int))lookupSymbol("xmms_remote_is_paused");
	if(sym1)
	{
		if(sym1(0))return KviMediaPlayerInterface::Paused;
		bool (*sym2)(int) = (bool (*)(int))lookupSymbol("xmms_remote_is_playing");
		if(sym2)
		{
			if(sym2(0))return KviMediaPlayerInterface::Playing;
			else return KviMediaPlayerInterface::Stopped;
		}
	}

	return KviMediaPlayerInterface::Unknown;
}

bool KviXmmsInterface::playMrl(const QString &mrl)
{
	void (*sym)(int,char *) = (void (*)(int,char *))lookupSymbol("xmms_remote_playlist_add_url_string");
	KviQCString tmp = mrl.local8Bit();
	if(!tmp.isEmpty())
	{
		if(sym)
		{
			sym(0,tmp.data());
			int (*sym1)(int) = (int (*)(int))lookupSymbol("xmms_remote_get_playlist_length");
			if(sym1)
			{
				int len = sym1(0);
				if(len > 0)
				{
					void (*sym2)(int,int) = (void (*)(int,int))lookupSymbol("xmms_remote_set_playlist_pos");
					if(sym2)
					{
						sym2(0,len - 1);
					} else return false;
				} else return false;
			} else return false;
		} else return false;
	}
	return true;
}

QString KviXmmsInterface::nowPlaying()
{
	int (*sym)(int) = (int (*)(int))lookupSymbol("xmms_remote_get_playlist_pos");
	if(!sym)return QString::null;
	int pos = sym(0);
	char * (*sym2)(int,int) = (char * (*)(int,int))lookupSymbol("xmms_remote_get_playlist_title");
	if(!sym2)return QString::null;
	return QString::fromLocal8Bit(sym2(0,pos));
}

QString KviXmmsInterface::mrl()
{
	int (*sym)(int) = (int (*)(int))lookupSymbol("xmms_remote_get_playlist_pos");
	if(!sym)return QString::null;
	int pos = sym(0);
	char * (*sym2)(int,int) = (char * (*)(int,int))lookupSymbol("xmms_remote_get_playlist_file");
	if(!sym2)return QString::null;
	QString ret = QString::fromLocal8Bit(sym2(0,pos));
	if(ret.length() > 1)
		if(ret[0] == '/')ret.prepend("file://");
	return ret;
}

int KviXmmsInterface::position()
{
	int (*sym)(int) = (int (*)(int))lookupSymbol("xmms_remote_get_playlist_pos");
	if(!sym)return -1;
	int pos = sym(0);
	int (*sym2)(int,int) = (int (*)(int,int))lookupSymbol("xmms_remote_get_output_time");
	if(!sym2)return -1;
	return sym2(0,pos);
}

int KviXmmsInterface::length()
{
	int (*sym)(int) = (int (*)(int))lookupSymbol("xmms_remote_get_playlist_pos");
	if(!sym)return -1;
	int pos = sym(0);
	int (*sym2)(int,int) = (int (*)(int,int))lookupSymbol("xmms_remote_get_playlist_time");
	if(!sym2)return -1;
	return sym2(0,pos);
}

int KviXmmsInterface::getPlayListPos()
{
	int (*sym)(int) = (int (*)(int))lookupSymbol("xmms_remote_get_playlist_pos");
	if(!sym)return -1;
	return sym(0);
}



#endif //!COMPILE_ON_WINDOWS
