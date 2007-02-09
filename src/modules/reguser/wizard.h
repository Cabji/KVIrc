#ifndef _WIZARD_H_
#define _WIZARD_H_
//
//   File : wizard.h
//   Creation date : Fri Jun 26 2002 21:21:21 CEST by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2002 Szymon Stefanek (pragma at kvirc dot net)
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

#include "kvi_string.h"

#include <kvi_tal_wizard.h>

class KviPixmap;
class KviPixmapSelector;
class KviTalVBoxLayout; 
class KviTalHBoxLayout; 
class QGridLayout; 
class QCheckBox;
class QLabel;
class QLineEdit;
class QWidget;
class KviRegisteredUserDataBase;

class KviRegistrationWizard : public KviTalWizard
{ 
    Q_OBJECT
public:
    KviRegistrationWizard(const char * startMask,KviRegisteredUserDataBase * db = 0,QWidget * par = 0,bool bModal = false);
    ~KviRegistrationWizard();

	KviRegisteredUserDataBase * m_pDb;

	KviStr m_szStartMask;

	bool m_bModal;

    QWidget* m_pPage1;
    QLabel* m_pLabel1;
    QLineEdit* m_pEditRealName;
    QWidget* m_pPage2;
    QLabel* m_pLabel2;
    QLineEdit* m_pNicknameEdit1;
    QLineEdit* m_pNicknameEdit2;
    QLineEdit* m_pHostEdit1;
    QLineEdit* m_pHostEdit2;
    QLineEdit* m_pUsernameEdit1;
    QLineEdit* m_pUsernameEdit2;
    QWidget* m_pPage3;
    QLabel* m_pLabel3;
	KviPixmapSelector * m_pAvatarSelector;
    QWidget* m_pPage4;
    QCheckBox* m_pNotifyCheck;
    QLabel* m_pNotifyNickLabel1;
    QLabel* m_pNotifyNickLabel2;
    QLabel* m_pLabel4;
    QLineEdit* m_pNotifyNickEdit1;
    QLineEdit* m_pNotifyNickEdit2;
    QWidget* m_pPage5;
    QLabel* m_pTextLabel5;

	KviPixmap * m_pAvatar;
protected:
	virtual void showEvent(QShowEvent *e);
	virtual void accept();
	virtual void reject();
protected slots:
	void realNameChanged(const QString &str);
	void maskChanged(const QString &str);
	void notifyNickChanged(const QString &);
	void notifyCheckToggled(bool);
};

#endif // _WIZARD_H_
