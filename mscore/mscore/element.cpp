//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: element.cpp,v 1.79 2006/04/12 14:58:10 wschweer Exp $
//
//  Copyright (C) 2002-2007 Werner Schweer and others
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

/**
 \file
 Implementation of Element, ElementList, StaffLines.
*/

#include "element.h"
#include "style.h"
#include "xml.h"
#include "score.h"
#include "preferences.h"
#include "staff.h"
#include "utils.h"
#include "sym.h"
#include "symbol.h"
#include "clef.h"
#include "layout.h"
#include "viewer.h"
#include "volta.h"
#include "ottava.h"
#include "trill.h"
#include "pedal.h"
#include "hairpin.h"
#include "keysig.h"
#include "timesig.h"
#include "barline.h"
#include "arpeggio.h"
#include "breath.h"
#include "bracket.h"
#include "chordrest.h"
#include "accidental.h"
#include "dynamics.h"
#include "text.h"
#include "note.h"
#include "tremolo.h"
#include "layoutbreak.h"
#include "repeat.h"

extern bool debugMode;
extern bool showInvisible;

// for debugging:
const char* elementNames[] = {
      "Symbol", "Text", "SlurSegment", "BarLine",
      "Stem", "Line", "SystemBracket",
      "Arpeggio",
      "Accidental", "Note",
      "Clef", "KeySig", "TimeSig", "Rest",
      "Breath",
      "RepeatMeasure",
      "Image",
      "Tie",
      "Attribute", "Dynamic", "Page", "Beam", "Hook", "Lyrics", "Repeat",
      "Tuplet", "VSpacer",
      "TempoText",
      "Volta",
      "HairpinSegment", "OttavaSegment", "PedalSegment", "TrillSegment",
      "VoltaSegment",
      "LayoutBreak",
      "LedgerLine",
      "Measure", "StaffLines",
      "Cursor", "Selection", "Lasso", "ShadowNote", "RubberBand",
      "NoteHead", "Tremolo",
      "HairPin", "Ottava", "Pedal", "Trill",
      "Segment", "System", "Compound", "Chord", "Slur",
      "Element", "ElementList", "StaffList", "MeasureList",
      "Layout"
      };

//---------------------------------------------------------
//   operator >
//---------------------------------------------------------

bool Element::operator>(const Element& el) const
      {
      if (tick() == el.tick())
            return type() > el.type();
      return tick() > el.tick();
      }

