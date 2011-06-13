//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id$
//
//  Copyright (C) 2010-2011 Werner Schweer and others
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

#include "stafftype.h"
#include "staff.h"
#include "xml.h"
#include "libmscore/painter.h"

QList<StaffType*> staffTypes;

//---------------------------------------------------------
//   initStaffTypes
//---------------------------------------------------------

void initStaffTypes()
      {
      StaffTypePitched* st = new StaffTypePitched("Pitched 5 lines");
      st->setLines(5);
      st->setLineDistance(Spatium(1.0));
      st->setGenClef(true);
      st->setGenKeysig(true);
      st->setSlashStyle(false);
      st->setShowBarlines(true);
      st->setShowLedgerLines(true);
      staffTypes.append(st);

      StaffTypeTablature* stab = new StaffTypeTablature("Tab");
      staffTypes.append(stab);

      StaffTypePercussion* sp = new StaffTypePercussion("Percussion 5 lines");
      sp->setLines(5);
      sp->setLineDistance(Spatium(1.0));
      sp->setGenClef(true);
      sp->setGenKeysig(false);
      sp->setSlashStyle(false);
      sp->setShowBarlines(true);
      sp->setShowLedgerLines(true);
      staffTypes.append(sp);
      }

//---------------------------------------------------------
//   StaffType
//---------------------------------------------------------

StaffType::StaffType()
      {
      _stepOffset      = 0;

      _genClef         = true;      // create clef at beginning of system
      _showBarlines    = true;
      _slashStyle      = false;     // do not show stems
      _genTimesig      = true;      // whether time signature is shown or not
      }

StaffType::StaffType(const QString& s)
      {
      _name            = s;
      _stepOffset      = 0;

      _genClef         = true;      // create clef at beginning of system
      _showBarlines    = true;
      _slashStyle      = false;     // do not show stems
      _genTimesig      = true;      // whether time signature is shown or not
      }

//---------------------------------------------------------
//   isEqual
//---------------------------------------------------------

bool StaffType::isEqual(const StaffType& st) const
      {
      return st.group() == group()
         && st._name         == _name
         && st._stepOffset   == _stepOffset
         && st._genClef      == _genClef
         && st._showBarlines == _showBarlines
         && st._slashStyle   == _slashStyle
         && st._genTimesig   == _genTimesig
         ;
      }

//---------------------------------------------------------
//   setLines
//---------------------------------------------------------

