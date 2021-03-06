//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id$
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "layoutbreak.h"
#include "score.h"
#include "mscore.h"

//---------------------------------------------------------
//   propertyList
//---------------------------------------------------------

static qreal defaultPause = 0.0;

Property<LayoutBreak> LayoutBreak::propertyList[] = {
      { P_SUBTYPE, T_LAYOUT_BREAK, "subtype", &LayoutBreak::pSubtype,   0 },
      { P_PAUSE,   T_REAL,         "pause",   &LayoutBreak::pPause,   &defaultPause },
      };

static const int PROPERTIES = sizeof(LayoutBreak::propertyList)/sizeof(*LayoutBreak::propertyList);

//---------------------------------------------------------
//   LayoutBreak
//---------------------------------------------------------

LayoutBreak::LayoutBreak(Score* score)
   : Element(score)
      {
      _subtype             = LAYOUT_BREAK_PAGE;
      _pause               = score->styleD(ST_SectionPause);
      _startWithLongNames  = true;
      _startWithMeasureOne = true;
      lw                   = spatium() * 0.3;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void LayoutBreak::write(Xml& xml) const
      {
      xml.stag(name());
      Element::writeProperties(xml);
      for (int i = 0; i < PROPERTIES; ++i) {
            const Property<LayoutBreak>& p = propertyList[i];
            xml.tag(p.name, p.type, ((*(LayoutBreak*)this).*(p.data))(), propertyList[i].defaultVal);
            }
      if (!_startWithLongNames)
            xml.tag("startWithLongNames", _startWithLongNames);
      if (!_startWithMeasureOne)
            xml.tag("startWithMeasureOne", _startWithMeasureOne);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void LayoutBreak::read(const QDomElement& de)
      {
      for (QDomElement e = de.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            const QString& tag(e.tagName());
            const QString& val(e.text());
            if (setProperty(tag, e))
                  ;
            else if (tag == "startWithLongNames")
                  _startWithLongNames = val.toInt();
            else if (tag == "startWithMeasureOne")
                  _startWithMeasureOne = val.toInt();
            else if (!Element::readProperties(e))
                  domError(e);
            }
      layout0();
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void LayoutBreak::draw(QPainter* painter) const
      {
      if (score()->printing() || !score()->showUnprintable())
            return;
      painter->setPen(QPen(selected() ? MScore::selectColor[0] : MScore::layoutBreakColor,
         lw, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

      painter->setBrush(Qt::NoBrush);
      painter->drawPath(path);
      }

//---------------------------------------------------------
//   layout0
//---------------------------------------------------------

void LayoutBreak::layout0()
      {
      qreal _spatium = spatium();
      path      = QPainterPath();
      qreal h  = _spatium * 4;
      qreal w  = _spatium * 2.5;
      qreal w1 = w * .6;

      switch(subtype()) {
            case LAYOUT_BREAK_LINE:
                  path.lineTo(w, 0.0);
                  path.lineTo(w, h);
                  path.lineTo(0.0, h);
                  path.lineTo(0.0, 0.0);

                  path.moveTo(w * .8, w * .7);
                  path.lineTo(w * .8, w);
                  path.lineTo(w * .2, w);

                  path.moveTo(w * .4, w * .8);
                  path.lineTo(w * .2, w);
                  path.lineTo(w * .4, w * 1.2);
                  break;

            case LAYOUT_BREAK_PAGE:
                  path.lineTo(w, 0.0);
                  path.lineTo(w, h-w1);
                  path.lineTo(w1, h-w1);
                  path.lineTo(w1, h);
                  path.lineTo(0.0, h);
                  path.lineTo(0.0, 0.0);
                  path.moveTo(w, h-w1);
                  path.lineTo(w1, h);
                  break;

            case LAYOUT_BREAK_SECTION:
                  path.lineTo(w, 0.0);
                  path.lineTo(w,  h);
                  path.lineTo(0.0,  h);
                  path.moveTo(w-_spatium * .8,  0.0);
                  path.lineTo(w-_spatium * .8,  h);
                  break;

            default:
                  qDebug("unknown layout break symbol\n");
                  break;
            }
      QRectF bb(0, 0, w, h);
      bb.adjust(-lw, -lw, lw, lw);
      setbbox(bb);
      }

//---------------------------------------------------------
//   setSubtype
//---------------------------------------------------------

void LayoutBreak::setSubtype(LayoutBreakType val)
      {
      _subtype = val;
      layout0();
      }

//---------------------------------------------------------
//   spatiumChanged
//---------------------------------------------------------

void LayoutBreak::spatiumChanged(qreal, qreal)
      {
      lw = spatium() * 0.3;
      layout0();
      }

//---------------------------------------------------------
//   acceptDrop
//---------------------------------------------------------

bool LayoutBreak::acceptDrop(MuseScoreView*, const QPointF&, Element* e) const
      {
      return e->type() == LAYOUT_BREAK && static_cast<LayoutBreak*>(e)->subtype() != subtype();
      }

//---------------------------------------------------------
//   drop
//---------------------------------------------------------

Element* LayoutBreak::drop(const DropData& data)
      {
      Element* e = data.element;
      score()->undoChangeElement(this, e);
      return e;
      }

//---------------------------------------------------------
//   property
//---------------------------------------------------------

Property<LayoutBreak>* LayoutBreak::property(int id) const
      {
      for (int i = 0; i < PROPERTIES; ++i) {
            if (propertyList[i].id == id)
                  return &propertyList[i];
            }
      return 0;
      }

//---------------------------------------------------------
//   getProperty
//---------------------------------------------------------

QVariant LayoutBreak::getProperty(int propertyId) const
      {
      Property<LayoutBreak>* p = property(propertyId);
      if (p)
            return ::getProperty(p->type, ((*(LayoutBreak*)this).*(p->data))());
      return Element::getProperty(propertyId);
      }

//---------------------------------------------------------
//   setProperty
//---------------------------------------------------------

bool LayoutBreak::setProperty(int propertyId, const QVariant& v)
      {
      Property<LayoutBreak>* p = property(propertyId);
      if (p) {
            ::setProperty(p->type, ((*this).*(p->data))(), v);
            setGenerated(false);
            return true;
            }
      return Element::setProperty(propertyId, v);
      }

bool LayoutBreak::setProperty(const QString& name, const QDomElement& e)
      {
      for (int i = 0; i < PROPERTIES; ++i) {
            if (propertyList[i].name == name) {
                  QVariant v = ::getProperty(propertyList[i].type, e);
                  ::setProperty(propertyList[i].type, ((*this).*(propertyList[i].data))(), v);
                  setGenerated(false);
                  return true;
                  }
            }
      return Element::setProperty(name, e);
      }


