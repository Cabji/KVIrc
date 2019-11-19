//=============================================================================
//
//   File : KvsObject_popupMenu.cpp
//   Creation date : Fri Mar 18 21:30:48 CEST 2005
//   by Tonino Imbesi(Grifisx) and Alessandro Carbone(Noldor)
//
//   This file is part of the KVIrc IRC client distribution
//   Copyright (C) 2005-2008 Alessandro Carbone (elfonol at gmail dot com)
//
//   This program is FREE software. You can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your option) any later version.
//
//   This program is distributed in the HOPE that it will be USEFUL,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program. If not, write to the Free Software Foundation,
//   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
//=============================================================================

#include "KvsObject_popupMenu.h"

#include "KviError.h"
#include "kvi_debug.h"
#include "KviLocale.h"
#include "KviIconManager.h"

#include <QCursor>
#include <QMenu>
#include <QAction>

/*
	@doc: popupmenu
	@keyterms:
		popupmenu object class,
	@title:
		popupmenu class
	@type:
		class
	@short:
		Provides a popup menu.
	@inherits:
		[class]object[/class]
		[class]widget[/class]
	@description:
		This widget can be used to display a popup menu. Technically, a popup menu consists of a list of menu items.[br]
		You add items with insertItem(). An item can be a string. In addition, items can have an optional icon drawn on the very left side.[br]
	@functions:
		!fn: $insertItem(<text:string>,[icon_id:string])
		Inserts menu items into a popup menu with optional icon and return the popup identifier.
		!fn: $addMenu(<popupmenu:hobject,[idx:uinteger])
		Add a popupmenu.
		With the optional parameter idx the popup will be inserted.
		!fn: $setTitle(<text:string>)
		Sets the popupmenu title to text.
		!fn: $exec([<widget:objects>,<x:uinteger>,<y:integer>])
		If called without parameters show the popup menu at the current pointer position.[br]
		With the optional parameters show the popup menu at the coordinate x,y widget parameter relative.
		!fn: $removeItem(<popup_id:Uinteger>)
		Removes the menu item that has the identifier ID.
		!fn: $removeItemAt(<index:uinteger>)
		Removes the menu item at position index.
		!fn: $insertSeparator(<index:uinteger>)
		Inserts a separator at position index.[br]
		If the index is negative the separator becomes the last menu item.
		!fn: $activatedEvent(<popup_id:uinteger>)
		This function is called when a menu item and return the the item ID.[br]
		The default implementation emits the [classfnc]$activated[/classfnc]() signal.
		!fn: $highligthtedEvent(<popup_id:uinteger>)
		This function is called when a menu item is highlighted (hovered) and return the item ID.[br]
		The default implementation emits the [classfnc]$highlighted[/classfnc]() signal.
	@signals:
		!sg: $activated()
		This signal is emitted by the default implementation of [classfnc]$activatedEvent[/classfnc]().
		!sg: $highlighted()
		This signal is emitted by the default implementation of [classfnc]$highlightedEvent[/classfnc]().
		[br]
	@examples:
		[example]
			// note: this code is designed for executing in Script Tester
			// you would design this differently if you are using classes

			// create a base widget
			%Kapp = $new(widget)

			// create a submenu for the popup menu
			%subMenu = $new(popupmenu)
			%submenu->$setTitle("Sub Menu")
			// assign the returned uid to a hash key, and give it a string name we can use in the slot (handler)
			%PumIds{"%subMenu->$insertItem("Extra stuff 1", 302)"} = "Extra stuff 1"
			%PumIds{"%subMenu->$insertItem("Extra stuff 2", 303)"} = "Extra stuff 2"
			%PumIds{"%subMenu->$insertItem("Extra stuff 3", 304)"} = "Extra stuff 3"
			%subMenu->$insertSeparator(0)
			%PumIds{"%subMenu->$insertItem("Extra stuff 4", 305)"} = "Extra stuff 4"
			%PumIds{"%subMenu->$insertItem("Extra stuff 5", 306)"} = "Extra stuff 5"

			// create the primary popup menu
			%Kapp->%pumOppClick = $new(popupmenu, %Kapp)
			%Kapp->%pumOppClick->$setTitle("Popup Menu")
			%PumIds{"%Kapp->%pumOppClick->$insertItem("File", $icon("file"))"} = "File"
			%PumIds{"%Kapp->%pumOppClick->$insertItem("Edit", $icon("editor"))"} = "Edit"
			%PumIds{"%Kapp->%pumOppClick->$insertItem("Help", $icon("help"))"} = "Help"

			// add the submenu to the primary menu
			%Kapp->%pumOppClick->$addMenu(%submenu, 0)

			// when we click the mouse, the popup menu will appear
			privateimpl (%Kapp, mousePressEvent)
			{
				@%pumOppClick->$exec()
			}

			// when we click something in the popup menu, this handler will execute
			privateimpl (%Kapp, handlePopupEvent)
			{
				// $0 will contain the uid of the popup menu item that was clicked
				debug -c "Popup Menu Item: %PumIds{$0}";
			}

			// connect the primary popupmenu's "activated" signal to the %Kapp's "handlePopupEvent" slot
			objects.connect %Kapp->%pumOppClick activated %Kapp handlePopupEvent

			// display the base widget on screen
			%Kapp->$show()
		[/example]
*/