Element::~Element()
      {
      if (_selected) {
            if (score())
                  score()->deselect(this);
            if (debugMode)
                  printf("delete selected element\n");
            }
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void Element::init()
      {
      _prev       = 0;
      _next       = 0;
      _parent     = 0;
      _anchorMeasure = 0;
      _selected   = false;
      _dropTarget = false;
      _visible    = true;
      _generated  = false;
      _voice      = 0;
      _staff      = 0;
      _color      = Qt::black;
      _mxmlOff    = 0;
      _pos.setX(0.0);
      _pos.setY(0.0);
      _userOff.setX(0.0);
      _userOff.setY(0.0);
      itemDiscovered = 0;
      }

//---------------------------------------------------------
//   Element
//---------------------------------------------------------

Element::Element(Score* s)
      {
      _score = s;
      init();
      setSubtype(0);
      // Added by DK
      setRepeatFlag(0x00);
      //--------------------------
      }

//---------------------------------------------------------
//   staffIdx
//---------------------------------------------------------

int Element::staffIdx() const
      {
      return _score->staves().indexOf(_staff);
      }

//---------------------------------------------------------
//   setTrack
//---------------------------------------------------------

void Element::setTrack(int track)
      {
      if (track < 0)
            return;
      setVoice(track % VOICES);
      setStaff(score()->staff(track / VOICES));
      }

//---------------------------------------------------------
//   color
//---------------------------------------------------------

QColor Element::color() const
      {
      if (score() && score()->printing())
            return _color;
      if (_selected)
            return preferences.selectColor[_voice];
      if (_dropTarget)
            return preferences.dropColor;
      if (!_visible)
            return Qt::gray;
      return _color;
      }

//---------------------------------------------------------
//   drag
//---------------------------------------------------------

/**
 Return update Rect relative to canvas.
*/

QRectF Element::drag(const QPointF& s)
      {
      QRectF r(abbox());
      setUserOff(s / _spatium);
      return abbox() | r;
      }

//---------------------------------------------------------
//   canvasPos
//    return position in canvas coordinates
//---------------------------------------------------------

QPointF Element::canvasPos() const
      {
      QPointF p(pos());
      for (Element* e = _parent; e; e = e->parent())
            p += e->pos();
      return p;
      }

//---------------------------------------------------------
//   mapToCanvas
//    maps point to canvas coordinates
//---------------------------------------------------------

QPointF Element::mapToCanvas(const QPointF& pp) const
      {
      QPointF p(pp);
      for (Element* e = _parent; e; e = e->parent())
            p += e->pos();
      return p;
      }

//---------------------------------------------------------
//   mapToElement
//    maps point to canvas coordinates
//---------------------------------------------------------

QPointF Element::mapToElement(const Element* e, const QPointF& pp) const
      {
      QPointF p(pp);
      for (Element* ee = e->parent(); ee; ee = ee->parent())
            p += ee->pos();
      for (Element* e = _parent; e; e = e->parent())
            p -= e->pos();
      return p;
      }

//---------------------------------------------------------
//   contains
//---------------------------------------------------------

/**
 Return true if \a p is inside the shape of the object.

 Note: \a p is in canvas coordinates
*/

bool Element::contains(const QPointF& p) const
      {
      return shape().contains(p - canvasPos());
      }

//---------------------------------------------------------
//   shape
//---------------------------------------------------------

/**
  Returns the shape of this element as a QPainterPath in local
  coordinates. The shape is used for collision detection and
  hit tests (contains())

  The default implementation calls bbox() to return a simple rectangular
  shape, but subclasses can reimplement this function to return a more
  accurate shape for non-rectangular elements.
*/

QPainterPath Element::shape() const
      {
      QPainterPath pp;
      pp.addRect(bbox());
      return pp;
      }

//---------------------------------------------------------
//  intersects
//---------------------------------------------------------

/**
 Return true if \a rr intersects bounding box of object.

 Note: \a rr is relative to the coordinate system of parent().
*/

bool Element::intersects(const QRectF& rr) const
      {
      QRectF r(rr);
      r.translate(pos());
      return bbox().intersects(r);
      }

//---------------------------------------------------------
//   properties
//---------------------------------------------------------

QList<Prop> Element::properties(Xml& xml) const
      {
      QList<Prop> pl;
      if (_subtype)
            pl.append(Prop("subtype", subtypeName()));
      if (!_userOff.isNull())
            pl.append(Prop("offset", _userOff));
      if (voice())
            pl.append(Prop("voice", voice()));
      if (selected())
            pl.append(Prop("selected", selected()));
      if (!visible())
            pl.append(Prop("visible", visible()));
      if (_time.isValid() && (_time.tick() != xml.curTick))
            pl.append(Prop("tick", _time.tick()));
      if (_duration.isValid())
            pl.append(Prop("ticklen", _duration.tick()));
      if (_color != Qt::black)
            pl.append(Prop("color", _color));
      return pl;
      }

//---------------------------------------------------------
//   writeProperties
//---------------------------------------------------------

void Element::writeProperties(Xml& xml) const
      {
      xml.prop(properties(xml));
      if (_time.isValid() && (_time.tick() != xml.curTick || debugMode))
            xml.curTick = _time.tick();
      }

//---------------------------------------------------------
//   readProperties
//---------------------------------------------------------

bool Element::readProperties(QDomElement e)
      {
      QString tag(e.tagName());
      QString val(e.text());
      int i = val.toInt();

      if (tag == "tick") {
            setTick(score()->fileDivision(i));
            score()->curTick = _time.tick();
            }
      else if (tag == "subtype") {
            // do not always call Element::setSubtype():
            this->setSubtype(val);
            }
      else if (tag == "ticklen")
            setTickLen(score()->fileDivision(i));
      else if (tag == "offset")
            setUserOff(readPoint(e));
      else if (tag == "visible")
            setVisible(i);
      else if (tag == "voice")
            setVoice(i);
      else if (tag == "selected")
            setSelected(i);
      else if (tag == "color") {
            int r = e.attribute("r", "0").toInt();
            int g = e.attribute("g", "0").toInt();
            int b = e.attribute("b", "0").toInt();
            _color.setRgb(r, g, b);
            }
      else
            return false;
      return true;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Element::write(Xml& xml) const
      {
      xml.stag(name());
      Element::writeProperties(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Element::read(QDomElement e)
      {
      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            if (!Element::readProperties(e))
                  domError(e);
            }
      if (_subtype == 0)      // make sure setSubtype() is called at least once
            setSubtype(0);
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

/**
 Remove \a el from the list. Return true on success.
*/

bool ElementList::remove(Element* el)
      {
      int idx = indexOf(el);
      if (idx == -1)
            return false;
      removeAt(idx);
      return true;
      }

//---------------------------------------------------------
//   replace
//---------------------------------------------------------

void ElementList::replace(Element* o, Element* n)
      {
      int idx = indexOf(o);
      if (idx == -1) {
            printf("ElementList::replace: element not found\n");
            return;
            }
      QList<Element*>::replace(idx, n);
      }

//---------------------------------------------------------
//   move
//---------------------------------------------------------

void ElementList::move(Element* el, int tick)
      {
      int idx = indexOf(el);
      if (idx == -1) {
            printf("ElementList::move: element not found\n");
            return;
            }
      QList<Element*>::removeAt(idx);
      el->setTick(tick);
      add(el);
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void ElementList::add(Element* e)
      {
      int tick = e->tick();
      for (iElement ie = begin(); ie != end(); ++ie) {
            if ((*ie)->tick() > tick) {
                  insert(ie, e);
                  return;
                  }
            }
      append(e);
      }

//---------------------------------------------------------
//   StaffLines
//---------------------------------------------------------

StaffLines::StaffLines(Score* s)
   : Element(s)
      {
      setLines(5);
      _width = 1.0;      // dummy
      }

//---------------------------------------------------------
//   bbox
//---------------------------------------------------------

QRectF StaffLines::bbox() const
      {
      int l = lines() - 1;
      qreal lw = point(score()->style()->staffLineWidth);

      if (l == 0)
            return QRectF(0.0, - 2.0 * _spatium - lw*.5, _width, 4 * _spatium + lw);
      else
            return QRectF(0.0, -lw*.5, _width, l * _spatium + lw);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void StaffLines::draw(QPainter& p)
      {
      QPointF _pos(0.0, 0.0);

      p.save();
      p.setRenderHint(QPainter::Antialiasing, false);

      QPen pen(p.pen());
      pen.setWidthF(point(score()->style()->staffLineWidth));
      if (pen.widthF() * p.worldMatrix().m11() < 1.0)
            pen.setWidth(0);
      pen.setCapStyle(Qt::FlatCap);
      p.setPen(pen);

      qreal x1 = _pos.x();
      qreal x2 = x1 + width();

      if (lines() == 1) {
            qreal y = _pos.y() + 2 * _spatium;
            p.drawLine(QLineF(x1, y, x2, y));
            }
      else {
            for (int i = 0; i < lines(); ++i) {
                  qreal y = _pos.y() + i * _spatium;
                  p.drawLine(QLineF(x1, y, x2, y));
                  }
            }
      p.restore();
      }

//---------------------------------------------------------
//   Line
//---------------------------------------------------------

Line::Line(Score* s, bool v)
   : Element(s)
      {
      vertical = v;
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Line::dump() const
      {
      printf("  width:%g height:%g vert:%d\n",
         point(_width), point(_len), vertical);
      }

//---------------------------------------------------------
//   setLen
//---------------------------------------------------------

void Line::setLen(Spatium l)
      {
      _len = l;
      }

//---------------------------------------------------------
//   setLineWidth
//---------------------------------------------------------

void Line::setLineWidth(Spatium w)
      {
      _width = w;
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Line::layout(ScoreLayout* layout)
      {
      double spatium = layout->spatium();
      double w = _width.val() * spatium;
      double l = _len.val() * spatium;
      double w2 = w * .5;
      if (vertical)
            setbbox(QRectF(-w2, -w2, w, l + w));
      else
            setbbox(QRectF(-w2, -w2, l + w, w));
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Line::draw(QPainter& p)
      {
      QPen pen(p.pen());
      pen.setWidthF(point(_width));
      p.setPen(pen);
      if (vertical)
            p.drawLine(QLineF(0.0, 0.0, 0.0, point(_len)));
      else
            p.drawLine(QLineF(0.0, 0.0, point(_len), 0.0));
      }

//---------------------------------------------------------
//   writeProperties
//---------------------------------------------------------

void Line::writeProperties(Xml& xml) const
      {
      xml.tag("lineWidth", _width.val());
      xml.tag("lineLen", _len.val());
      if (!vertical)
            xml.tag("vertical", vertical);
      }

//---------------------------------------------------------
//   readProperties
//---------------------------------------------------------

bool Line::readProperties(QDomElement e)
      {
      QString tag(e.tagName());
      QString val(e.text());

      if (tag == "lineWidth")
            _width = Spatium(val.toDouble());
      else if (tag == "lineLen")
            _len = Spatium(val.toDouble());
      else if (tag == "vertical")
            vertical = val.toInt();
      else
            return false;
      return true;
      }

//---------------------------------------------------------
//   Compound
//---------------------------------------------------------

Compound::Compound(Score* s)
   : Element(s)
      {
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Compound::draw(QPainter& p)
      {
      foreach(Element* e, elemente) {
            QPointF pt(e->pos());
            p.translate(pt);
            e->draw(p);
            p.translate(-pt);
            }
      }

//---------------------------------------------------------
//   addElement
//---------------------------------------------------------

/**
 offset \a x and \a y are in Point units
*/

void Compound::addElement(Element* e, double x, double y)
      {
      e->setPos(x, y);
      e->setParent(this);
      elemente.push_back(e);
      }

//---------------------------------------------------------
//   bbox
//---------------------------------------------------------

QRectF Compound::bbox() const
      {
      _bbox = QRectF();
      for (ciElement i = elemente.begin(); i != elemente.end(); ++i) {
            const Element* e = *i;
            _bbox |= e->bbox().translated(e->pos());
            }
      return _bbox;
      }

//---------------------------------------------------------
//   setSelected
//---------------------------------------------------------

void Compound::setSelected(bool f)
      {
      Element::setSelected(f);
      for (ciElement i = elemente.begin(); i != elemente.end(); ++i)
            (*i)->setSelected(f);
      }

//---------------------------------------------------------
//   setVisible
//---------------------------------------------------------

void Compound::setVisible(bool f)
      {
      Element::setVisible(f);
      for (ciElement i = elemente.begin(); i != elemente.end(); ++i)
            (*i)->setVisible(f);
      }

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void Compound::clear()
      {
      for (ciElement i = elemente.begin(); i != elemente.end(); ++i)
            delete *i;
      elemente.clear();
      }

//---------------------------------------------------------
//   Cursor
//---------------------------------------------------------

Cursor::Cursor(Score* s, Viewer* v)
   : Element(s)
      {
      viewer    = v;
      _on       = false;
      _blink    = true;
      double w  = 2.0 / viewer->matrix().m11();
      setbbox(QRectF(0.0, 0.0, w, 6 * _spatium));
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Cursor::draw(QPainter& p)
      {
      if (!(_on && _blink))
            return;
      p.fillRect(abbox(), QBrush(preferences.selectColor[voice()]));
      }

//---------------------------------------------------------
//   Lasso
//---------------------------------------------------------

Lasso::Lasso(Score* s)
   : Element(s)
      {
      setVisible(false);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Lasso::draw(QPainter& p)
      {
      p.setBrush(Qt::NoBrush);
      QPen pen(QColor(preferences.selectColor[0]));
      // always 2 pixel width
      qreal w = 2.0 / p.matrix().m11();
      pen.setWidthF(w);
      p.setPen(pen);
      p.drawRect(bbox());
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Element::dump() const
      {
      printf("---Element: %s, pos(%4.2f,%4.2f)\n"
         "   bbox(%g,%g,%g,%g)\n"
         "   abox(%g,%g,%g,%g)\n"
         "  parent: %p\n",
         name(), _pos.x(), _pos.y(),
         _bbox.x(), _bbox.y(), _bbox.width(), _bbox.height(),
         abbox().x(), abbox().y(), abbox().width(), abbox().height(),
         parent());
      }

//---------------------------------------------------------
//   RubberBand::draw
//---------------------------------------------------------

void RubberBand::draw(QPainter& p)
      {
      if (!showRubberBand)
            return;
      p.setPen(Qt::red);
      p.drawLine(QLineF(_p1, _p2));
      }

//---------------------------------------------------------
//   VSpacer
//---------------------------------------------------------

VSpacer::VSpacer(Score* s, double h)
   : Element(s)
      {
      height = h;
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void VSpacer::draw(QPainter&)
      {
//      int lw       = lrint(.5 * tf->mag() * _spatium);
//      int len      = lrint(height * tf->mag() * _spatium);
//      QPoint _pos  = tf->fpos2ipoint(QPointF(0, 0));

//      p.setPen(QPen(QColor(Qt::blue), lw));
//TODO      p.drawLine(_pos.x(), _pos.y(), _pos.x(), _pos.y() + height);
      }

//---------------------------------------------------------
//   space
//---------------------------------------------------------

void Element::space(double& min, double& extra) const
      {
      min   = 0.0;
      extra = width();
      }

//---------------------------------------------------------
//   mimeData
//---------------------------------------------------------

QByteArray Element::mimeData(const QPointF& dragOffset) const
      {
      QBuffer buffer;
      buffer.open(QIODevice::WriteOnly);
      Xml xml(&buffer);
      xml.stag("Element");
      if (!dragOffset.isNull())
            xml.tag("dragOffset", dragOffset);
      write(xml);
      xml.etag();
      buffer.close();
      return buffer.buffer();
      }

//---------------------------------------------------------
//   readType
//---------------------------------------------------------

int Element::readType(QDomElement& e, QPointF* dragOffset)
      {
      int type = -1;

      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            //
            // DEBUG:
            // check names; remove non needed elements
            //
            if (e.tagName() == "dragOffset")
                  *dragOffset = readPoint(e);
            else if (e.tagName() == "Dynamic")
                  type = DYNAMIC;
            else if (e.tagName() == "Symbol")
                  type = SYMBOL;
            else if (e.tagName() == "Text")
                  type = TEXT;
            else if (e.tagName() == "StaffLines")
                  type = STAFF_LINES;
            else if (e.tagName() == "Slur")
                  type = SLUR_SEGMENT;
            else if (e.tagName() == "Note")
                  type = NOTE;
            else if (e.tagName() == "BarLine")
                  type = BAR_LINE;
            else if (e.tagName() == "Stem")
                  type = STEM;
            else if (e.tagName() == "Bracket")
                  type = BRACKET;
            else if (e.tagName() == "Accidental")
                  type = ACCIDENTAL;
            else if (e.tagName() == "Clef")
                  type = CLEF;
            else if (e.tagName() == "KeySig")
                  type = KEYSIG;
            else if (e.tagName() == "TimeSig")
                  type = TIMESIG;
            else if (e.tagName() == "Chord")
                  type = CHORD;
            else if (e.tagName() == "Rest")
                  type = REST;
            else if (e.tagName() == "Tie")
                  type = TIE;
            else if (e.tagName() == "Slur")
                  type = SLUR;
            else if (e.tagName() == "Measure")
                  type = MEASURE;
            else if (e.tagName() == "Attribute")
                  type = ATTRIBUTE;
            else if (e.tagName() == "Page")
                  type = PAGE;
            else if (e.tagName() == "Beam")
                  type = BEAM;
            else if (e.tagName() == "Hook")
                  type = HOOK;
            else if (e.tagName() == "Lyric")
                  type = LYRICS;
            else if (e.tagName() == "System")
                  type = SYSTEM;
            else if (e.tagName() == "HairPin")
                  type = HAIRPIN;
            else if (e.tagName() == "Tuplet")
                  type = TUPLET;
            else if (e.tagName() == "VSpacer")
                  type = VSPACER;
            else if (e.tagName() == "Segment")
                  type = SEGMENT;
            else if (e.tagName() == "TempoText")
                  type = TEMPO_TEXT;
            else if (e.tagName() == "Volta")
                  type = VOLTA;
            else if (e.tagName() == "Ottava")
                  type = OTTAVA;
            else if (e.tagName() == "Pedal")
                  type = PEDAL;
            else if (e.tagName() == "Trill")
                  type = TRILL;
            else if (e.tagName() == "LayoutBreak")
                  type = LAYOUT_BREAK;
            else if (e.tagName() == "LedgerLine")
                  type = LEDGER_LINE;
            else if (e.tagName() == "Image")
                  type = IMAGE;
            else if (e.tagName() == "Breath")
                  type = BREATH;
            else if (e.tagName() == "Arpeggio")
                  type = ARPEGGIO;
            else if (e.tagName() == "NoteHead")
                  type = NOTEHEAD;
            else if (e.tagName() == "Tremolo")
                  type = TREMOLO;
            else if (e.tagName() == "RepeatMeasure")
                  type = REPEAT_MEASURE;
            else if (e.tagName() == "Repeat")
                  type = REPEAT;
            else {
                  domError(e);
                  type = 0;
                  break;
                  }
            if (type >= 0)
                  break;
            }
      return type;
      }

//---------------------------------------------------------
//   create
//    Element factory
//---------------------------------------------------------

Element* Element::create(int type, Score* score)
      {
      Element* el = 0;
      switch(type) {
            case VOLTA:
                  el = new Volta(score);
                  break;
            case OTTAVA:
                  el = new Ottava(score);
                  break;
            case TRILL:
                  el = new Trill(score);
                  break;
            case PEDAL:
                  el = new Pedal(score);
                  break;
            case HAIRPIN:
                  el = new Hairpin(score);
                  break;
            case CLEF:
                  el = new Clef(score);
                  break;
            case KEYSIG:
                  el = new KeySig(score);
                  break;
            case TIMESIG:
                  el = new TimeSig(score);
                  break;
            case BAR_LINE:
                  el = new BarLine(score);
                  break;
            case ARPEGGIO:
                  el = new Arpeggio(score);
                  break;
            case BREATH:
                  el = new Breath(score);
                  break;
            case BRACKET:
                  el = new Bracket(score);
                  break;
            case ATTRIBUTE:
                  el = new NoteAttribute(score);
                  break;
            case ACCIDENTAL:
                  el = new Accidental(score);
                  break;
            case DYNAMIC:
                  el = new Dynamic(score);
                  break;
            case TEXT:
                  el = new Text(score);
                  break;
            case NOTEHEAD:
                  el = new NoteHead(score);
                  break;
            case TREMOLO:
                  el = new Tremolo(score);
                  break;
            case LAYOUT_BREAK:
                  el = new LayoutBreak(score);
                  break;
            case REPEAT:
                  el = new Repeat(score);
                  break;
            case REPEAT_MEASURE:
                  el = new RepeatMeasure(score);
                  break;
            default:
                  break;
            }
      return el;
      }

//---------------------------------------------------------
//   startEdit
//---------------------------------------------------------

bool Element::startEdit(const QPointF&)
      {
      return !_generated;
      }

//---------------------------------------------------------
//   editDrag
//---------------------------------------------------------

void Element::editDrag(int, const QPointF&, const QPointF& delta)
      {
      setUserOff(userOff() + delta / _spatium);
      }

//---------------------------------------------------------
//   edit
//    return true if event is accepted
//---------------------------------------------------------

bool Element::edit(int, QKeyEvent* ev)
      {
      if (ev->key() ==  Qt::Key_Home) {
            setUserOff(QPoint());
            return true;
            }
      return false;
      }

