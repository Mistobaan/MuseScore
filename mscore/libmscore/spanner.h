//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id:$
//
//  Copyright (C) 2010-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __SPANNER_H__
#define __SPANNER_H__

#include "element.h"

class Segment;
class Spanner;
class System;

enum SpannerSegmentType {
      SEGMENT_SINGLE, SEGMENT_BEGIN, SEGMENT_MIDDLE, SEGMENT_END
      };

//---------------------------------------------------------
//   SpannerSegment
//---------------------------------------------------------

class SpannerSegment : public Element {

      virtual bool isEditable() const { return true; }

   protected:
      System* _system;

   public:
      SpannerSegment(Score* s);
      SpannerSegment(const SpannerSegment&);
      virtual SpannerSegment* clone() const = 0;
      Spanner* spanner() const                         { return (Spanner*)parent();  }
      void setSpannerSegmentType(SpannerSegmentType s) { setSubtype(s);              }
      SpannerSegmentType spannerSegmentType() const    { return SpannerSegmentType(subtype()); }
      void setSystem(System* s)                        { _system = s;                }
      virtual void startEdit(MuseScoreView*, const QPointF&);
      virtual void endEdit()                           { parent()->endEdit(); }
      };

//---------------------------------------------------------
//   Spanner
//---------------------------------------------------------

class Spanner : public Element {
      Element* _startElement;
      Element* _endElement;
      Anchor _anchor;         // enum Anchor { ANCHOR_SEGMENT, ANCHOR_MEASURE};

      QList<SpannerSegment*> segments;

      int _tick1, _tick2;     // used for backward compatibility
      int _id;                // used for xml serialization

   protected:
      Element* oStartElement; // start/end element at startEdit()
      Element* oEndElement;
      qreal _yoffset;        // in spatium units

   public:
      Spanner(Score*);
      Spanner(const Spanner&);
      ~Spanner();

      virtual ElementType type() const = 0;

      void setStartElement(Element* e) { _startElement = e;    }
      void setEndElement(Element* e)   { _endElement = e;      }
      Element* startElement() const    { return _startElement; }
      Element* endElement() const      { return _endElement;   }

      //
      // used for backward compatibility:
      //
      void __setTick1(int v)   { _tick1 = v;    }
      void __setTick2(int v)   { _tick2 = v;    }
      int __tick1() const      { return _tick1; }
      int __tick2() const      { return _tick2; }

      int id() const           { return _id; }
      void setId(int v)        { _id = v;    }

      Anchor anchor() const    { return _anchor;   }
      void setAnchor(Anchor a) { _anchor = a;      }

      const QList<SpannerSegment*>& spannerSegments() const { return segments; }
      QList<SpannerSegment*>& spannerSegments()             { return segments; }

      virtual void add(Element*);
      virtual void remove(Element*);
      virtual void scanElements(void* data, void (*func)(void*, Element*), bool all=true);
      virtual void startEdit(MuseScoreView*, const QPointF&);
      virtual void setSelected(bool f);
      void setYoff(qreal d) { _yoffset = d;        }
      qreal yoff() const    { return _yoffset;     }
      };
#endif
