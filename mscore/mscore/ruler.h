//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id:$
//
//  Copyright (C) 2009 Werner Schweer and others
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

#ifndef __RULER_H__
#define __RULER_H__

#include "pos.h"

static const int rulerHeight = 28;

//---------------------------------------------------------
//   Ruler
//---------------------------------------------------------

class Ruler : public QWidget {
      Q_OBJECT

      Score* _score;
      Pos _cursor;
      bool _showCursor;
      int metronomeRulerMag;
      double _xmag;
      int _xpos;
      TType _timeType;
      QFont _font1, _font2;

      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      virtual void mouseReleaseEvent(QMouseEvent*);

      Pos pix2pos(int x) const;
      int pos2pix(const Pos& p) const;

   public slots:
      void setXpos(int);

   public:
      Ruler(Score*, QWidget* parent = 0);
      void setXmag(double);
      };


#endif

