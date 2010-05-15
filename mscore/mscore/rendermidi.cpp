//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id$
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

/**
 \file
 render score into event list
*/

#include "mscore.h"
#include "score.h"
#include "volta.h"
#include "note.h"
#include "instrument.h"
#include "part.h"
#include "chord.h"
#include "style.h"
#include "ottava.h"
#include "slur.h"
#include "stafftext.h"
#include "repeat.h"
#include "articulation.h"
#include "arpeggio.h"
#include "durationtype.h"
#include "measure.h"
#include "al/tempo.h"
#include "al/sig.h"
#include "repeatlist.h"
#include "velo.h"
#include "dynamics.h"
#include "navigate.h"
#include "pedal.h"
#include "staff.h"

//---------------------------------------------------------
//   updateChannel
//---------------------------------------------------------

void Score::updateChannel()
      {
      foreach(Staff* s, _staves) {
            for (int i = 0; i < VOICES; ++i)
                  s->channelList(i)->clear();
            }
      for (Measure* m = firstMeasure(); m; m = m->nextMeasure()) {
            foreach(const Element* e, *m->el()) {
                  if (e->type() != STAFF_TEXT)
                        continue;
                  const StaffText* st = static_cast<const StaffText*>(e);
                  for (int voice = 0; voice < VOICES; ++voice) {
                        QString an(st->channelName(voice));
                        if (an.isEmpty())
                              continue;
                        Staff* staff = _staves[st->staffIdx()];
                        int a = staff->part()->channelIdx(an);
                        if (a != -1)
                              staff->channelList(voice)->insert(st->tick(), a);
                        }
                  }
            }

      for (Segment* s = firstMeasure()->first(SegChordRest | SegGrace); s; s = s->next1(SegChordRest | SegGrace)) {
            foreach(Staff* st, _staves) {
                  int strack = st->idx() * VOICES;
                  int etrack = strack + VOICES;
                  for (int track = strack; track < etrack; ++track) {
                        if (!s->element(track))
                              continue;
                        Element* e = s->element(track);
                        if (e->type() != CHORD)
                              continue;
                        Chord* c = static_cast<Chord*>(e);
                        int channel = st->channel(c->tick(), c->voice());
                        foreach (Note* note, c->notes()) {
                              if (note->hidden())
                                    continue;
                              if (note->tieBack())
                                    continue;
                              note->setSubchannel(channel);
                              }
                        }
                  }
            }
      }

//---------------------------------------------------------
//   searchVolta
//    return volta at tick
//---------------------------------------------------------

