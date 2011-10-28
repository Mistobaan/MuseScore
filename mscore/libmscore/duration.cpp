//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id: duration.h 4426 2011-06-25 08:52:44Z wschweer $
//
//  Copyright (C) 2008-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "duration.h"
#include "tuplet.h"
#include "score.h"
#include "undo.h"

//---------------------------------------------------------
//   DurationElement
//---------------------------------------------------------

DurationElement::~DurationElement()
      {
      if (tuplet() && tuplet()->elements().front() == this)
            delete tuplet();
      }

//---------------------------------------------------------
//   properties
//---------------------------------------------------------

QList<Prop> DurationElement::properties(Xml& xml, bool /*clipboardmode*/) const
      {
      QList<Prop> pl = Element::properties(xml);
      if (tuplet())
            pl.append(Prop("Tuplet", tuplet()->id()));
      return pl;
      }

//---------------------------------------------------------
//   readProperties
//---------------------------------------------------------

bool DurationElement::readProperties(QDomElement e, QList<Tuplet*>* tuplets, const QList<Slur*>*)
      {
      if (Element::readProperties(e))
            return true;
      const QString& tag(e.tagName());
      if (tag == "Tuplet") {
            // setTuplet(0);
            int i = e.text().toInt();
            foreach(Tuplet* t, *tuplets) {
                  if (t->id() == i) {
                        setTuplet(t);
                        if (!score()->undo()->active())  // HACK, also added in Undo::AddElement()
                              t->add(this);
                        break;
                        }
                  }
            if (tuplet() == 0) {
                  qDebug("DurationElement:read(): Tuplet id %d not found", i);
                  Tuplet* t = score()->searchTuplet(e, i);
                  if (t) {
                        qDebug("   ...found outside measure, input file corrupted?");
                        setTuplet(t);
                        if (!score()->undo()->active())     // HACK
                              t->add(this);
                        tuplets->append(t);
                        }
                  }
            }
      else
            return false;
      return true;
      }

//---------------------------------------------------------
//   writeTuplet
//---------------------------------------------------------

void DurationElement::writeTuplet(Xml& xml)
      {
      if (tuplet() && tuplet()->elements().front() == this) {
            tuplet()->writeTuplet(xml);           // recursion
            tuplet()->setId(xml.tupletId++);
            tuplet()->write(xml);
            }
      }

