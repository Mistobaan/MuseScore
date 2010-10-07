//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id: style.h 3555 2010-10-06 11:15:52Z wschweer $
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

#ifndef __STYLE_P_H__
#define __STYLE_P_H__

// private header for Style

class Xml;
class ChordDescription;
class ChordList;

//---------------------------------------------------------
//   TextStyleData
//---------------------------------------------------------

class TextStyleData : public QSharedData {
   protected:
      QString name;
      QString family;
      int size;
      bool bold;
      bool italic;
      bool underline;
      bool hasFrame;
      Align align;
      double xoff, yoff;                  // absolute offset: inch or spatium
      OffsetType offsetType;
      double rxoff, ryoff;                // relative offset: % of parent width/height
      bool sizeIsSpatiumDependent;        // text point size depends on _spatium unit

      double frameWidth;
      double paddingWidth;
      int frameRound;
      QColor frameColor;
      bool circle;
      bool systemFlag;
      QColor foregroundColor;

   public:
      TextStyleData(QString _name, QString _family, int _size,
         bool _bold, bool _italic, bool _underline,
         Align _align,
         double _xoff, double _yoff, OffsetType _ot,
         double _rxoff, double _ryoff,
         bool sd,
         double fw, double pw, int fr,
         QColor co, bool circle, bool systemFlag,
         QColor fg);
      TextStyleData() {}

      void write(Xml&) const;
      void read(QDomElement);

      QFont font(double space) const;
      QFont fontPx(double spatium) const;
      QRectF bbox(double space, const QString& s) const { return fontMetrics(space).boundingRect(s); }
      QFontMetricsF fontMetrics(double space) const     { return QFontMetricsF(font(space)); }
      bool operator!=(const TextStyleData& s) const;
      friend class TextStyle;
      };

//---------------------------------------------------------
//   StyleData
//    this structure contains all style elements
//---------------------------------------------------------

class StyleData : public QSharedData {
   protected:
      QVector<StyleVal> _values;
      mutable ChordList* _chordList;
      QList<TextStyle> _textStyles;

      void set(const StyleVal& v)                         { _values[v.getIdx()] = v; }
      StyleVal value(StyleIdx idx) const                  { return _values[idx];     }
      const TextStyle& textStyle(TextStyleType idx) const { return _textStyles[idx]; }
      const TextStyle& textStyle(const QString&) const;
      TextStyleType textStyleType(const QString&) const;
      void setTextStyle(const TextStyle& ts);

   public:
      StyleData();
      StyleData(const StyleData&);
      ~StyleData();

      bool load(QFile* qf);
      void load(QDomElement e);
      void save(Xml& xml, bool optimize) const;
      bool isDefault(StyleIdx) const;

      const ChordDescription* chordDescription(int id) const;
      ChordList* chordList() const;
      friend class Style;
      };

#endif