static QHash<int, QAction *> actionsDict;
static int iIdentifier = 0;

static int addActionToDict(QAction * pAction)
{
	actionsDict.insert(iIdentifier, pAction);
	iIdentifier++;
	return (iIdentifier - 1);
}
static void removeMenuAllActions(QMenu * pMenu)
{
	QList<QAction *> pActionsList = pMenu->actions();
	QList<QAction *> pActionsListDictValues = actionsDict.values();
	for(int i = 0; i < pActionsList.count(); i++)
	{
		int iIdx = pActionsListDictValues.indexOf(pActionsList.at(i));
		if(iIdx >= 0)
			actionsDict.remove(actionsDict.key(pActionsList.at(i)));
	}
}

static QAction * getAction(int idx)
{
	return actionsDict.value(idx);
}
static void removeAction(int idx)
{
	actionsDict.remove(idx);
}

KVSO_BEGIN_REGISTERCLASS(KvsObject_popupMenu, "popupmenu", "widget")
KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_popupMenu, insertItem)
KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_popupMenu, setTitle)
KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_popupMenu, exec)
KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_popupMenu, insertSeparator)
KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_popupMenu, removeItem)
KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_popupMenu, addMenu)

// events
KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_popupMenu, highlightedEvent)
KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_popupMenu, activatedEvent)

KVSO_END_REGISTERCLASS(KvsObject_popupMenu)

KVSO_BEGIN_CONSTRUCTOR(KvsObject_popupMenu, KvsObject_widget)

KVSO_END_CONSTRUCTOR(KvsObject_popupMenu)

KVSO_BEGIN_DESTRUCTOR(KvsObject_popupMenu)

KVSO_END_DESTRUCTOR(KvsObject_popupMenu)

bool KvsObject_popupMenu::init(KviKvsRunTimeContext *, KviKvsVariantList *)
{
	SET_OBJECT(QMenu)
	connect(widget(), SIGNAL(triggered(QAction *)), this, SLOT(slottriggered(QAction *)));
	connect(widget(), SIGNAL(destroyed(QObject *)), this, SLOT(aboutToDie(QObject *)));
	connect(widget(), SIGNAL(hovered(QAction *)), this, SLOT(slothovered(QAction *)));
	return true;
}

KVSO_CLASS_FUNCTION(popupMenu, insertItem)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szItem, szIcon;
	KVSO_PARAMETERS_BEGIN(c)
	KVSO_PARAMETER("text", KVS_PT_STRING, 0, szItem)
	KVSO_PARAMETER("icon_id", KVS_PT_STRING, KVS_PF_OPTIONAL, szIcon)
	KVSO_PARAMETERS_END(c)

	QPixmap * pix = nullptr;
	QAction * pAction = nullptr;
	if(!szIcon.isEmpty())
	{
		pix = g_pIconManager->getImage(szIcon);
		if(pix)
			pAction = ((QMenu *)widget())->addAction(*pix, szItem);
		else
			c->warning(__tr2qs_ctx("Icon '%Q' doesn't exist", "objects"), &szIcon);
	}
	else
		pAction = ((QMenu *)widget())->addAction(szItem);
	int identifier = addActionToDict(pAction);
	c->returnValue()->setInteger((kvs_int_t)identifier);
	return true;
}

KVSO_CLASS_FUNCTION(popupMenu, setTitle)
{
	QString szTitle;
	KVSO_PARAMETERS_BEGIN(c)
	KVSO_PARAMETER("title", KVS_PT_STRING, 0, szTitle)
	KVSO_PARAMETERS_END(c)
	if(!widget())
		return true;
	((QMenu *)widget())->setTitle(szTitle);

	return true;
}

