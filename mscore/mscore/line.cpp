//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: line.cpp,v 1.4 2006/03/13 21:35:59 wschweer Exp $
//
//  Copyright (C) 2002-2006 Werner Schweer (ws@seh.de)
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

#include "line.h"
#include "segment.h"
#include "measure.h"
#include "score.h"
#include "xml.h"
#include "layout.h"
#include "viewer.h"

//---------------------------------------------------------
//   LineSegment
//---------------------------------------------------------

LineSegment::LineSegment(Score* s)
   : Element(s)
      {
      _segmentType = SEGMENT_SINGLE;
      mode  = NORMAL;
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void LineSegment::draw(QPainter& p)
      {
      if (mode != NORMAL) {
            qreal lw = 2.0/p.matrix().m11();
            QPen pen(Qt::blue);
            pen.setWidthF(lw);
            p.setPen(pen);
            if (mode == DRAG1) {
                  p.setBrush(Qt::blue);
                  p.drawRect(r1);
                  p.setBrush(Qt::NoBrush);
                  p.drawRect(r2);
                  }
            else {
                  p.setBrush(Qt::NoBrush);
                  p.drawRect(r1);
                  p.setBrush(Qt::blue);
                  p.drawRect(r2);
                  }
            }
      }

//---------------------------------------------------------
//   startEdit
//---------------------------------------------------------

bool LineSegment::startEdit(QMatrix& matrix, const QPointF&)
      {
      mode = DRAG2;
      QPointF pp2(_p2 + _userOff2 * _spatium);

      qreal w   = 8.0 / matrix.m11();
      qreal h   = 8.0 / matrix.m22();
      QRectF r(-w/2, -h/2, w, h);
      qreal lw  = 1.0 / matrix.m11();
      QRectF br = r.adjusted(-lw, -lw, lw, lw);
      r1        = r;
      bbr1      = br;
      r2        = r.translated(pp2);
      bbr2      = br.translated(pp2);
      return true;
      }

//---------------------------------------------------------
//   startEditDrag
//---------------------------------------------------------

bool LineSegment::startEditDrag(Viewer* viewer, const QPointF& p)
      {
      int tick1 = line()->tick();
      int tick2 = line()->tick2();

      if (bbr1.contains(p)) {
            mode = DRAG1;
            QPointF anchor1 = score()->tick2Anchor(tick1, staffIdx());
            QLineF l(anchor1, canvasPos());
            viewer->setDropAnchor(l);
            }
      else if (bbr2.contains(p)) {
            mode = DRAG2;
            QPointF anchor2 = score()->tick2Anchor(tick2, staffIdx());
            QLineF l(anchor2, canvasPos2());
            viewer->setDropAnchor(l);
            }
      else {
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   editDrag
//---------------------------------------------------------

bool LineSegment::editDrag(Viewer* viewer, QPointF* start, const QPointF& d)
      {
      QPointF aapos(*start + d);
      QPointF delta(d.x(), 0);    // only x-axis move
      int tick;
      QPointF anchor;
      score()->pos2TickAnchor(aapos, staff(), &tick, &anchor);

      if (mode == DRAG1) {
            r2.translate(-delta);
            bbr2.translate(-delta);

            QPointF apos(canvasPos() + delta);
            viewer->setDropAnchor(QLineF(anchor, apos));

            if (_segmentType == SEGMENT_SINGLE || _segmentType == SEGMENT_BEGIN)
                  line()->setTick(tick);

            QPointF apos2(canvasPos2());
            setUserXoffset((apos.x() - anchor.x()) / _spatium);
            setXpos(anchor.x() - parent()->canvasPos().x());

            score()->pos2TickAnchor(apos2, staff(), &tick, &anchor);
            _p2.setX(anchor.x() - canvasPos().x());
            _userOff2.setX((apos2.x() - anchor.x()) / _spatium);
            }
      else {
            r2.translate(delta);
            bbr2.translate(delta);

            QPointF apos(canvasPos2() + delta);
            viewer->setDropAnchor(QLineF(anchor, apos));;

            if (_segmentType == SEGMENT_SINGLE || _segmentType == SEGMENT_END)
                  line()->setTick2(tick);

            setUserXoffset2((apos.x() - anchor.x()) / _spatium);
            setXpos2(anchor.x() - canvasPos().x());
            }
      return true;
      }

//---------------------------------------------------------
//   edit
//---------------------------------------------------------

bool LineSegment::edit(QKeyEvent* ev)
      {
      QPointF delta;
      switch (ev->key()) {
            case Qt::Key_Up:
                  _userOff.ry() += -.3;
                  break;
            case Qt::Key_Down:
                  _userOff.ry() += .3;
                  break;
            case Qt::Key_Left:
                  delta = QPointF(-1, 0);
                  break;
            case Qt::Key_Right:
                  delta = QPointF(1, 0);
                  break;
            case Qt::Key_Tab:
                  if (mode == DRAG1)
                        mode = DRAG2;
                  else if (mode == DRAG2)
                        mode = DRAG1;
                  break;
            }
      if (mode == DRAG1) {
            setUserOff(userOff() + delta);
            r1.moveTopLeft(r1.topLeft() + delta * _spatium);
            bbr1.moveTopLeft(bbr1.topLeft() + delta * _spatium);
            }
      else if (mode == DRAG2) {
            setUserOff2(userOff() + delta);
            r2.moveTopLeft(r2.topLeft() + delta * _spatium);
            bbr2.moveTopLeft(bbr2.topLeft() + delta * _spatium);
            }
      return false;
      }

//---------------------------------------------------------
//   endEditDrag
//---------------------------------------------------------

bool LineSegment::endEditDrag()
      {
      // TODO: must move to different Measure?
      return false;
      }

//---------------------------------------------------------
//   endEdit
//---------------------------------------------------------

void LineSegment::endEdit()
      {
      mode = NORMAL;
      }

//---------------------------------------------------------
//   drag
//---------------------------------------------------------

QRectF LineSegment::drag(const QPointF& s)
      {
      QRectF r(abbox());
      QPointF newOffset(s / _spatium);
      QPointF diff(userOff() - newOffset);
      setUserOff(newOffset);
      setUserXoffset2(userOff2().x() - diff.x());
      return abbox() | r;
      }

//---------------------------------------------------------
//   endDrag
//---------------------------------------------------------

void LineSegment::endDrag()
      {
      }

//---------------------------------------------------------
//   SLine
//---------------------------------------------------------

SLine::SLine(Score* s)
   : Element(s)
      {
      setTick(0);
      _tick2 = 0;
      }

//---------------------------------------------------------
//   setTick2
//---------------------------------------------------------

void SLine::setTick2(int t)
      {
      _tick2 = t;
      }

//---------------------------------------------------------
//   layout
//    compute segments from tick1 tick2
//---------------------------------------------------------

void SLine::layout(ScoreLayout* layout)
      {
      if (!parent()) {
            //
            // when used in a palette, SLine has no parent and
            // tick and tick2 has no meaning so no layout is
            // possible and needed
            //
            if (!segments.isEmpty())
                  setbbox(segments.front()->bbox());
            return;
            }

      Segment* seg1 = _score->tick2segment(tick());
      Segment* seg2 = _score->tick2segment(_tick2);
      if (seg1 == 0 || seg2 == 0) {
            printf("SLine Layout: seg not found\n");
            return;
            }
      System* system1 = seg1->measure()->system();
      System* system2 = seg2->measure()->system();

      QPointF ppos(parent()->canvasPos());
      QPointF p1 = seg1->canvasPos() - ppos; //  - ipos();

      iSystem is = layout->systems()->begin();
      while (is != layout->systems()->end()) {
            if (*is == system1)
                  break;
            ++is;
            }
      int segmentsNeeded = 1;
      for (iSystem iis = is; iis != layout->systems()->end(); ++iis, ++segmentsNeeded) {
            if (*iis == system2)
                  break;
            }
      if (segmentsNeeded != segments.size()) {
printf("segments needed %d  but there are %d\n", segmentsNeeded, segments.size());
            // if line break changes do a complete re-layout;
            // this especially removes all user editing

            // TODO: selected segments?
            foreach(LineSegment* seg, segments)
                  delete seg;
            segments.clear();
            }

      int seg = 0;
      for (; is != layout->systems()->end(); ++is, ++seg) {
            if (seg >= segments.size()) {
                  printf("create new line segment\n");
                  segments.append(createSegment());
                  }
            LineSegment* hps = segments[seg];
            if (seg == 0) {
                  hps->setPos(p1);
                  }
            else {
                  hps->setPos((*is)->canvasPos() - parent()->canvasPos());
                  }
            if (*is == system2) {
                  QPointF p2 = seg2->canvasPos() - hps->canvasPos();
                  hps->setXpos2(p2.x());
                  break;
                  }
            hps->setXpos2((*is)->canvasPos().x()
               + (*is)->bbox().width()
               - _spatium * 0.5
               - hps->canvasPos().x()
               );
            }

      int idx = 0;
      int n = segments.size();
      foreach(LineSegment* seg, segments) {
            ++idx;
            if (n == 1)
                  seg->setSegmentType(LineSegment::SEGMENT_SINGLE);
            else if (idx == 1)
                  seg->setSegmentType(LineSegment::SEGMENT_BEGIN);
            else if (idx == n)
                  seg->setSegmentType(LineSegment::SEGMENT_END);
            else
                  seg->setSegmentType(LineSegment::SEGMENT_MIDDLE);
            seg->layout(layout);
            }
      }

//---------------------------------------------------------
//   writeProperties
//---------------------------------------------------------

void SLine::writeProperties(Xml& xml) const
      {
      Element::writeProperties(xml);
      xml.tag("tick2", _tick2);
      //
      // check if user has modified the default layout
      //
      bool modified = false;
      foreach(LineSegment* seg, segments) {
            if (!seg->userOff().isNull() || !seg->userOff2().isNull()) {
                  modified = true;
                  break;
                  }
            }
      if (!modified)
            return;

      //
      // write user modified layout
      //
      foreach(LineSegment* seg, segments) {
            xml.stag("Segment");
            xml.tag("off1", seg->userOff());
            xml.tag("off2", seg->userOff2());
            xml.etag("Segment");
            }
      }

//---------------------------------------------------------
//   readProperties
//---------------------------------------------------------

bool SLine::readProperties(QDomNode node)
      {
      if (Element::readProperties(node))
            return true;
      QDomElement e = node.toElement();
      if (e.isNull())
            return true;
      QString tag(e.tagName());
      QString val(e.text());
      int i = val.toInt();
      if (tag == "tick2")
            _tick2 = score()->fileDivision(i);
      else if (tag == "Segment") {
            LineSegment* ls = createSegment();
            for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
                  e = n.toElement();
                  if (e.isNull())
                        continue;
                  if (e.tagName() == "off1")
                        ls->setUserOff(readPoint(n));
                  else if (e.tagName() == "off2")
                        ls->setUserOff2(readPoint(n));
                  else
                        domError(e);
                  }
            segments.append(ls);
            }
      else
            return false;
      return true;
      }

//---------------------------------------------------------
//   setLen
//    used to create an element suitable for palette
//---------------------------------------------------------

void SLine::setLen(double l)
      {
      if (segments.isEmpty())
            segments.append(createSegment());
      LineSegment* s = segments.front();
      s->setPos(QPointF());
      s->setPos2(QPointF(l, 0));
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void SLine::draw(QPainter& p)
      {
      foreach(LineSegment* seg, segments) {
            p.save();
            p.translate(seg->pos());
            seg->draw(p);
            p.restore();
            }
      }

//---------------------------------------------------------
//   collectElements
//---------------------------------------------------------

void SLine::collectElements(QList<Element*>& el)
      {
      foreach(LineSegment* seg, segments)
            el.append(seg);
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void SLine::add(Element* e)
      {
      segments.append((LineSegment*) e);
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void SLine::remove(Element*)
      {
      printf("SLine::remove=========\n");
      segments.clear();
      }

//---------------------------------------------------------
//   change
//---------------------------------------------------------

void SLine::change(Element* os, Element* ns)
      {
      int n = segments.size();
      for (int i = 0; i < n; ++i) {
            if (segments[i] == os) {
                  segments[i] = (LineSegment*)ns;
                  return;
                  }
            }
      printf("SLine::change: segment not found\n");
      }

//---------------------------------------------------------
//   bbox
//    used by palette: only one segment
//---------------------------------------------------------

QRectF SLine::bbox() const
      {
      if (segments.isEmpty())
            return QRectF();
      else
            return segments[0]->bbox();
      }