void StaffType::setLines(int val)
      {
      _lines = val;
      switch(_lines) {
            case 1:
                  _stepOffset = -4;
                  break;
            default:
                  _stepOffset = 0;
                  break;
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void StaffType::write(Xml& xml, int idx) const
      {
      xml.stag(QString("StaffType idx=\"%1\" group=\"%2\"").arg(idx).arg(groupName()));
      writeProperties(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   writeProperties
//---------------------------------------------------------

void StaffType::writeProperties(Xml& xml) const
      {
      xml.tag("name", name());
      if (lines() != 5)
            xml.tag("lines", lines());
      if (lineDistance().val() != 1.0)
            xml.tag("lineDistance", lineDistance().val());
      if (!genClef())
            xml.tag("clef", genClef());
      if (slashStyle())
            xml.tag("slashStyle", slashStyle());
      if (!showBarlines())
            xml.tag("barlines", showBarlines());
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void StaffType::read(QDomElement e)
      {
      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            if(!readProperties(e))
                  domError(e);
            }
      }

//---------------------------------------------------------
//   readProperties
//---------------------------------------------------------

bool StaffType::readProperties(QDomElement e)
      {
      QString tag(e.tagName());
      int v = e.text().toInt();
      if (tag == "name")
            setName(e.text());
      else if (tag == "lines")
            setLines(v);
      else if (tag == "lineDistance")
            setLineDistance(Spatium(e.text().toDouble()));
      else if (tag == "clef")
            setGenClef(v);
      else if (tag == "slashStyle")
            setSlashStyle(v);
      else if (tag == "barlines")
            setShowBarlines(v);
      else
            return false;
      return true;
      }

//---------------------------------------------------------
//   StaffTypePitched
//---------------------------------------------------------

StaffTypePitched::StaffTypePitched()
   : StaffType()
      {
      _genKeysig       = true;      // create key signature at beginning of system
      _showLedgerLines = true;
      }

//---------------------------------------------------------
//   isEqual
//---------------------------------------------------------

bool StaffTypePitched::isEqual(const StaffType& st) const
      {
      return StaffType::isEqual(st)
         && static_cast<const StaffTypePitched&>(st)._genKeysig       == _genKeysig
         && static_cast<const StaffTypePitched&>(st)._showLedgerLines == _showLedgerLines
         ;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void StaffTypePitched::write(Xml& xml, int idx) const
      {
      xml.stag(QString("StaffType idx=\"%1\" group=\"%2\"").arg(idx).arg(groupName()));
      StaffType::writeProperties(xml);

      if (!genKeysig())
            xml.tag("keysig", genKeysig());
      if (!showLedgerLines())
            xml.tag("ledgerlines", showLedgerLines());
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void StaffTypePitched::read(QDomElement e)
      {
      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            int v = e.text().toInt();
            if (tag == "keysig")
                  setGenKeysig(v);
            else if (tag == "ledgerlines")
                  setShowLedgerLines(v);
            else {
                  if (!StaffType::readProperties(e))
                        domError(e);
                  }
            }
      }

//---------------------------------------------------------
//   StaffTypePercussion
//---------------------------------------------------------

StaffTypePercussion::StaffTypePercussion()
   : StaffType()
      {
      _genKeysig       = true;      // create key signature at beginning of system
      _showLedgerLines = true;
      }

//---------------------------------------------------------
//   isEqual
//---------------------------------------------------------

bool StaffTypePercussion::isEqual(const StaffType& st) const
      {
      return StaffType::isEqual(st)
         && static_cast<const StaffTypePercussion&>(st)._genKeysig       == _genKeysig
         && static_cast<const StaffTypePercussion&>(st)._showLedgerLines == _showLedgerLines
         ;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void StaffTypePercussion::write(Xml& xml, int idx) const
      {
      xml.stag(QString("StaffType idx=\"%1\" group=\"%2\"").arg(idx).arg(groupName()));
      StaffType::writeProperties(xml);

      if (!genKeysig())
            xml.tag("keysig", genKeysig());
      if (!showLedgerLines())
            xml.tag("ledgerlines", showLedgerLines());
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void StaffTypePercussion::read(QDomElement e)
      {
      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            int v = e.text().toInt();
            if (tag == "keysig")
                  setGenKeysig(v);
            else if (tag == "ledgerlines")
                  setShowLedgerLines(v);
            else {
                  if (!StaffType::readProperties(e))
                        domError(e);
                  }
            }
      }

//---------------------------------------------------------
//   StaffTypeTablature
//---------------------------------------------------------

#define TAB_DEFAULT_DUR_YOFFS	(-1.75)
#define TAB_DEFAULT_LINE_SP	(1.5)

void StaffTypeTablature::init()
      {
      // set reasonable defaults for inherited members
      setLines(6);
      setLineDistance(Spatium(TAB_DEFAULT_LINE_SP));
      setGenClef(true);
//      setGenKeysig(false);
      setSlashStyle(false);
      setShowBarlines(true);
//      setShowLedgerLines(false);
      // for specific members
      setDurationFontName("MScoreTabulatureModern");
      setDurationFontSize(15.0);
      setDurationFontUserY(0.0);
      setFretFontName("MScoreTabulatureModern");
      setFretFontSize(10.0);
      setFretFontUserY(0.0);
      setGenDurations(false);
      setGenTimesig(false);
      setLinesThrough(false);
      setOnLines(true);
      setUpsideDown(false);
      setUseNumbers(true);
      // internal
      _durationMetricsValid = _fretMetricsValid = false;
      _durationBoxH = _durationBoxY = _durationYOffset = _fretBoxH = _fretBoxY = _fretYOffset = _refDPI = 0.0;
      }

//---------------------------------------------------------
//   isEqual
//---------------------------------------------------------

bool StaffTypeTablature::isEqual(const StaffType& st) const
      {
      return StaffType::isEqual(st)
         && static_cast<const StaffTypeTablature&>(st)._durationFontName   == _durationFontName
         && static_cast<const StaffTypeTablature&>(st)._durationFontSize   == _durationFontSize
         && static_cast<const StaffTypeTablature&>(st)._durationFontUserY  == _durationFontUserY
         && static_cast<const StaffTypeTablature&>(st)._fretFontName       == _fretFontName
         && static_cast<const StaffTypeTablature&>(st)._fretFontSize       == _fretFontSize
         && static_cast<const StaffTypeTablature&>(st)._fretFontUserY      == _fretFontUserY
         && static_cast<const StaffTypeTablature&>(st)._genDurations       == _genDurations
         && static_cast<const StaffTypeTablature&>(st)._linesThrough       == _linesThrough
         && static_cast<const StaffTypeTablature&>(st)._onLines            == _onLines
         && static_cast<const StaffTypeTablature&>(st)._upsideDown         == _upsideDown
         && static_cast<const StaffTypeTablature&>(st)._useNumbers         == _useNumbers
         ;
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void StaffTypeTablature::read(QDomElement e)
      {
      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            int v = e.text().toInt();
            double val = e.text().toDouble();
            if(tag == "durations")
                  setGenDurations(v != 0);
            else if(tag == "durationFontName")
                  setDurationFontName(e.text());
            else if(tag == "durationFontSize")
                  setDurationFontSize(val);
            else if(tag == "durationFontY")
                  setDurationFontUserY(val);
            else if(tag == "fretFontName")
                  setFretFontName(e.text());
            else if(tag == "fretFontSize")
                  setFretFontSize(val);
            else if(tag == "fretFontY")
                  setFretFontUserY(val);
            else if(tag == "linesThrough")
                  setLinesThrough(v != 0);
            else if(tag == "onLines")
                  setOnLines(v != 0);
            else if(tag == "timesig")
                  setGenTimesig(v != 0);
            else if(tag == "upsideDown")
                  setUpsideDown(v != 0);
            else if(tag == "useNumbers")
                  setUseNumbers(v != 0);
            else
                  if(!StaffType::readProperties(e))
                        domError(e);
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void StaffTypeTablature::write(Xml& xml, int idx) const
      {
      xml.stag(QString("StaffType idx=\"%1\" group=\"%2\"").arg(idx).arg(groupName()));
      StaffType::writeProperties(xml);
      xml.tag("durations",        _genDurations);
      xml.tag("durationFontName", _durationFontName);
      xml.tag("durationFontSize", _durationFontSize);
      xml.tag("durationFontY",    _durationFontUserY);
      xml.tag("fretFontName",     _fretFontName);
      xml.tag("fretFontSize",     _fretFontSize);
      xml.tag("fretFontY",        _fretFontUserY);
      xml.tag("linesThrough",     _linesThrough);
      xml.tag("onLines",          _onLines);
      xml.tag("timesig",          _genTimesig);
      xml.tag("upsideDown",       _upsideDown);
      xml.tag("useNumbers",       _useNumbers);
      xml.etag();
      }

//---------------------------------------------------------
//   setOnLines
//---------------------------------------------------------

void StaffTypeTablature::setOnLines(bool val)
      {
      _onLines = val;
      _durationMetricsValid = _fretMetricsValid = false;
      }

//---------------------------------------------------------
//   set metrics
//    checks whether the internally computed metrics are is still valid and re-computes them, if not
//---------------------------------------------------------

static QString    g_strNumbers("0123456789");
static QString    g_strLetters("abcdefghiklmnopq");
// used both to generate duration symbols and to compute duration metrics:
static QChar g_cDurationChars[] = { 0xE0FF, 0xE100, 0xE101, 0xE102, 0xE103, 0xE104,
//                                   Longa  Brevis   Whole   Half   Quarter  1/8
                                    0xE105, 0xE106, 0xE107, 0xE108, 0xE109, 0xE10B, ' ', ' '};
//                                   1\16    1\32    1\64    1\128   1\256   dot
#define STAFFTYPETAB_NUMOFDURCHARS  12    /* how many used chars there are in g_cDurationChar[] */
#define STAFFTYPETAB_IDXOFDOTCHAR   11    /* the offset of the dot char in g_cDurationChars[] */


void StaffTypeTablature::setDurationMetrics()
{
      if(_durationMetricsValid && _refDPI == DPI)           // metrics are still valid
            return;

      QFontMetricsF fm(durationFont());
      QRectF bb( fm.tightBoundingRect(QString(g_cDurationChars, STAFFTYPETAB_NUMOFDURCHARS)) );
      // move symbols so that the lowest margin 'sits' on the base line:
      // move down by the whole part above (negative) the base line
      // ( -bb.y() ) then up by the whole height ( -bb.height()/2 )
      _durationYOffset = -bb.y() - bb.height()
      // then move up by a default margin and, if marks are above lines, by half the line distance
      // (converted from spatium units to raster units)
            + ( TAB_DEFAULT_DUR_YOFFS - (_onLines ? 0.0 : lineDistance().val()/2.0) ) * DPI*SPATIUM20;
      _durationBoxH = bb.height();
      _durationBoxY = bb.y()  + _durationYOffset;
      // keep track of the conditions under which metrics have been computed
      _refDPI = DPI;
      _durationMetricsValid = true;
}

void StaffTypeTablature::setFretMetrics()
{
      if(_fretMetricsValid && _refDPI == DPI)
            return;

      QFontMetricsF fm(fretFont());
      // compute total height of used characters
      QRectF bb(fm.tightBoundingRect(_useNumbers ? g_strNumbers : g_strLetters));
      // compute vertical displacement
      if(_useNumbers) {
            // for numbers: centre on '0': move down by the whole part above (negative)
            // the base line ( -bb.y() ) then up by half the whole height ( -bb.height()/2 )
            QRectF bx( fm.tightBoundingRect("0") );
            _fretYOffset = -(bx.y() + bx.height()/2.0);
            // _fretYOffset = -(bb.y() + bb.height()/2.0);  // <- using bbox of all chars
            }
      else {
            // for letters: centre on the 'a' ascender, by moving down half of the part above the base line in bx
            QRectF bx( fm.tightBoundingRect("a") );
            _fretYOffset = -bx.y() / 2.0;
            }
      // if on string, we are done; if between strings, raise by half line distance
      if(!_onLines)
            _fretYOffset -= lineDistance().val()*DPI*SPATIUM20 / 2.0;

      // from _fretYOffset, compute _charBoxH and _charBoxY
      _fretBoxH = bb.height();
      _fretBoxY = bb.y()  + _fretYOffset;

      // keep track of the conditions under which metrics have been computed
      _refDPI = DPI;
      _fretMetricsValid = true;
}

//---------------------------------------------------------
//   TabDurationSymbol
//---------------------------------------------------------

TabDurationSymbol::TabDurationSymbol(Score* s)
   : Element(s)
      {
      setFlags(ELEMENT_MOVABLE | ELEMENT_SELECTABLE);
      setGenerated(true);
      _tab  = 0;
      _text = QString();
      }

TabDurationSymbol::TabDurationSymbol(Score* s, StaffTypeTablature * tab, Duration::DurationType type, int dots)
   : Element(s)
      {
      setFlags(ELEMENT_MOVABLE | ELEMENT_SELECTABLE);
      setGenerated(true);
      _tab  = tab;
      buildText(type, dots);
      }

TabDurationSymbol::TabDurationSymbol(const TabDurationSymbol& e)
   : Element(e)
      {
      _tab = e._tab;
      _text = e._text;
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------
/*
void TabDurationSymbol::layout()
      {
      QFontMetricsF fm(_tab->durationFont());
      double mags = magS();
      double w = fm.width(_text);
      _bbox = QRectF(0.0, _tab->durationBoxY() * mags, w * mags, _tab->durationBoxH() * mags);
      }
*/
//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void TabDurationSymbol::draw(Painter* painter) const
      {
      if(!_tab)
            return;
      double mag = magS();
      double imag = 1.0 / mag;

      painter->scale(mag);
      painter->setFont(_tab->durationFont());
      painter->drawText(0.0, _tab->durationFontYOffset(), _text);
      painter->scale(imag);
      }

//---------------------------------------------------------
//   buildText
//---------------------------------------------------------

void TabDurationSymbol::buildText(Duration::DurationType type, int dots)
      {
      // text string is a main symbol plus as many dots as required by chord duration
      _text = QString(g_cDurationChars[type]);
      for(int count=0; count < dots; count++)
            _text.append(g_cDurationChars[STAFFTYPETAB_IDXOFDOTCHAR]);
      }
