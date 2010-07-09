//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id$
//
//  Copyright (C) 2002-2009 Werner Schweer and others
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

#ifndef __KEY_H__
#define __KEY_H__

class Xml;
class Score;

//---------------------------------------------------------
//   KeySigEvent
//---------------------------------------------------------

struct KeySigEvent {
      union {
            int subtype;
            struct {
                  int _accidentalType:4;
                  int _naturalType:4;
                  unsigned _customType:16;
                  bool _custom : 1;
                  bool _invalid : 1;
                  };
            struct {
                  int _nothingR:6;
                  bool _invalidR : 1;
                  bool _customR : 1;
                  unsigned _customTypeR:16;
                  int _naturalTypeR:4;
                  int _accidentalTypeR:4;
                  };
            };
      KeySigEvent();
      KeySigEvent(int v);

      bool isValid() const;
      bool operator==(const KeySigEvent& e) const;
      bool operator!=(const KeySigEvent& e) const;
      void setCustomType(int v);
      void setAccidentalType(int v);
      
      void setNaturalType(int v);
      void setCustom(bool v);
      
      int accidentalType() const;
      int naturalType() const;
      unsigned customType() const;
      bool custom() const;
      bool invalid() const;
      
      void print() const;
      };

//---------------------------------------------------------
//   KeyList
//    this list is instantiated for every staff
//    to keep track of key signature changes
//---------------------------------------------------------

typedef std::map<const int, KeySigEvent>::iterator iKeyList;
typedef std::map<const int, KeySigEvent>::const_iterator ciKeyList;

class KeyList : public std::map<const int, KeySigEvent> {
   public:
      KeyList() {}
      KeySigEvent key(int tick) const;
      void read(QDomElement, Score*);
      void write(Xml&, const char* name) const;

      void removeTime(int start, int len);
      void insertTime(int start, int len);
      };

extern int transposeKey(int oldKey, int semitones);

#endif

