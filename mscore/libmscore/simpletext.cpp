//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id:$
//
//  Copyright (C) 2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "simpletext.h"
#include "score.h"
#include "segment.h"
#include "measure.h"
#include "system.h"

//---------------------------------------------------------
//   TextSubtype
//---------------------------------------------------------

struct TextSubtype {
      unsigned id;
      const char* name;
      };

static const TextSubtype textSubtypes[] = {
      { TEXT_TITLE,               "Title"             },
      { TEXT_SUBTITLE,            "Subtitle"          },
      { TEXT_COMPOSER,            "Composer"          },
      { TEXT_POET,                "Poet"              },
      { TEXT_TRANSLATOR,          "Translator"        },
      { TEXT_MEASURE_NUMBER,      "MeasureNumber"     },
      { TEXT_FINGERING,           "Fingering"         },
      { TEXT_INSTRUMENT_LONG,     "InstrumentLong"    },
      { TEXT_INSTRUMENT_SHORT,    "InstrumentShort"   },
      { TEXT_INSTRUMENT_EXCERPT,  "InstrumentExcerpt" },
      { TEXT_TEMPO,               "Tempo"             },
      { TEXT_LYRIC,               "Lyric"             },
      { TEXT_FIGURED_BASS,        "FiguredBass"       },
      { TEXT_TUPLET,              "Tuplet"            },
      { TEXT_SYSTEM,              "System"            },
      { TEXT_STAFF,               "Staff"             },
      { TEXT_CHORD,               ""                  },
      { TEXT_REHEARSAL_MARK,      "RehearsalMark"     },
      { TEXT_REPEAT,              "Repeat"            },
      { TEXT_VOLTA,               "Volta"             },
      { TEXT_FRAME,               "Frame"             },
      { TEXT_TEXTLINE,            "TextLine"          },
      { TEXT_STRING_NUMBER,       "StringNumber"      },
      { TEXT_INSTRUMENT_CHANGE,   "InstrumentChange"  },
      { TEXT_LYRICS_VERSE_NUMBER, "LyricsVerseNumber" }
      };

//---------------------------------------------------------
//   SimpleText
//---------------------------------------------------------

SimpleText::SimpleText(Score* s)
   : Element(s)
      {
      _textStyle           = TEXT_STYLE_STAFF;
      _layoutToParentWidth = false;
      }

SimpleText::SimpleText(const SimpleText& st)
   : Element(st)
      {
      _text                = st._text;
      _textStyle           = st._textStyle;
      _layoutToParentWidth = st._layoutToParentWidth;
      frame                = st.frame;
      }

SimpleText::~SimpleText()
      {
      }

//---------------------------------------------------------
//   style
//---------------------------------------------------------

const TextStyle& SimpleText::style() const
      {
      return score()->textStyle(_textStyle);
      }

//---------------------------------------------------------
//   subtypeName
//---------------------------------------------------------

const QString SimpleText::subtypeName() const
      {
      for (unsigned i = 0; i < sizeof(textSubtypes)/sizeof(*textSubtypes); ++i) {
            if (textSubtypes[i].id == i)
                  return QString(textSubtypes[i].name);
            }
      qDebug("SimpleText:subtypeName: unknown text(%s) subtype %d", name(), subtype());
      return "?";
      }

//---------------------------------------------------------
//   setSubtype
//---------------------------------------------------------

