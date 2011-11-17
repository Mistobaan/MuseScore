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

#include "measurebase.h"
#include "measure.h"
#include "staff.h"
#include "lyrics.h"
#include "score.h"
#include "chord.h"
#include "note.h"
#include "layoutbreak.h"
#include "image.h"
#include "segment.h"
#include "tempo.h"

//---------------------------------------------------------
//   MeasureBase
//---------------------------------------------------------

MeasureBase::MeasureBase(Score* score)
   : Element(score)
      {
      _prev = 0;
      _next = 0;
      _lineBreak    = false;
      _pageBreak    = false;
      _sectionBreak = 0;
      _dirty        = true;
      }

MeasureBase::MeasureBase(const MeasureBase& m)
   : Element(m)
      {
      _next         = m._next;
      _prev         = m._prev;
      _tick         = m._tick;
      _mw           = m._mw;
      _dirty        = m._dirty;
      _lineBreak    = m._lineBreak;
      _pageBreak    = m._pageBreak;
      _sectionBreak = m._sectionBreak ? new LayoutBreak(*m._sectionBreak) : 0;

      foreach(Element* e, m._el)
            add(e->clone());
      }

//---------------------------------------------------------
//   setScore
//---------------------------------------------------------

void MeasureBase::setScore(Score* score)
      {
      Element::setScore(score);
      if (_sectionBreak)
            _sectionBreak->setScore(score);
      foreach(Element* e, _el)
            e->setScore(score);
      }

//---------------------------------------------------------
//   MeasureBase
//---------------------------------------------------------

MeasureBase::~MeasureBase()
      {
      foreach(Element* e, _el)
            delete e;
      }

//---------------------------------------------------------
//   scanElements
//---------------------------------------------------------

void MeasureBase::scanElements(void* data, void (*func)(void*, Element*), bool all)
      {
      if (type() == MEASURE) {
            foreach(Element* e, _el) {
                  if ((e->track() == -1) || ((Measure*)this)->visible(e->staffIdx()))
                        e->scanElements(data, func, all);
                  }
            }
      else {
            foreach(Element* e, _el) {
                  e->scanElements(data, func, all);
                  }
            }
//      if (all)
            func(data, this);
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

/**
 Add new Element \a el to MeasureBase
*/

void MeasureBase::add(Element* e)
      {
      e->setParent(this);
      if (e->type() == LAYOUT_BREAK) {
            LayoutBreak* b = static_cast<LayoutBreak*>(e);
            foreach (Element* ee, _el) {
                  if (ee->type() == LAYOUT_BREAK && ee->subtype() == e->subtype()) {
                        if (debugMode)
                              qDebug("warning: layout break already set\n");
                        return;
                        }
                  }
            switch(e->subtype()) {
                  case LAYOUT_BREAK_PAGE:
                        _pageBreak = true;
                        break;
                  case LAYOUT_BREAK_LINE:
                        _lineBreak = true;
                        break;
                  case LAYOUT_BREAK_SECTION:
                        _sectionBreak = b;
                        score()->tempomap()->setPause(endTick(), b->pause());
                        break;
                  }
            }
      _el.append(e);
      if (e->type() == IMAGE)
            static_cast<Image*>(e)->reference();
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

/**
 Remove Element \a el from MeasureBase.
*/

void MeasureBase::remove(Element* el)
      {
      if (el->type() == LAYOUT_BREAK) {
            switch(el->subtype()) {
                  case LAYOUT_BREAK_PAGE:
                        _pageBreak = false;
                        break;
                  case LAYOUT_BREAK_LINE:
                        _lineBreak = false;
                        break;
                  case LAYOUT_BREAK_SECTION:
                        _sectionBreak = 0;
                        score()->tempomap()->setPause(endTick(), 0);
                        break;
                  }
            }
      if (!_el.remove(el))
            qDebug("MeasureBase(%p)::remove(%s,%p) not found\n", this, el->name(), el);
      if (el->type() == IMAGE)
            static_cast<Image*>(el)->dereference();
      }


//---------------------------------------------------------
//   nextMeasure
//---------------------------------------------------------

Measure* MeasureBase::nextMeasure() const
      {
      MeasureBase* m = next();
      while (m) {
            if (m->type() == MEASURE)
                  return static_cast<Measure*>(m);
            m = m->next();
            }
      return 0;
      }

//---------------------------------------------------------
//   prevMeasure
//---------------------------------------------------------

Measure* MeasureBase::prevMeasure() const
      {
      MeasureBase* m = prev();
      while (m) {
            if (m->type() == MEASURE)
                  return static_cast<Measure*>(m);
            m = m->prev();
            }
      return 0;
      }

//---------------------------------------------------------
//   pause
//---------------------------------------------------------

qreal MeasureBase::pause() const
      {
      return _sectionBreak ? _sectionBreak->pause() : 0.0;
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void MeasureBase::layout()
      {
      int breakCount = 0;
      foreach (Element* element, _el) {
            if (element->type() == LAYOUT_BREAK) {
                  qreal _spatium = spatium();
                  qreal x = -_spatium - element->width() + width()
                            - breakCount * (element->width() + _spatium * .8);
                  qreal y = -2 * _spatium - element->height();
                  element->setPos(x, y);
                  breakCount++;
                  }
            }
      }

//---------------------------------------------------------
//   first
//---------------------------------------------------------

MeasureBase* Score::first() const
      {
      return _measures.first();
      }

//---------------------------------------------------------
//   last
//---------------------------------------------------------

MeasureBase* Score::last()  const
      {
      return _measures.last();
      }