KVSO_CLASS_FUNCTION(popupMenu, exec)
{
	CHECK_INTERNAL_POINTER(widget())
	if(!c->params()->count())
	{
		((QMenu *)widget())->exec(QCursor::pos());
		return true;
	}

	KviKvsObject * pObject;
	kvs_uint_t iX, iY;
	QString szLabel, szIcon;
	kvs_hobject_t hObject;
	KVSO_PARAMETERS_BEGIN(c)
	KVSO_PARAMETER("widget", KVS_PT_HOBJECT, 0, hObject)
	KVSO_PARAMETER("x", KVS_PT_UNSIGNEDINTEGER, 0, iX)
	KVSO_PARAMETER("y", KVS_PT_UNSIGNEDINTEGER, 0, iY)
	KVSO_PARAMETERS_END(c)
	pObject = KviKvsKernel::instance()->objectController()->lookupObject(hObject);
	CHECK_HOBJECT_IS_WIDGET(pObject)
	((QMenu *)widget())->exec(((QWidget *)(pObject->object()))->mapToGlobal(QPoint(iX, iY)));

	return true;
}

KVSO_CLASS_FUNCTION(popupMenu, addMenu)
{
	CHECK_INTERNAL_POINTER(widget())
	KviKvsObject * pObject;
	kvs_uint_t iIdx;
	kvs_hobject_t hObject;
	KVSO_PARAMETERS_BEGIN(c)
	KVSO_PARAMETER("popupmenu", KVS_PT_HOBJECT, 0, hObject)
	KVSO_PARAMETER("index", KVS_PT_UNSIGNEDINTEGER, KVS_PF_OPTIONAL, iIdx)
	KVSO_PARAMETERS_END(c)

	pObject = KviKvsKernel::instance()->objectController()->lookupObject(hObject);
	if(!pObject)
	{
		c->warning(__tr2qs_ctx("Popup menu parameter is not an object", "objects"));
		return true;
	}
	if(!pObject->object())
	{
		c->warning(__tr2qs_ctx("Popup menu parameter is not a valid object", "objects"));
		return true;
	}
	if(!pObject->inheritsClass("popupmenu"))
	{
		c->warning(__tr2qs_ctx("Popupmenu object required", "objects"));
		return true;
	}
	QAction * pAction;
	if(iIdx)
	{
		QAction * pActionBefore = actionsDict[iIdx];
		pAction = ((QMenu *)widget())->insertMenu(pActionBefore, (QMenu *)pObject->object());
	}
	else
	{
		pAction = ((QMenu *)widget())->addMenu((QMenu *)pObject->object());
	}
	int identifier = addActionToDict(pAction);
	c->returnValue()->setInteger((kvs_int_t)identifier);
	identifier++;
	return true;
}

KVSO_CLASS_FUNCTION(popupMenu, removeItem)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_int_t iIdx;
	KVSO_PARAMETERS_BEGIN(c)
	KVSO_PARAMETER("item_id", KVS_PT_INTEGER, 0, iIdx)
	KVSO_PARAMETERS_END(c)
	QAction * pAction = getAction(iIdx);
	if(pAction)
	{
		((QMenu *)widget())->removeAction(pAction);
		removeAction(iIdx);
	}
	return true;
}

KVSO_CLASS_FUNCTION(popupMenu, insertSeparator)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_uint_t iIndex;
	KVSO_PARAMETERS_BEGIN(c)
	KVSO_PARAMETER("index", KVS_PT_UNSIGNEDINTEGER, 0, iIndex)
	KVSO_PARAMETERS_END(c)
	QAction * pAction = getAction(iIndex);
	if(pAction)
		((QMenu *)widget())->insertSeparator(pAction);
	return true;
}

void KvsObject_popupMenu::slothovered(QAction * a)
{
	QHashIterator<int, QAction *> i(actionsDict);
	bool bFound = false;
	while(i.hasNext())
	{
		i.next();
		if(i.value() == a)
		{
			bFound = true;
			break;
		}
	}

	// check if the action was created inside this class
	if(bFound)
	{
		KviKvsVariantList params(new KviKvsVariant((kvs_int_t)i.key()));
		callFunction(this, "highlightedEvent", &params);
	}
}
void KvsObject_popupMenu::aboutToDie(QObject * pObject)
{
	qDebug("Removing popup from KVS dict");
	removeMenuAllActions((QMenu *)pObject);
}
KVSO_CLASS_FUNCTION(popupMenu, highlightedEvent)
{
	emitSignal("highlighted", c, c->params());
	return true;
}

void KvsObject_popupMenu::slottriggered(QAction * a)
{
	QHashIterator<int, QAction *> i(actionsDict);
	bool bFound = false;
	while(i.hasNext())
	{
		i.next();
		if(i.value() == a)
		{
			bFound = true;
			break;
		}
	}

	// check if the action was created inside this class
	if(bFound)
	{
		KviKvsVariantList params(new KviKvsVariant((kvs_int_t)i.key()));
		callFunction(this, "activatedEvent", &params);
	}
}

KVSO_CLASS_FUNCTION(popupMenu, activatedEvent)
{
	emitSignal("activated", c, c->params());
	return true;
}
