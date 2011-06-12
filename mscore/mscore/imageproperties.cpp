//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id: image.cpp -1   $
//
//  Copyright (C) 2007-2011 Werner Schweer and others
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

#include "imageproperties.h"
#include "xml.h"
#include "mscore.h"
#include "preferences.h"
#include "score.h"
#include "undo.h"
#include "libmscore/painter.h"

//---------------------------------------------------------
//   ImageProperties
//---------------------------------------------------------

ImageProperties::ImageProperties(Image* i, QWidget* parent)
   : QDialog(parent)
      {
      img = i;
      setupUi(this);
      lockAspectRatio->setChecked(img->lockAspectRatio());
      autoScale->setChecked(img->autoScale());
      }