Volta* Score::searchVolta(int tick) const
      {
      foreach(Element* el, *gel()) {
            if (el->type() == VOLTA) {
                  Volta* volta = (Volta*)el;
                  if (tick >= volta->tick() && tick < volta->tick2())
                        return volta;
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//   isVolta
//    return true if no volta found
//---------------------------------------------------------

bool Score::isVolta(int tick, int repeat) const
      {
      Volta* volta = searchVolta(tick);
      if (volta == 0)
            return true;
      return volta->hasEnding(repeat);
      }

//---------------------------------------------------------
//   collectChord
//---------------------------------------------------------

void Score::collectChord(EventMap* events, Instrument* instr, Chord* chord, int tick, int len)
      {
      Arpeggio* arpeggio = chord->arpeggio();

      int arpeggioOffset = 0;
      static const int arpeggioNoteDistance = Duration(Duration::V_32ND).ticks();
      if (arpeggio && chord->notes().size() * arpeggioNoteDistance <= len)
            arpeggioOffset = arpeggioNoteDistance;

      int i = 0;
      foreach(Note* note, chord->notes()) {
            int channel = instr->channel(note->subchannel()).channel;
            int onTime = tick;
            if (arpeggio){
                if(arpeggio->subtype() != ARP_DOWN)
                    onTime = tick + i * arpeggioOffset;
                else
                    onTime = tick + (chord->notes().size() - 1 - i) * arpeggioOffset;
                }
            collectNote(events, channel, note, onTime, len);
            i++;
            }
      }

//---------------------------------------------------------
//   playNote
//---------------------------------------------------------

static void playNote(EventMap* events, Note* note, int channel, int pitch,
   int velo, int onTime, int offTime)
      {
      Event* ev = new Event(ME_NOTEON);
      ev->setChannel(channel);
      ev->setPitch(pitch);
      ev->setVelo(velo);
      ev->setTuning(note->tuning());
      ev->setNote(note);
      events->insertMulti(onTime, ev);

      ev = new Event(ME_NOTEON);
      ev->setChannel(channel);
      ev->setPitch(pitch);
      ev->setVelo(0);
      ev->setNote(note);
      events->insertMulti(offTime, ev);
      }

//---------------------------------------------------------
//   collectNote
//---------------------------------------------------------

void Score::collectNote(EventMap* events, int channel, Note* note, int onTime, int len)
      {
      int noteTick    = note->chord()->tick();
      int noteOffTick = noteTick + note->chord()->tickLen();

      if (note->onTimeType() == AUTO_VAL)
            note->setOnTimeOffset(onTime - noteTick);
      else if (note->onTimeType() == USER_VAL)
            onTime = noteTick + note->onTimeOffset();
      else {
            note->setOnTimeOffset(onTime - noteTick);
            onTime += note->onTimeUserOffset();
            }
      int offTime = onTime + len;
      if (note->offTimeType() == AUTO_VAL)
            note->setOffTimeOffset(offTime - noteOffTick);
      else if (note->offTimeType() == USER_VAL)
            offTime = noteOffTick + note->offTimeOffset();
      else  {
            note->setOffTimeOffset(offTime - noteOffTick);
            offTime += note->offTimeUserOffset();
            }

      if (note->hidden() || note->tieBack())       // do not play overlapping notes
            return;

      int pitch  = note->ppitch();
      int velo = note->velocity();
      if (note->veloType() == OFFSET_VAL) {
            velo = velo + (velo * note->veloOffset()) / 100;
            if (velo < 1)
                  velo = 1;
            else if (velo > 127)
                  velo = 127;
            }
      bool mordent = false;
      foreach(Articulation* a, *note->chord()->getArticulations()) {
            if (a->subtype() == MordentSym) {
                  mordent = true;
                  break;
                  }
            }
      if (mordent) {
            int l = len / 8;

            // TODO: downstep depends on scale
            int downstep = 2;

            playNote(events, note, channel, pitch, velo, onTime, onTime + l);
            playNote(events, note, channel, pitch-downstep, velo, onTime+l, onTime + l+l);
            playNote(events, note, channel, pitch, velo, onTime+l+l, onTime + len);
            }
      else
            playNote(events, note, channel, pitch, velo, onTime, offTime);
      }

//---------------------------------------------------------
//   OttavaShiftSegment
//---------------------------------------------------------

struct OttavaShiftSegment {
      int stick;
      int etick;
      int shift;
      };

//---------------------------------------------------------
//   collectMeasureEvents
//---------------------------------------------------------

void Score::collectMeasureEvents(EventMap* events, Measure* m, int staffIdx, int tickOffset)
      {
      QList<Chord*> lv;       // appoggiatura
      QList<Chord*> sv;       // acciaccatura

      // for the purpose of knowing whether to find the playPos after repeats
      bool playExpandRepeats = getAction("repeat")->isChecked();
      Part* instr = part(staffIdx);

      for (int voice = 0; voice < VOICES; ++voice) {
            int track = staffIdx * VOICES + voice;
            for (Segment* seg = m->first(); seg; seg = seg->next()) {
                  Element* el = seg->element(track);
                  if (!el || el->type() != CHORD)
                        continue;
                  Chord* chord = static_cast<Chord*>(el);
                  if (chord->noteType() != NOTE_NORMAL) {
                        if (chord->noteType() == NOTE_ACCIACCATURA)
                              sv.append(chord);
                        else if (chord->noteType() == NOTE_APPOGGIATURA)
                              lv.append(chord);
                        continue;
                        }
                  int gateTime = 100;
                  int tick     = chord->tick();
                  if (playExpandRepeats && !_foundPlayPosAfterRepeats && tick == playPos()) {
                        setPlayPos(tick + tickOffset);
                        _foundPlayPosAfterRepeats = true;
                        }
                  foreach (Element* e, *m->score()->gel()) {
                        if (e->type() == SLUR) {
                              Slur* slur = static_cast<Slur*>(e);
                              if (slur->startElement()->staffIdx() != staffIdx)
                                    continue;
                              int tick1 = slur->startElement()->tick();
                              int tick2 = slur->endElement()->tick();
                              int strack = slur->startElement()->track();
                              if ((tick >= tick1) && (tick < tick2) && (strack == track)) {
                                    gateTime = _style[ST_slurGateTime].toInt();
                                    }
                              }
                        }
                  foreach (Articulation* a, *chord->getArticulations())
                        gateTime = (gateTime * a->relGateTime()) / 100;

                  // compute len of chord

                  int len = chord->tickLen();
                  int apl = 0;
                  if (!lv.isEmpty()) {
                        foreach(Chord* c, lv) {
                              apl += c->tickLen();
                              }
                        // treat appogiatura as acciaccatura if it exceeds the note length
                        if (apl >= len)
                              sv = lv;
                        }
                  if (!sv.isEmpty()) {
                        //
                        // move acciaccatura's in front of
                        // main note
                        //
                        int sl  = len / 4;
                        int ssl = sl / sv.size();
                        foreach(Chord* c, sv) {
                              collectChord(events, instr,
                                 c,
                                 tick + tickOffset - sl,
                                 ssl * gateTime / 100 - 1
                                 );
                              sl -= ssl;
                              }
                        }
                  else if (!lv.isEmpty()) {
                        //
                        // appoggiatura's use time from main note
                        //
                        int sl = 0;
                        foreach(Chord* c, lv) {
                              int ssl = c->tickLen();
                              collectChord(events, instr,
                                 c,
                                 tick + tickOffset + sl,
                                 ssl * gateTime / 100 - 1
                                 );
                              sl += ssl;
                              }
                        len -= sl;
                        tick += sl;
                        }
                  {
                  tick += tickOffset;
                  Arpeggio* arpeggio = chord->arpeggio();

                  int arpeggioOffset = 0;
                  static const int arpeggioNoteDistance = Duration(Duration::V_32ND).ticks();
                  if (arpeggio && chord->notes().size() * arpeggioNoteDistance <= len)
                        arpeggioOffset = arpeggioNoteDistance;

                  int i = 0;

                  // -- swing -- //
                  double swingCoeff= swingRatio();

                  //deal with odd measure in anacrusis
                  int offSet = 0;
                  if (!sigmap()->timesig(m->tick()).nominalEqualActual() && m->tickLen()%480 !=0) {
                        offSet = 480 - m->tickLen() % 480;
                        }

                  //detect 8th on the offbeat
                  bool swing = ((tick - m->tick()+offSet)%AL::division == 240 && chord->tickLen() == 240);

                  if (swing)
                        tick += (swingCoeff * AL::division /2);

                  //on the beat and a 8th
                  bool swingBeat = ((tick - m->tick()+offSet)%AL::division == 0 && chord->tickLen() == 240);
                  if (swingBeat) {
                        //find chord on counter beat and verify it's an 8th
                        ChordRest* ncr = nextChordRest(chord);
                        if (ncr) {
                              swingBeat = ((ncr->tick() == tick + 240) && (ncr->tickLen() == 240));
                              }
                        else
                              swingBeat = false;
                        }

                  // -- end swing -- //

                  foreach(Note* note, chord->notes()) {
                        int channel = instr->channel(note->subchannel()).channel;

                        int tickLen = note->chord()->tickLen();
                        int len     = note->chord()->tickLen();
                        bool tiedNote = false;
                        int lastNoteLen = len;
                        if (note->tieFor()) {
                              Note* n = note;
                              tiedNote = true;
                              while (n->tieFor()) {
                                    if (n->tieFor()->endNote() == 0)
                                          break;
                                    n = n->tieFor()->endNote();
                                    lastNoteLen = n->chord()->tickLen();
                                    len += lastNoteLen;
                                    }
                              }
                        if (tiedNote)
                              len = len - lastNoteLen + ((lastNoteLen * gateTime) / 100 - 1);
                        else
                              len = (len * gateTime) / 100 - 1;

                        //swing

                        if (swing)
                            len *= (1-swingCoeff);
                        if (swingBeat)
                            len *= (1+swingCoeff);

                        int noteLen;
                        if (note->offTimeType() == AUTO_VAL) {
                              note->setOffTimeOffset(tickLen - len);
                              noteLen = len;
                              }
                        else if (note->offTimeType() == USER_VAL)
                              noteLen = tickLen + note->offTimeOffset();
                        else  {
                              note->setOffTimeOffset(tickLen - len);
                              noteLen = tickLen + note->offTimeOffset() + note->offTimeUserOffset();
                              }

                        int onTime = tick;
                        if(arpeggio){
                            if(arpeggio->subtype() != ARP_DOWN)
                                onTime = tick + i * arpeggioOffset;
                            else
                                onTime = tick + (chord->notes().size() - 1 - i) * arpeggioOffset;
                            }
                        collectNote(events, channel, note, onTime, noteLen);
                        i++;
                        }
                  }
                  lv.clear();
                  sv.clear();
                  }
            }
      //
      // collect program changes and controller
      //
      foreach(const Element* e, *m->el()) {
            if (e->type() != STAFF_TEXT || e->staffIdx() != staffIdx)
                  continue;
            const StaffText* st = static_cast<const StaffText*>(e);
            int tick = st->tick() + tickOffset;
            foreach (const ChannelActions& ca, *st->channelActions()) {
                  int channel = ca.channel;
                  foreach(const QString& ma, ca.midiActionNames) {
                        NamedEventList* nel = instr->midiAction(ma, channel);
                        if (!nel)
                              continue;
                        int n = nel->events.size();
                        for (int i = n-1; i >= 0; --i) {
                              Event* event = new Event(*nel->events[i]);
                              event->setOntime(tick);
                              event->setChannel(channel);
                              events->insertMulti(tick, event);
                              }
                        }
                  }
            if (st->setAeolusStops()) {
                  Staff* staff = st->staff();
                  int voice   = 0;
                  int channel = staff->channel(tick, voice);

                  for (int i = 0; i < 4; ++i) {
                        for (int k = 0; k < 16; ++k) {
                              if (st->getAeolusStop(i, k)) {
                                    Event* event = new Event;
                                    event->setType(ME_CONTROLLER);
                                    event->setController(98);
                                    event->setValue(k);
                                    event->setOntime(tick);
                                    event->setChannel(channel);
                                    events->insertMulti(tick, event);
                                    }
                              }
                        Event* event = new Event;
                        event->setType(ME_CONTROLLER);
                        event->setController(98);
                        event->setValue(96 + i);
                        event->setOntime(tick);
                        event->setChannel(channel);
                        events->insertMulti(tick, event);

                        event = new Event;
                        event->setType(ME_CONTROLLER);
                        event->setController(98);
                        event->setValue(64 + i);
                        event->setOntime(tick);
                        event->setChannel(channel);
                        events->insertMulti(tick, event);
                        }
                  }
            }
      }

//---------------------------------------------------------
//   searchLabel
//---------------------------------------------------------

Measure* Score::searchLabel(const QString& s, Measure* start)
      {
      if (s == "start")
            return firstMeasure();
      if (start == 0)
            start = firstMeasure();
      for (Measure* m = start; m; m = m->nextMeasure()) {
            foreach(const Element* e, *m->el()) {
                  if (e->type() == MARKER) {
                        const Marker* marker = static_cast<const Marker*>(e);
                        if (marker->label() == s)
                              return m;
                        }
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//   toEList
//---------------------------------------------------------

void Score::toEList(EventMap* events, int staffIdx)
      {
      Part* instr = part(staffIdx);
      int channel = instr->channel(0).channel;

      foreach(const RepeatSegment* rs, *_repeatList) {
            int startTick  = rs->tick;
            int endTick    = startTick + rs->len;
            int tickOffset = rs->utick - rs->tick;
            for (Measure* m = tick2measure(startTick); m; m = m->nextMeasure()) {
                  collectMeasureEvents(events, m, staffIdx, tickOffset);
                  if (m->tick() + m->tickLen() >= endTick)
                        break;
                  }
            foreach(Element* e, _gel) {
                  if (e->type() == PEDAL && e->staffIdx() == staffIdx) {
                        Pedal* p = static_cast<Pedal*>(e);
                        if (p->tick() >= startTick && p->tick() < endTick) {
                              Event* ev = new Event(ME_CONTROLLER);
                              ev->setChannel(channel);
                              ev->setController(CTRL_SUSTAIN);
                              ev->setValue(127);
                              events->insertMulti(p->tick() + tickOffset, ev);

                              ev = new Event(ME_CONTROLLER);
                              ev->setChannel(channel);
                              ev->setController(CTRL_SUSTAIN);
                              ev->setValue(0);
                              events->insertMulti(p->tick2() + tickOffset, ev);
                              }
                        }
                  }
            }
      }

//---------------------------------------------------------
//   updateRepeatList
//---------------------------------------------------------

void Score::updateRepeatList(bool expandRepeats)
      {
      if (!expandRepeats) {
            foreach(RepeatSegment* s, *_repeatList)
                  delete s;
            _repeatList->clear();
            Measure* m = lastMeasure();
            if (m == 0)
                  return;
            RepeatSegment* s = new RepeatSegment;
            s->tick  = 0;
            s->len   = m->tick() + m->tickLen();
            s->utick = 0;
            s->utime = 0.0;
            s->timeOffset = 0.0;
            _repeatList->append(s);
            }
      else
            _repeatList->unwind();
      if (debugMode)
            _repeatList->dump();
      }

//---------------------------------------------------------
//   toEList
//    export score to event list
//---------------------------------------------------------

void Score::toEList(EventMap* events)
      {
      updateRepeatList(getAction("repeat")->isChecked());
      _foundPlayPosAfterRepeats = false;
      updateChannel();
      for (int staffIdx = 0; staffIdx < nstaves(); ++staffIdx)
            toEList(events, staffIdx);
      }

//---------------------------------------------------------
//   fixPpitch
//    calculate play pitch and velocity for all notes
//---------------------------------------------------------

void Score::fixPpitch()
      {
      int ns = nstaves();
      QList<OttavaShiftSegment> osl[ns];

      //
      //    collect ottavas
      //
      foreach(Element* e, _gel) {
            if (e->type() == OTTAVA) {
                  Ottava* ottava = static_cast<Ottava*>(e);
                  OttavaShiftSegment ss;
                  ss.stick = ottava->tick();
                  ss.etick = ottava->tick2();
                  ss.shift = ottava->pitchShift();
                  osl[e->staffIdx()].append(ss);
                  }
            }
      //
      //    collect Dynamics
      //
      VeloList velo[ns];

      for (int staffIdx = 0; staffIdx < nstaves(); ++staffIdx) {
            velo[staffIdx].setVelo(0, 80);
            Part* prt      = part(staffIdx);
            int partStaves = prt->nstaves();
            int partStaff  = Score::staffIdx(prt);

            for (Measure* m = firstMeasure(); m; m = m->nextMeasure()) {
                  foreach(const Element* e, *m->el()) {
                        if (e->type() != DYNAMIC)
                              continue;
                        const Dynamic* d = static_cast<const Dynamic*>(e);
                        int v = d->velocity();
                        int dStaffIdx = d->staffIdx();
                        if (v < 1)     //  illegal value
                              continue;
                        switch(d->dynType()) {
                              case DYNAMIC_STAFF:
                                    if( dStaffIdx == staffIdx)
                                        velo[staffIdx].setVelo(d->tick(), v);
                                    break;
                              case DYNAMIC_PART:
                                    if(dStaffIdx >= partStaff && dStaffIdx < partStaff+partStaves){
                                        for (int i = partStaff; i < partStaff+partStaves; ++i)
                                              velo[i].setVelo(d->tick(), v);
                                    }
                                    break;
                              case DYNAMIC_SYSTEM:
                                    for (int i = 0; i < nstaves(); ++i)
                                          velo[i].setVelo(d->tick(), v);
                                    break;
                              }
                        }
                  }
            }

      for (int staffIdx = 0; staffIdx < ns; ++staffIdx) {
            int pitchOffset = styleB(ST_concertPitch) ? 0 : part(staffIdx)->transpose().chromatic;
            Instrument* instr = part(staffIdx);

            for (Segment* seg = firstSegment(); seg; seg = seg->next1()) {
                  if (seg->subtype() != SegChordRest && seg->subtype() != SegGrace)
                        continue;
                  int ottavaShift = 0;
                  foreach(const OttavaShiftSegment& ss, osl[staffIdx]) {
                        if (seg->tick() >= ss.stick && seg->tick() < ss.etick) {
                              ottavaShift = ss.shift;
                              break;
                              }
                        }
                  int strack = staffIdx * VOICES;
                  int etrack = strack + VOICES;
                  for (int track = strack; track < etrack; ++track) {
                        Element* el = seg->element(track);
                        if (!el || el->type() != CHORD)
                              continue;
                        Chord* chord = static_cast<Chord*>(el);

                        //
                        // get velocity depending on dynamic marks as "p" of "sfz"
                        // crescendo and diminuendo
                        //
                        int velocity = velo[staffIdx].velo(chord->tick());
                        foreach(Note* note, chord->notes()) {

                              //
                              // adjust velocity for instrument, channel and
                              // depending on articulation marks
                              //
                              int channel = note->subchannel();
                              instr->updateVelocity(&velocity, channel, "");
                              foreach(Articulation* a, *chord->getArticulations())
                                    instr->updateVelocity(&velocity, channel, a->subtypeName());

                              note->setPpitch(note->pitch() + pitchOffset + ottavaShift);
                              switch (note->veloType()) {
                                    case OFFSET_VAL:
                                          velocity += (velocity * note->veloOffset()) / 100;
                                          // fall through

                                    case AUTO_VAL:
                                          if (velocity > 127)
                                                velocity = 127;
                                          else if (velocity < 1)
                                                velocity = 1;
                                          note->setVelocity(velocity);
                                          break;
                                    case USER_VAL:
                                          break;
                                    }
                              }
                        }
                  }
            }
      }


