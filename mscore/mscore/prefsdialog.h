//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id:$
//
//  Copyright (C) 2002-2010 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __PREFSDIALOG_H__
#define __PREFSDIALOG_H__

#include "ui_prefsdialog.h"

class Shortcut;
class Preferences;

//---------------------------------------------------------
//   PreferenceDialog
//---------------------------------------------------------

class PreferenceDialog : public QDialog, private Ui::PrefsDialogBase {
      Q_OBJECT

      QMap<QString, Shortcut*> localShortcuts;
      bool shortcutsChanged;
      QButtonGroup* recordButtons;

      void apply();
      bool sfChanged;
      void updateSCListView();
      void setUseMidiOutput(bool);
      void updateValues(Preferences*);

   private slots:
      void buttonBoxClicked(QAbstractButton*);
      void bgClicked(bool);
      void fgClicked(bool);
      void selectFgWallpaper();
      void selectBgWallpaper();
      void selectWorkingDirectory();
      void selectInstrumentList();
      void selectStartWith();
      void playPanelCurClicked();
      void resetShortcutClicked();
      void clearShortcutClicked();
      void defineShortcutClicked();
      void portaudioApiActivated(int idx);
      void resetAllValues();
      void paperSizeChanged(double);
      void pageFormatSelected(int);
      void landscapeToggled(bool);
      void styleFileButtonClicked();
      void recordButtonClicked(int);

   signals:
      void preferencesChanged();

   public:
      PreferenceDialog(QWidget* parent);
      void updateRemote();
      };

#endif