void SimpleText::setSubtype(const QString& s)
      {
      unsigned st = TEXT_UNKNOWN;
      for (unsigned i = 0; i < sizeof(textSubtypes)/sizeof(*textSubtypes); ++i) {
            if (textSubtypes[i].name == s) {
                  st = textSubtypes[i].id;
                  break;
                  }
            }
      if (st == TEXT_UNKNOWN)
            qDebug("SimpleText(%s): setSubtype: unknown type <%s>", name(), qPrintable(s));
      setSubtype(st);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void SimpleText::draw(QPainter* p) const
      {
      p->setPen(curColor());
      p->setFont(style().fontPx(spatium()));
      p->drawText(drawingRect, alignFlags(), _text);
      drawFrame(p);
      }

//---------------------------------------------------------
//   drawFrame
//---------------------------------------------------------

void SimpleText::drawFrame(QPainter* painter) const
      {
      if (!style().hasFrame())
            return;

      QColor color(frameColor());
      if (!visible())
            color = Qt::gray;
      QPen pen(curColor(), frameWidth() * DPMM);
      painter->setPen(pen);
      painter->setBrush(Qt::NoBrush);
      if (circle())
            painter->drawArc(frame, 0, 5760);
      else {
            int r2 = frameRound() * lrint((frame.width() / frame.height()));
            if (r2 > 99)
                  r2 = 99;
            painter->drawRoundRect(frame, frameRound(), r2);
            }
      }

//---------------------------------------------------------
//   textColor
//---------------------------------------------------------

QColor SimpleText::textColor() const
      {
      QColor color;
      if (selected())
            return MScore::selectColor[0];
      if (!visible())
            return Qt::gray;
      return style().foregroundColor();
      }

//---------------------------------------------------------
//   alignFlags
//---------------------------------------------------------

int SimpleText::alignFlags() const
      {
      int flags = Qt::TextDontClip;
      Align align = style().align();
      if (align & ALIGN_HCENTER)
            flags |= Qt::AlignHCenter;
      else if (align & ALIGN_RIGHT)
            flags |= Qt::AlignRight;
      else
            flags |= Qt::AlignLeft;
      if (align & ALIGN_VCENTER)
            flags |= Qt::AlignVCenter;
      else if (align & ALIGN_BOTTOM)
            flags |= Qt::AlignBottom;
      else if (flags & ALIGN_BASELINE)
            ;
      else
            flags |= Qt::AlignTop;
      if (_layoutToParentWidth)
            flags |= Qt::TextWordWrap;
      return flags;
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void SimpleText::layout()
      {
      if (parent() == 0)
            return;

      const TextStyle& s(style());

      QPointF o(s.offset(spatium()));
      if (_layoutToParentWidth && parent()) {
            Element* e = parent();
            if ((type() == MARKER || type() == JUMP) && e->parent())
                  e = e->parent();
            qreal w = e->width();
            qreal h = e->height();
            drawingRect = QRectF(0.0, 0.0, w, h);
            QPointF ro = s.reloff() * .01;
            setPos(o + QPointF(ro.x() * w, ro.y() * h));
            }
      else {
            drawingRect = QRectF();
            setPos(o);
            }

      QFontMetricsF fm(s.fontPx(spatium()));
      setbbox(fm.boundingRect(drawingRect, alignFlags(), _text));

      if (hasFrame())
            layoutFrame();
      }

//---------------------------------------------------------
//   layoutFrame
//---------------------------------------------------------

void SimpleText::layoutFrame()
      {
      frame = bbox();
      if (circle()) {
            if (frame.width() > frame.height()) {
                  frame.setY(frame.y() + (frame.width() - frame.height()) * -.5);
                  frame.setHeight(frame.width());
                  }
            else {
                  frame.setX(frame.x() + (frame.height() - frame.width()) * -.5);
                  frame.setWidth(frame.height());
                  }
            }
      qreal w = (paddingWidth() + frameWidth() * .5) * DPMM;
      frame.adjust(-w, -w, w, w);
      w = frameWidth() * DPMM;
      setbbox(frame.adjusted(-w, -w, w, w));
      }

//---------------------------------------------------------
//   lineSpacing
//---------------------------------------------------------

qreal SimpleText::lineSpacing() const
      {
      return QFontMetricsF(style().font(spatium())).lineSpacing();
      }

//---------------------------------------------------------
//   lineHeight
//---------------------------------------------------------

qreal SimpleText::lineHeight() const
      {
      return QFontMetricsF(style().font(spatium())).height();
      }

//---------------------------------------------------------
//   baseLine
//---------------------------------------------------------

qreal SimpleText::baseLine() const
      {
      return QFontMetricsF(style().font(spatium())).ascent();
      }

//---------------------------------------------------------
//   frameWidth
//---------------------------------------------------------

qreal SimpleText::frameWidth() const
      {
      return style().frameWidth();
      }

//---------------------------------------------------------
//   hasFrame
//---------------------------------------------------------

bool SimpleText::hasFrame() const
      {
      return style().hasFrame();
      }

//---------------------------------------------------------
//   paddingWidth
//---------------------------------------------------------

qreal SimpleText::paddingWidth() const
      {
      return style().paddingWidth();
      }

//---------------------------------------------------------
//   frameColor
//---------------------------------------------------------

QColor SimpleText::frameColor() const
      {
      return style().frameColor();
      }

//---------------------------------------------------------
//   frameRound
//---------------------------------------------------------

int SimpleText::frameRound() const
      {
      return style().frameRound();
      }

//---------------------------------------------------------
//   circle
//---------------------------------------------------------

bool SimpleText::circle() const
      {
      return style().circle();
      }
