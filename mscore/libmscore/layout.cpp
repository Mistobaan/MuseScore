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

#include <fenv.h>
#include "page.h"
#include "sig.h"
#include "key.h"
#include "clef.h"
#include "score.h"
#include "segment.h"
#include "text.h"
#include "staff.h"
#include "style.h"
#include "timesig.h"
#include "chord.h"
#include "note.h"
#include "slur.h"
#include "keysig.h"
#include "barline.h"
#include "repeat.h"
#include "box.h"
#include "system.h"
#include "part.h"
#include "utils.h"
#include "measure.h"
#include "volta.h"
#include "beam.h"
#include "tuplet.h"
#include "sym.h"
#include "fingering.h"
#include "stem.h"
#include "layoutbreak.h"
#include "mscore.h"
#include "accidental.h"
#include "undo.h"
#include "layout.h"

//---------------------------------------------------------
//   rebuildBspTree
//---------------------------------------------------------

void Score::rebuildBspTree()
      {
      foreach(Page* page, _pages)
            page->rebuildBspTree();
      }

//---------------------------------------------------------
//   searchNote
//    search for note or rest before or at tick position tick
//    in staff
//---------------------------------------------------------

ChordRest* Score::searchNote(int tick, int track) const
      {
      int startTrack = track;
      int endTrack   = startTrack + 1;

      ChordRest* ipe = 0;
      for (const Measure* m = firstMeasure(); m; m = m->nextMeasure()) {
            for (int track = startTrack; track < endTrack; ++track) {
                  for (Segment* segment = m->first(SegChordRest);
                     segment; segment = segment->next(SegChordRest)) {
                        ChordRest* cr = static_cast<ChordRest*>(segment->element(track));
                        if (!cr)
                              continue;
                        if (cr->tick() == tick)
                              return cr;
                        if (cr->tick() >  tick)
                              return ipe ? ipe : cr;
                        ipe = cr;
                        }
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//   clefOffset
//---------------------------------------------------------

int Score::clefOffset(int tick, Staff* staff) const
      {
      return clefTable[staff->clef(tick)].yOffset;
      }

//---------------------------------------------------------
//   AcEl
//---------------------------------------------------------

struct AcEl {
      Note* note;
      qreal x;
      };

//---------------------------------------------------------
//   layoutChords1
//    only called from layout0
//    - calculate displaced note heads
//---------------------------------------------------------

void Score::layoutChords1(Segment* segment, int staffIdx)
      {
      Staff* staff = Score::staff(staffIdx);

      if (staff->part()->instr()->drumset() || staff->useTablature())
            return;

      int startTrack = staffIdx * VOICES;
      int endTrack   = startTrack + VOICES;
      int voices     = 0;
      QList<Note*> notes;
      for (int track = startTrack; track < endTrack; ++track) {
            Element* e = segment->element(track);
            if (e && (e->type() == CHORD)) {
                  ++voices;
                  notes.append(static_cast<Chord*>(e)->notes());
                  }
            }
      if (notes.isEmpty())
            return;

      int startIdx, endIdx, incIdx;

      if (notes[0]->chord()->up() || (voices > 1)) {
            startIdx = 0;
            incIdx   = 1;
            endIdx   = notes.size();
            for (int i = 0; i < endIdx-1; ++i) {
                  if ((notes[i]->line() == notes[i+1]->line())
                     && (notes[i]->track() != notes[i+1]->track())
                     && (!notes[i]->chord()->up() && notes[i+1]->chord()->up())
                     ) {
                        Note* n = notes[i];
                        notes[i] = notes[i+1];
                        notes[i+1] = n;
                        }
                  }
            }
      else {
            startIdx = notes.size() - 1;
            incIdx   = -1;
            endIdx   = -1;
            }

      bool moveLeft = false;
      int ll        = 1000;      // line distance to previous note head
      bool isLeft   = notes[startIdx]->chord()->up();
      int move1     = notes[startIdx]->chord()->staffMove();
      bool mirror   = false;
      int lastHeadGroup  = -1;
      int lastHeadType  = -1;

      for (int idx = startIdx; idx != endIdx; idx += incIdx) {
            Note* note    = notes[idx];
            Chord* chord  = note->chord();
            int move      = chord->staffMove();
            int line      = note->line();
            int ticks     = chord->actualTicks();
            int headGroup = note->headGroup();
            int headType  =  (note->headType() == HEAD_AUTO) ? note->chord()->durationType().headType() : note->headType() - 1;

            bool conflict = (qAbs(ll - line) < 2) && (move1 == move);
            bool sameHead = (ll == line) && (headGroup == lastHeadGroup) && (headType == lastHeadType);
            if ((chord->up() != isLeft) || conflict)
                  isLeft = !isLeft;
            bool nmirror  = (chord->up() != isLeft) && !sameHead;

            note->setHidden(false);
            chord->rxpos() = 0.0;

            if (conflict && (nmirror == mirror) && idx) {
                  if (sameHead) {
                        Note* pnote = notes[idx-1];
                        if (note->userOff().isNull() && pnote->userOff().isNull()) {
                              if (ticks > pnote->chord()->actualTicks()) {
                                    pnote->setHidden(true);
                                    // TODO: pnote->setAccidentalType(ACC_NONE);
                                    note->setHidden(false);
                                    }
                              else {
                                    // TODO: note->setAccidentalType(ACC_NONE);
                                    note->setHidden(true);
                                    }
                              }
                        else
                              note->setHidden(false);
                        }
                  else {
                        if ((line > ll) || !chord->up()) {
                              note->chord()->rxpos() = note->headWidth() - note->point(styleS(ST_stemWidth));
                              note->rxpos() = 0.0;
                              }
                        else {
                              notes[idx-incIdx]->chord()->rxpos() = note->headWidth() - note->point(styleS(ST_stemWidth));
                              note->rxpos() = 0.0;
                              }
                        moveLeft = true;
                        }
                  }
            if (note->userMirror() == DH_AUTO) {
                  mirror = nmirror;
                  }
            else {
                  mirror = note->chord()->up();
                  if (note->userMirror() == DH_LEFT)
                        mirror = !mirror;
                  }
            note->setMirror(mirror);
            if (mirror)                   //??
                  moveLeft = true;

            move1         = move;
            ll            = line;
            lastHeadGroup = headGroup;
            lastHeadType  = headType;
            }

      //---------------------------------------------------
      //    layout accidentals
      //    find column for dots
      //---------------------------------------------------

      QList<AcEl> aclist;

      qreal dotPosX  = 0.0;
      int nNotes = notes.size();
      for (int i = nNotes-1; i >= 0; --i) {
            Note* note     = notes[i];
            Accidental* ac = note->accidental();
            if (ac) {
                  ac->setMag(note->mag());
                  ac->layout();
                  AcEl acel;
                  acel.note = note;
                  acel.x    = 0.0;
                  aclist.append(acel);
                  }
            qreal xx = note->pos().x() + note->headWidth();
            if (xx > dotPosX)
                  dotPosX = xx;
            }
      segment->setDotPosX(staffIdx, dotPosX);

      int nAcc = aclist.size();
      if (nAcc == 0)
            return;
      qreal pd  = point(styleS(ST_accidentalDistance));
      qreal pnd = point(styleS(ST_accidentalNoteDistance));
      //
      // layout top accidental
      //
      Note* note      = aclist[0].note;
      Accidental* acc = note->accidental();
      aclist[0].x     = -pnd * acc->mag() - acc->width() - acc->bbox().x();

      //
      // layout bottom accidental
      //
      if (nAcc > 1) {
            note = aclist[nAcc-1].note;
            acc  = note->accidental();
            int l1 = aclist[0].note->line();
            int l2 = note->line();

            int st1   = aclist[0].note->accidental()->subtype();
            int st2   = acc->subtype();
            int ldiff = st1 == ACC_FLAT ? 4 : 5;

            if (qAbs(l1-l2) > ldiff) {
                  aclist[nAcc-1].x = -pnd * acc->mag() - acc->width() - acc->bbox().x();
                  }
            else {
                  if ((st1 == ACC_FLAT) && (st2 == ACC_FLAT) && (qAbs(l1-l2) > 2))
                        aclist[nAcc-1].x = aclist[0].x - acc->width() * .5;
                  else
                        aclist[nAcc-1].x = aclist[0].x - acc->width();
                  }
            }

      //
      // layout middle accidentals
      //
      if (nAcc > 2) {
            int n = nAcc - 1;
            for (int i = 1; i < n; ++i) {
                  note = aclist[i].note;
                  acc  = note->accidental();
                  int l1 = aclist[i-1].note->line();
                  int l2 = note->line();
                  int l3 = aclist[n].note->line();
                  qreal x = 0.0;

                  int st1 = aclist[i-1].note->accidental()->subtype();
                  int st2 = acc->subtype();

                  int ldiff = st1 == ACC_FLAT ? 4 : 5;
                  if (qAbs(l1-l2) <= ldiff) {   // overlap accidental above
                        if ((st1 == ACC_FLAT) && (st2 == ACC_FLAT) && (qAbs(l1-l2) > 2))
                              x = aclist[i-1].x + acc->width() * .5;    // undercut flats
                        else
                              x = aclist[i-1].x;
                        }

                  ldiff = acc->subtype() == ACC_FLAT ? 4 : 5;
                  if (qAbs(l2-l3) <= ldiff) {       // overlap accidental below
                        if (aclist[n].x < x)
                              x = aclist[n].x;
                        }
                  if (x == 0.0 || x > acc->width())
                        x = -pnd * acc->mag() - acc->bbox().x();
                  else
                        x -= pd * acc->mag();   // accidental distance
                  aclist[i].x = x - acc->width() - acc->bbox().x();
                  }
            }

      foreach(const AcEl e, aclist) {
            Note* note = e.note;
            qreal x   = e.x;
            if (moveLeft) {
                  Chord* chord = note->chord();
                  if (((note->mirror() && chord->up()) || (!note->mirror() && !chord->up())))
                        x -= note->headWidth();
                  }
            note->accidental()->setPos(x, 0);
            note->accidental()->adjustReadPos();
            }
      }

//-------------------------------------------------------------------
//    layoutStage1
//    - compute note head lines and accidentals
//    - mark multi measure rest breaks if in multi measure rest mode
//-------------------------------------------------------------------

void Score::layoutStage1()
      {
      int idx = 0;
      for (Measure* m = firstMeasure(); m; m = m->nextMeasure()) {
            ++idx;
            m->layoutStage1();
            foreach(Spanner* spanner, m->spannerFor()) {
                  if (spanner->type() == VOLTA) {
                        m->setBreakMMRest(true);
                        }
                  }
            MeasureBase* mb = m->prev();
            if (mb && mb->type() == MEASURE) {
                  Measure* pm = static_cast<Measure*>(mb);
                  if (pm->endBarLineType() != NORMAL_BAR && pm->endBarLineType() != BROKEN_BAR)
                        m->setBreakMMRest(true);
                  foreach(Spanner* spanner, pm->spannerBack()) {
                        if (spanner->type() == VOLTA) {
                              m->setBreakMMRest(true);
                              }
                        }
                  }
            }
      }

//---------------------------------------------------------
//   layoutStage2
//    auto - beamer
//---------------------------------------------------------

void Score::layoutStage2()
      {
      int tracks = nstaves() * VOICES;

      for (int track = 0; track < tracks; ++track) {
            ChordRest* a1    = 0;      // start of (potential) beam
            Beam* beam       = 0;      // current beam
            Measure* measure = 0;

            BeamMode bm = BEAM_AUTO;
            SegmentTypes st = SegGrace | SegChordRest;
            for (Segment* segment = firstSegment(st); segment; segment = segment->next1(st)) {
                  ChordRest* cr = static_cast<ChordRest*>(segment->element(track));
                  if (cr == 0)
                        continue;
                  bm = cr->beamMode();
                  if (cr->measure() != measure) {
                        if (measure && !beamModeMid(bm)) {
                              if (beam) {
                                    beam->layout1();
                                    beam = 0;
                                    }
                              else if (a1) {
                                    a1->removeDeleteBeam();
                                    a1->layoutStem1();
                                    a1 = 0;
                                    }
                              }
                        measure = cr->measure();
                        if (!beamModeMid(bm)) {
                              a1      = 0;
                              beam    = 0;
                              }
                        }
                  if (segment->subtype() == SegGrace) {
                        Segment* nseg = segment->next();
                        if (nseg
                           && nseg->subtype() == SegGrace
                           && nseg->element(track)
                           && static_cast<ChordRest*>(nseg->element(track))->durationType().hooks())
                              {
                              Beam* b = cr->beam();
                              if (b == 0 || b->elements().front() != cr) {
                                    b = new Beam(this);
                                    b->setTrack(track);
                                    b->setGenerated(true);
                                    cr->removeDeleteBeam();
                                    b->add(cr);
                                    }
                              Segment* s = nseg;
                              for (;;) {
                                    nseg = s;
                                    ChordRest* cr = static_cast<ChordRest*>(nseg->element(track));
                                    b->add(cr);
                                    s = nseg->next();
                                    if (!s || (s->subtype() != SegGrace) || !s->element(track))
                                          break;
                                    }
                              b->layout1();
                              segment = nseg;
                              }
                        else {
                              cr->removeDeleteBeam();
                              cr->layoutStem1();
                              }
                        continue;
                        }
                  if ((cr->durationType().type() <= TDuration::V_QUARTER) || (bm == BEAM_NO)) {
                        if (beam) {
                              beam->layout1();
                              beam = 0;
                              }
                        if (a1) {
                              a1->removeDeleteBeam();
                              a1->layoutStem1();
                              a1 = 0;
                              }
                        cr->removeDeleteBeam();
                        cr->layoutStem1();
                        continue;
                        }
                  bool beamEnd = false;
                  if (beam) {
                        ChordRest* le = beam->elements().back();
                        if ((!beamModeMid(bm) && (le->tuplet() != cr->tuplet())) || (bm == BEAM_BEGIN)) {
                              beamEnd = true;
                              }
                        else if (!beamModeMid(bm)) {
                              if (endBeam(measure->timesig(), cr, le))
                                    beamEnd = true;
                              if (le->tick() + le->actualTicks() < cr->tick())
                                    beamEnd = true;
                              }
                        if (beamEnd) {
                              beam->layout1();
                              beam = 0;
                              }
                        else {
                              cr->removeDeleteBeam();
                              beam->add(cr);
                              cr = 0;

                              // is cr the last beam element?
                              if (bm == BEAM_END) {
                                    beam->layout1();
                                    beam = 0;
                                    }
                              }
                        }
                  if (cr && cr->tuplet() && (cr->tuplet()->elements().back() == cr)) {
                        if (beam) {
                              beam->layout1();
                              beam = 0;
                              cr->removeDeleteBeam();
                              cr->layoutStem1();
                              }
                        else if (a1) {
                              beam = a1->beam();
                              if (beam == 0 || beam->elements().front() != a1) {
                                    beam = new Beam(this);
                                    beam->setTrack(track);
                                    beam->setGenerated(true);
                                    a1->removeDeleteBeam();
                                    beam->add(a1);
                                    }
                              cr->removeDeleteBeam();
                              beam->add(cr);
                              a1 = 0;
                              beam->layout1();
                              beam = 0;
                              }
                        else {
                              cr->setBeam(0);
                              cr->layoutStem1();
                              }
                        }
                  else if (cr) {
                        if (a1 == 0)
                              a1 = cr;
                        else {
                              if (!beamModeMid(bm)
                                   &&
                                   (endBeam(measure->timesig(), cr, a1)
                                   || bm == BEAM_BEGIN
                                   || (a1->segment()->subtype() != cr->segment()->subtype())
                                   || (a1->tick() + a1->actualTicks() < cr->tick())
                                   )
                                 ) {
                                    a1->removeDeleteBeam();
                                    a1->layoutStem1();      //?
                                    a1 = cr;
                                    }
                              else {
                                    beam = a1->beam();
                                    if (beam == 0 || beam->elements().front() != a1) {
                                          beam = new Beam(this);
                                          beam->setGenerated(true);
                                          beam->setTrack(track);
                                          a1->removeDeleteBeam();
                                          beam->add(a1);
                                          }
                                    cr->removeDeleteBeam();
                                    beam->add(cr);
                                    a1 = 0;
                                    }
                              }
                        }
                  }
            if (beam)
                  beam->layout1();
            else if (a1) {
                  a1->removeDeleteBeam();
                  a1->layoutStem1();
                  }
            }
      }

//---------------------------------------------------------
//   layoutStage3
//---------------------------------------------------------

void Score::layoutStage3()
      {
      for (int staffIdx = 0; staffIdx < nstaves(); ++staffIdx) {
            for (Segment* segment = firstSegment(); segment; segment = segment->next1()) {
                  if ((segment->subtype() == SegChordRest) || (segment->subtype() == SegGrace)) {
                        layoutChords1(segment, staffIdx);
                        }
                  }
            }
      }

//---------------------------------------------------------
//   layout
//    - measures are akkumulated into systems
//    - systems are akkumulated into pages
//   already existent systems and pages are reused
//---------------------------------------------------------

void Score::doLayout()
      {
      {
      QWriteLocker locker(&_layoutLock);

      _symIdx = 0;
      if (_style.valueSt(ST_MusicalSymbolFont) == "Gonville")
            _symIdx = 1;

      initSymbols(_symIdx);

      if (layoutFlags & LAYOUT_FIX_TICKS)
            fixTicks();
      if (layoutFlags & LAYOUT_FIX_PITCH_VELO)
            updateVelo();
      layoutFlags = 0;

      int nstaves = _staves.size();
      for (int staffIdx = 0; staffIdx < nstaves; ++staffIdx) {
            Staff* st = _staves[staffIdx];
            if (!st->updateKeymap())
                  continue;
            int track = staffIdx * VOICES;
            st->keymap()->clear();
            KeySig* key1 = 0;
            for (Measure* m = firstMeasure(); m; m = m->nextMeasure()) {
                  for (Segment* s = m->first(); s; s = s->next()) {
                        if (s == s->next())
                              abort();
                        Element* e = s->element(track);
                        if (e == 0 || e->generated())
                              continue;
                        if ((s->subtype() == SegKeySig) && st->updateKeymap()) {
                              KeySig* ks = static_cast<KeySig*>(e);
                              int naturals = key1 ? key1->keySigEvent().accidentalType() : 0;
                              ks->setOldSig(naturals);
                              st->setKey(s->tick(), ks->keySigEvent());
                              key1 = ks;
                              }
                        }
                  if (m->sectionBreak() && (_layoutMode != LayoutFloat))
                        key1 = 0;
                  }
            st->setUpdateKeymap(false);
            }

#if 0 // DEBUG
      if (startLayout) {
            startLayout->setDirty();
            if (doReLayout()) {
                  startLayout = 0;
                  return;
                  }
            }
#endif
      if (_staves.isEmpty() || first() == 0) {
            // score is empty
            foreach(Page* page, _pages)
                  delete page;
            _pages.clear();

            Page* page = addPage();
            page->layout();
            page->setNo(0);
            page->setPos(0.0, 0.0);
            page->rebuildBspTree();
            return;
            }

      layoutStage1();   // compute note head lines and accidentals
      layoutStage2();   // beam notes, finally decide if chord is up/down
      layoutStage3();   // compute note head horizontal positions

      layoutSystems();  // create list of systems
      layoutPages();    // create list of pages

      //---------------------------------------------------
      //   place Spanner & beams
      //---------------------------------------------------

      int tracks = nstaves * VOICES;
      for (int track = 0; track < tracks; ++track) {
            for (Segment* segment = firstSegment(); segment; segment = segment->next1()) {
                  Element* e = segment->element(track);
                  if (e && e->isChordRest()) {
                        ChordRest* cr = static_cast<ChordRest*>(e);
                        if (cr->beam() && cr->beam()->elements().front() == cr)
                              cr->beam()->layout();

                        if (cr->type() == CHORD) {
                              Chord* c = static_cast<Chord*>(cr);
                              if (!c->beam())
                                    c->layoutStem();
                              c->layoutArpeggio2();
                              foreach(Note* n, c->notes()) {
                                    Tie* tie = n->tieFor();
                                    if (tie)
                                          tie->layout();
                                    }
                              }
                        cr->layoutArticulations();
                        }
                  else if (e && e->type() == BAR_LINE)
                        e->layout();
                  if (track == tracks-1) {
                        foreach(Spanner* s, segment->spannerFor())
                              s->layout();
                        foreach(Element* e, segment->annotations())
                              e->layout();
                        }
                  }
            }

      for (Measure* m = firstMeasure(); m; m = m->nextMeasure()) {
            m->layout2();
            foreach(Spanner* s, m->spannerFor())
                  s->layout();
            }

      rebuildBspTree();

      }     // unlock mutex
      foreach(MuseScoreView* v, viewer)
            v->layoutChanged();
      }

//---------------------------------------------------------
//   processSystemHeader
//    add generated timesig keysig and clef
//---------------------------------------------------------

void Score::processSystemHeader(Measure* m, bool isFirstSystem)
      {
      int tick = m->tick();
      int i = 0;
      foreach (Staff* staff, _staves) {
            if (!m->system()->staff(i)->show()) {
                  ++i;
                  continue;
                  }

            KeySig* keysig = 0;
            Clef*   clef   = 0;
            int strack     = i * VOICES;

            // we assume that keysigs and clefs are only in the first
            // track (voice 0) of a staff

            const KeySigEvent& keyIdx = staff->key(tick);
            const KeySigEvent& oKeySigBefore = staff->key(tick-1);

            for (Segment* seg = m->first(); seg; seg = seg->next()) {
                  // search only up to the first ChordRest
                  if (seg->subtype() == SegChordRest)
                        break;
                  Element* el = seg->element(strack);
                  if (!el)
                        continue;
                  switch (el->type()) {
                        case KEYSIG:
                              keysig = static_cast<KeySig*>(el);
                              keysig->changeKeySigEvent(keyIdx);
                              if (!keysig->isCustom() && oKeySigBefore.accidentalType() == keysig->keySignature())
                                    keysig->setOldSig(0);
                              keysig->setMag(staff->mag());
                              break;
                        case CLEF:
                              clef = static_cast<Clef*>(el);
                              clef->setMag(staff->mag());
                              clef->setSmall(false);
                              break;
                        case TIMESIG:
                              el->setMag(staff->mag());
                              break;
                        default:
                              break;
                        }
                  }
            bool needKeysig = keyIdx.isValid()
               /* && keyIdx.accidentalType() != 0 */
               && (isFirstSystem || styleB(ST_genKeysig))
               ;
            if (staff->useTablature())
                  needKeysig = false;
            if (needKeysig && !keysig) {
                  //
                  // create missing key signature
                  //
                  keysig = keySigFactory(keyIdx);
                  if (keysig) {
                        // if signature is not custom or prev. signature has same accid. as
                        // this one, reset naturals
                        if (!keysig->isCustom() && oKeySigBefore.accidentalType() == keysig->keySignature())
                              keysig->setOldSig(0);
                        keysig->setTrack(i * VOICES);
                        keysig->setGenerated(true);
                        keysig->setMag(staff->mag());
                        Segment* seg = m->undoGetSegment(SegKeySig, tick);
                        keysig->setParent(seg);
                        undoAddElement(keysig);
                        }
                  }
            else if (!needKeysig && keysig)
                  undoRemoveElement(keysig);
            bool needClef = isFirstSystem || styleB(ST_genClef);
            if (needClef) {
                  if (!clef) {
                        //
                        // create missing clef
                        //
                        clef = new Clef(this);
                        clef->setClefType(staff->clefTypeList(tick));
                        clef->setTrack(i * VOICES);
                        clef->setGenerated(true);
                        clef->setSmall(false);
                        clef->setMag(staff->mag());

                        Segment* s = m->undoGetSegment(SegClef, tick);
                        clef->setParent(s);
                        undoAddElement(clef);
                        }
                  if (clef->generated())
                        clef->setClefType(staff->clefTypeList(tick));
                  }
            else {
                  if (clef && clef->generated())
                        undoRemoveElement(clef);
                  }
            ++i;
            }
      }

//---------------------------------------------------------
//   getNextSystem
//---------------------------------------------------------

System* Score::getNextSystem(bool isFirstSystem, bool isVbox)
      {
      System* system;
      if (curSystem >= _systems.size()) {
            system = new System(this);
            _systems.append(system);
            }
      else {
            system = _systems[curSystem];
            system->clear();   // remove measures from system
            }
      system->setFirstSystem(isFirstSystem);
      system->setVbox(isVbox);
      if (!isVbox) {
            int nstaves = Score::nstaves();
            for (int i = system->staves()->size(); i < nstaves; ++i)
                  system->insertStaff(i);
            int dn = system->staves()->size() - nstaves;
            for (int i = 0; i < dn; ++i)
                  system->removeStaff(system->staves()->size()-1);
            }
      return system;
      }

//---------------------------------------------------------
//   skipEmptyMeasures
//    search for empty measures; return number if empty
//    measures in sequence
//---------------------------------------------------------

Measure* Score::skipEmptyMeasures(Measure* m, System* system)
      {
      Measure* sm = m;
      int n       = 0;
      while (m->isEmpty()) {
            MeasureBase* mb = m->next();
            if (m->breakMultiMeasureRest() && n)
                  break;
            ++n;
            if (!mb || (mb->type() != MEASURE))
                  break;
            m = static_cast<Measure*>(mb);
            }
      m = sm;
      if (n >= styleI(ST_minEmptyMeasures)) {
            m->setMultiMeasure(n);  // first measure is presented as multi measure rest
            m->setSystem(system);
            for (int i = 1; i < n; ++i) {
                  m = static_cast<Measure*>(m->next());
                  m->setMultiMeasure(-1);
                  m->setSystem(system);
                  }
            }
      else
            m->setMultiMeasure(0);
      return m;
      }

//---------------------------------------------------------
//   layoutSystem
//    return true if line continues
//---------------------------------------------------------

bool Score::layoutSystem(qreal& minWidth, qreal w, bool isFirstSystem, bool longName)
      {
      System* system = getNextSystem(isFirstSystem, false);

      qreal xo = 0;
      if (curMeasure->type() == HBOX)
            xo = point(static_cast<Box*>(curMeasure)->boxWidth());

      system->setInstrumentNames(longName);
      system->layout(xo);

      minWidth              = system->leftMargin();
      qreal systemWidth     = w;
      bool continueFlag     = false;
      bool isFirstMeasure   = true;
      Measure* firstMeasure = 0;
      Measure* lastMeasure  = 0;

      for (; curMeasure;) {
            MeasureBase* nextMeasure;
            if (curMeasure->type() == MEASURE) {
                  Measure* m = static_cast<Measure*>(curMeasure);
                  if (styleB(ST_createMultiMeasureRests)) {
                        nextMeasure = skipEmptyMeasures(m, system)->next();
                        }
                  else {
                        m->setMultiMeasure(0);
                        nextMeasure = curMeasure->next();
                        }
                  }
            else
                  nextMeasure = curMeasure->next();

            System* oldSystem = curMeasure->system();
            curMeasure->setSystem(system);
            qreal ww      = 0.0;
            qreal stretch = 0.0;

            if (curMeasure->type() == HBOX) {
                  ww = point(static_cast<Box*>(curMeasure)->boxWidth());
                  if (!isFirstMeasure) {
                        // try to put another system on current row
                        // if not a line break
                        continueFlag = !((curMeasure->lineBreak() || curMeasure->pageBreak())
                           && (_layoutMode != LayoutFloat));
                        }
                  }
            else if (curMeasure->type() == MEASURE) {
                  Measure* m = static_cast<Measure*>(curMeasure);
                  if (firstMeasure == 0)
                        firstMeasure = m;

                  if (isFirstMeasure)
                        processSystemHeader(m, isFirstSystem);

                  m->createEndBarLines();       // TODO: not set here

                  m->layoutX(1.0, true);
                  ww      = m->layoutWidth().stretchable;
                  stretch = m->userStretch() * styleD(ST_measureSpacing);

                  ww *= stretch;
                  if (ww < point(styleS(ST_minMeasureWidth)))
                        ww = point(styleS(ST_minMeasureWidth));
                  isFirstMeasure = false;
                  }

            // collect at least one measure
            if ((minWidth + ww > systemWidth) && !system->measures().isEmpty()) {
                  curMeasure->setSystem(oldSystem);
                  break;
                  }

            if (curMeasure->type() == MEASURE)
                  lastMeasure = static_cast<Measure*>(curMeasure);
            minWidth += ww;
            system->measures().append(curMeasure);
            ElementType nt = curMeasure->next() ? curMeasure->next()->type() : INVALID;
            int n = styleI(ST_FixMeasureNumbers);
            if ((n && system->measures().size() >= n)
               || continueFlag || ((curMeasure->pageBreak() || curMeasure->lineBreak()) && _layoutMode != LayoutFloat)
               || (nt == VBOX || nt == TBOX || nt == FBOX)) {
                  system->setPageBreak(curMeasure->pageBreak());
                  curMeasure = nextMeasure;
                  break;
                  }
            curMeasure = nextMeasure;
            }

      if (firstMeasure && lastMeasure && firstMeasure != lastMeasure) {
            if (removeGeneratedElements(firstMeasure, lastMeasure)) {
                  foreach(MeasureBase* mb, system->measures()) {
                        if (mb->type() != MEASURE)
                              continue;
                        Measure* m = static_cast<Measure*>(mb);
                        m->setDirty(true);
                        m->layoutX(1.0, true);
                        }
                  }
            }

      //
      //    hide empty staves
      //
      int staves = _staves.size();
      int staffIdx = 0;
      foreach (Staff* staff, _staves) {
            SysStaff* s  = system->staff(staffIdx);
            bool oldShow = s->show();
            if (styleB(ST_hideEmptyStaves)
               && (staves > 1)
               && !(isFirstSystem && styleB(ST_dontHideStavesInFirstSystem))
               ) {
                  bool hideStaff = true;
                  foreach(MeasureBase* m, system->measures()) {
                        if (m->type() != MEASURE)
                              continue;
                        Measure* measure = static_cast<Measure*>(m);
                        if (!measure->isMeasureRest(staffIdx)) {
                              hideStaff = false;
                              break;
                              }
                        }
                  s->setShow(hideStaff ? false : staff->show());
                  }
            else {
                  s->setShow(true);
                  }

            if (oldShow != s->show()) {
                  foreach (MeasureBase* mb, system->measures()) {
                        if (mb->type() != MEASURE)
                              continue;
                        static_cast<Measure*>(mb)->createEndBarLines();
                        }
                  }
            ++staffIdx;
            }
      return continueFlag && curMeasure;
      }

//---------------------------------------------------------
//   removeGeneratedElements (TimeSig Announce)
//    helper function
//---------------------------------------------------------

bool Score::removeGeneratedElements(Measure* sm, Measure* em)
      {
      bool layoutChanged = false;
      for (Measure* m = sm; m; m = m->nextMeasure()) {
            //
            // remove generated elements from all measures in [sm;em]
            //    assume: generated elements are only living in voice 0
            //    - do not remove end bar lines
            //    - set size of clefs to small
            //
            for (Segment* seg = m->first(); seg; seg = seg->next()) {
                  SegmentType st = seg->subtype();
                  if (st == SegEndBarLine)
                        continue;
                  for (int staffIdx = 0;  staffIdx < nstaves(); ++staffIdx) {
                        Element* el = seg->element(staffIdx * VOICES);
                        if (el == 0)
                              continue;
                        qreal staffMag = staff(staffIdx)->mag();

                        if (el->generated() && ((st == SegTimeSigAnnounce && m != em)
                            || (el->type() == CLEF && seg->tick() != sm->tick())
                            || (el->type() == KEYSIG && seg->tick() != sm->tick())))
                              {
                              if (!_undoRedo) {
                                    undoRemoveElement(el);
                                    layoutChanged = true;
                                    }
                              }
                        else if (el->type() == CLEF) {
                              Clef* clef = static_cast<Clef*>(el);
                              System* s = m->system();
                              bool small = seg != m->first() || s->firstMeasure() != m;
                              if (clef->small() != small) {
                                    clef->setSmall(small);
                                    layoutChanged = true;
                                    }
                              if (clef->mag() != staffMag) {
                                    clef->setMag(staffMag);
                                    layoutChanged = true;
                                    }
                              }
                        else if (el->type() == KEYSIG || el->type() == TIMESIG) {
                              if (el->mag() != staffMag) {
                                    el->setMag(staffMag);
                                    layoutChanged = true;
                                    }
                              }
                        }
                  }
            if (m == em)
                  break;
            }
      return layoutChanged;
      }

//---------------------------------------------------------
//   addPage
//---------------------------------------------------------

Page* Score::addPage()
      {
      Page* page = new Page(this);
      page->setNo(_pages.size());
      _pages.push_back(page);
      return page;
      }

//---------------------------------------------------------
//   connectTies
///   Rebuild tie connections.
//---------------------------------------------------------

void Score::connectTies()
      {
      int tracks = nstaves() * VOICES;
      Measure* m = firstMeasure();
      if (!m)
            return;
      for (Segment* s = m->first(); s; s = s->next1()) {
            for (int i = 0; i < tracks; ++i) {
                  Element* el = s->element(i);
                  if (el == 0 || el->type() != CHORD)
                        continue;
                  foreach(Note* n, static_cast<Chord*>(el)->notes()) {
                        Tie* tie = n->tieFor();
                        if (!tie)
                              continue;
                        Note* nnote = searchTieNote(n);
                        if (nnote == 0) {
                              qDebug("next note at %d voice %d for tie not found",
                                 s->tick(), i );
                              // n->setTieFor(0);  show short bow
                              // delete tie;
                              }
                        else {
                              tie->setEndNote(nnote);
                              nnote->setTieBack(tie);
                              }
                        }
                  }
            }
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void Score::add(Element* el)
      {
      switch(el->type()) {
            case MEASURE:
            case HBOX:
            case VBOX:
            case TBOX:
            case FBOX:
                  measures()->add((MeasureBase*)el);
                  break;
            case BEAM:
                  {
                  Beam* b = static_cast<Beam*>(el);
                  foreach(ChordRest* cr, b->elements())
                        cr->setBeam(b);
                  }
                  break;
            case SLUR:
                  break;
            default:
                  qDebug("Score::add() invalid element <%s>\n", el->name());
                  delete el;
                  break;
            }
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void Score::remove(Element* el)
      {
      switch(el->type()) {
            case MEASURE:
            case HBOX:
            case VBOX:
            case TBOX:
            case FBOX:
                  measures()->remove(static_cast<MeasureBase*>(el));
                  break;
            case BEAM:
                  {
                  Beam* b = static_cast<Beam*>(el);
                  foreach(ChordRest* cr, b->elements())
                        cr->setBeam(0);
                  }
                  break;
            case SLUR:
                  break;
            default:
                  qDebug("Score::remove(): invalid element %s\n", el->name());
                  break;
            }
      }

//---------------------------------------------------------
//   reLayout
//---------------------------------------------------------

void Score::reLayout(Measure* m)
      {
      startLayout = m;
      }

//---------------------------------------------------------
//   doReLayout
//    return true, if relayout was successful; if false
//    a full layout must be done starting at "startLayout"
//---------------------------------------------------------

bool Score::doReLayout()
      {
#if 0
      if (startLayout->type() == MEASURE)
            static_cast<Measure*>(startLayout)->layout0();
#endif
      System* system  = startLayout->system();
      qreal sysWidth  = system->width();
      qreal minWidth = system->leftMargin();

      //
      //  check if measures still fit in system
      //
      MeasureBase* m = 0;
      foreach(m, system->measures()) {
            qreal ww;
            if (m->type() == HBOX)
                  ww = point(static_cast<Box*>(m)->boxWidth());
            else {            // MEASURE
                  Measure* measure = static_cast<Measure*>(m);
//TODOXX                  measure->layout0();
//TODO            measure->layoutBeams1();

                  measure->layoutX(1.0, true);
                  ww      = measure->layoutWidth().stretchable;
                  qreal stretch = measure->userStretch() * styleD(ST_measureSpacing);

                  ww *= stretch;
                  if (ww < point(styleS(ST_minMeasureWidth)))
                        ww = point(styleS(ST_minMeasureWidth));
                  }
            minWidth += ww;
            }
      if (minWidth > sysWidth)            // measure does not fit: do full layout
            return false;

      //
      // check if another measure will fit into system
      //
      m = m->next();
      if (m && m->type() == MEASURE) {
            Measure* measure = static_cast<Measure*>(m);
            measure->layoutX(1.0, true);
            qreal ww      = measure->layoutWidth().stretchable;
            qreal stretch = measure->userStretch() * styleD(ST_measureSpacing);

            ww *= stretch;
            if (ww < point(styleS(ST_minMeasureWidth)))
                  ww = point(styleS(ST_minMeasureWidth));
            if ((minWidth + ww) <= sysWidth)    // if another measure fits, do full layout
                  return false;
            }
      //
      // stretch measures
      //
      minWidth    = system->leftMargin();
      qreal totalWeight = 0.0;
      foreach (MeasureBase* mb, system->measures()) {
            if (mb->type() == HBOX)
                  minWidth += point(static_cast<Box*>(mb)->boxWidth());
            else {
                  Measure* m   = static_cast<Measure*>(mb);
                  minWidth    += m->layoutWidth().stretchable;
                  totalWeight += m->ticks() * m->userStretch();
                  }
            }

      qreal rest = (sysWidth - minWidth) / totalWeight;

      bool firstMeasure = true;
      QPointF pos;
      foreach(MeasureBase* mb, system->measures()) {
            qreal ww = 0.0;
            if (mb->type() == MEASURE) {
                  if (firstMeasure) {
                        pos.rx() += system->leftMargin();
                        firstMeasure = false;
                        }
                  mb->setPos(pos);
                  Measure* m    = static_cast<Measure*>(mb);
                  qreal weight = m->ticks() * m->userStretch();
                  ww            = m->layoutWidth().stretchable + rest * weight;
                  m->layout(ww);
                  }
            else if (mb->type() == HBOX) {
                  mb->setPos(pos);
                  ww = point(static_cast<Box*>(mb)->boxWidth());
                  mb->layout();
                  }
            pos.rx() += ww;
            }

      foreach(MeasureBase* mb, system->measures()) {
            if (mb->type() == MEASURE)
                  static_cast<Measure*>(mb)->layout2();
            }

      rebuildBspTree();
      return true;
      }

//---------------------------------------------------------
//   layoutFingering
//    - place numbers above a note execpt for the last
//      staff in a multi stave part (piano)
//    - does not handle chords
//---------------------------------------------------------

void Score::layoutFingering(Fingering* f)
      {
      if (f == 0)
            return;
      Note* note   = f->note();
      Chord* chord = note->chord();
      Staff* staff = chord->staff();
      Part* part   = staff->part();
      int n        = part->nstaves();
      bool below   = (n > 1) && (staff->rstaff() == n-1);

      f->layout();
      qreal x = 0.0;
      qreal y = 0.0;
      qreal headWidth = note->headWidth();
      qreal headHeight = note->headHeight();
      qreal fh = headHeight;        // TODO: fingering number height

      if (chord->notes().size() == 1) {
            x = headWidth * .5;
            if (below) {
                  // place fingering below note
                  y = fh + spatium() * .4;
                  if (chord->stem() && !chord->up()) {
                        // on stem side
                        y += chord->stem()->height();
                        x -= spatium() * .4;
                        }
                  }
            else {
                  // place fingering above note
                  y = -headHeight - spatium() * .4;
                  if (chord->stem() && chord->up()) {
                        // on stem side
                        y -= chord->stem()->height();
                        x += spatium() * .4;
                        }
                  }
            }
      f->setUserOff(QPointF(x, y));
      }

//---------------------------------------------------------
//   layoutSystemRow
//    return hight in h
//---------------------------------------------------------

QList<System*> Score::layoutSystemRow(qreal rowWidth, bool isFirstSystem, bool useLongName)
      {
      bool raggedRight = MScore::layoutDebug;

      QList<System*> sl;

      qreal ww = rowWidth;
      qreal minWidth;
      for (bool a = true; a;) {
            a = layoutSystem(minWidth, ww, isFirstSystem, useLongName);
            sl.append(_systems[curSystem]);
            ++curSystem;
            ww -= minWidth;
            }
      //
      // dont stretch last system row, if minWidth is <= lastSystemFillLimit
      //
      if (curMeasure == 0 && ((minWidth / rowWidth) <= styleD(ST_lastSystemFillLimit)))
            raggedRight = true;

      //-------------------------------------------------------
      //    Round II
      //    stretch measures
      //    "nm" measures fit on this line of score
      //-------------------------------------------------------

      bool needRelayout = false;

      foreach (System* system, sl) {
            //
            //    add cautionary time/key signatures if needed
            //

            if (system->measures().isEmpty()) {
                  qDebug("system %p is empty\n", system);
                  abort();
                  }
            Measure* m = system->lastMeasure();
            bool hasCourtesyKeysig = false;
            Measure* nm = m ? m->nextMeasure() : 0;
            Segment* s;

            if (m && nm && !(m->sectionBreak() && _layoutMode != LayoutFloat)) {
                  int tick = m->tick() + m->ticks();

                  // locate a time sig. in the next measure and, if found,
                  // check if it has cout. sig. turned off
                  TimeSig* ts;
                  Segment* tss         = nm->findSegment(SegTimeSig, tick);
                  bool showCourtesySig = tss && styleB(ST_genCourtesyTimesig);
                  if (showCourtesySig) {
                        ts = static_cast<TimeSig*>(tss->element(0));
                        if (ts && !ts->showCourtesySig())
                              showCourtesySig = false;     // this key change has court. sig turned off
                        }
                  if (showCourtesySig) {
                        // if due, create a new courtesy time signature for each staff
                        s  = m->undoGetSegment(SegTimeSigAnnounce, tick);
                        int nstaves = Score::nstaves();
                        for (int track = 0; track < nstaves * VOICES; track += VOICES) {
                              TimeSig* nts = static_cast<TimeSig*>(tss->element(track));
                              if (!nts)
                                    continue;
                              ts = static_cast<TimeSig*>(s->element(track));
                              if (ts == 0) {
                                    ts = new TimeSig(this);
                                    ts->setTrack(track);
                                    ts->setGenerated(true);
                                    ts->setMag(ts->staff()->mag());
                                    ts->setParent(s);
                                    undoAddElement(ts);
                                    }
                              ts->setFrom(nts);
                              m->setDirty(true);
                              }
                        }

                  // courtesy key signatures
                  int n = _staves.size();
                  for (int staffIdx = 0; staffIdx < n; ++staffIdx) {
                        int track = staffIdx * VOICES;
                        Staff* staff = _staves[staffIdx];
                        showCourtesySig = false;

                        KeySigEvent key1 = staff->key(tick - 1);
                        KeySigEvent key2 = staff->key(tick);
                        if (styleB(ST_genCourtesyKeysig) && (key1 != key2)) {
                              // locate a key sig. in next measure and, if found,
                              // check if it has court. sig turned off
                              s = nm->findSegment(SegKeySig, tick);
                              showCourtesySig = true;	// assume this key change has court. sig turned on
                              if (s) {
                                    KeySig* ks = static_cast<KeySig*>(s->element(track));
                                    if (ks && !ks->showCourtesySig())
                                          showCourtesySig = false;     // this key change has court. sig turned off
                                    }

                              if (showCourtesySig) {
                                    hasCourtesyKeysig = true;
                                    s  = m->undoGetSegment(SegKeySigAnnounce, tick);
                                    KeySig* ks = static_cast<KeySig*>(s->element(track));
                                    KeySigEvent ksv(key2);
                                    ksv.setNaturalType(key1.accidentalType());

                                    if (!ks) {
                                          ks = new KeySig(this);
                                          ks->setKeySigEvent(ksv);
                                          ks->setTrack(track);
                                          ks->setGenerated(true);
                                          ks->setMag(staff->mag());
                                          ks->setParent(s);
                                          undoAddElement(ks);
                                          }
                                    else if (ks->keySigEvent() != ksv) {
                                          undo(new ChangeKeySig(ks, ksv,
                                             ks->showCourtesySig(), ks->showNaturals()));
                                          }
                                    // change bar line to qreal bar line
                                    m->setEndBarLineType(DOUBLE_BAR, true);
                                    }
                              }
                        if (!showCourtesySig) {
                              // remove any existent courtesy key signature
                              Segment* s = m->findSegment(SegKeySigAnnounce, tick);
                              if (s && s->element(track))
                                    undoRemoveElement(s->element(track));
                              }
                        }

                  // courtesy clefs
                  // no courtesy clef if this measure is the end of a repeat

                  if (styleB(ST_genCourtesyClef) && !(m->repeatFlags() & RepeatEnd)) {
                        Clef* c;
                        int n = _staves.size();
                        for (int staffIdx = 0; staffIdx < n; ++staffIdx) {
                              Staff* staff = _staves[staffIdx];
                              ClefType c1 = staff->clef(tick - 1);
                              ClefType c2 = staff->clef(tick);
                              if (c1 != c2) {
                                    // locate a clef in next measure and, if found,
                                    // check if it has court. sig turned off
                                    s = nm->findSegment(SegClef, tick);
                                    showCourtesySig = true;	// assume this clef change has court. sig turned on
                                    if (s) {
                                          c = static_cast<Clef*>(s->element(staffIdx*VOICES));
                                          if (c && !c->showCourtesyClef())
                                                continue;   // this key change has court. sig turned off
                                          }

                                    s = m->undoGetSegment(SegClef, tick);
                                    int track = staffIdx * VOICES;
                                    if (!s->element(track)) {
                                          c = new Clef(this);
                                          c->setClefType(c2);
                                          c->setTrack(track);
                                          c->setGenerated(true);
                                          c->setSmall(true);
                                          c->setMag(staff->mag());
                                          c->setParent(s);
                                          undoAddElement(c);
                                          }
                                    }
                              }
                        }
                  }

            const QList<MeasureBase*>& ml = system->measures();
            int n                         = ml.size();
            while (n > 0) {
                  if (ml[n-1]->type() == MEASURE)
                        break;
                  --n;
                  }

            //
            //    compute repeat bar lines
            //
            bool firstMeasure = true;
            MeasureBase* lmb = ml.back();
            if (lmb->type() == MEASURE) {
                  if (static_cast<Measure*>(lmb)->multiMeasure() > 0) {
                        for (;;lmb = lmb->next()) {
                              if (lmb->next() == 0)
                                    break;
                              if ((lmb->next()->type() == MEASURE) && ((Measure*)(lmb->next()))->multiMeasure() >= 0)
                                    break;
                              }
                        }
                  }
            for (MeasureBase* mb = ml.front(); mb; mb = mb->next()) {
                  if (mb->type() != MEASURE) {
                        if (mb == lmb)
                              break;
                        continue;
                        }
                  Measure* m = static_cast<Measure*>(mb);
                  // first measure repeat?
                  bool fmr = firstMeasure && (m->repeatFlags() & RepeatStart);

                  if (mb == ml.back()) {       // last measure in system?
                        //
                        // if last bar has a courtesy key signature,
                        // create a double bar line as end bar line
                        //
                        BarLineType bl = hasCourtesyKeysig ? DOUBLE_BAR : NORMAL_BAR;

                        if (m->repeatFlags() & RepeatEnd)
                              m->setEndBarLineType(END_REPEAT, true);
                        else if (m->endBarLineGenerated())
                              m->setEndBarLineType(bl, true);
                        if (m->setStartRepeatBarLine(fmr))
                              m->setDirty(true);
                        }
                  else {
                        MeasureBase* mb = m->next();
                        while (mb && mb->type() != MEASURE && (mb != ml.back()))
                              mb = mb->next();

                        Measure* nm = 0;
                        if (mb && mb->type() == MEASURE)
                              nm = static_cast<Measure*>(mb);

                        needRelayout |= m->setStartRepeatBarLine(fmr);
                        if (m->repeatFlags() & RepeatEnd) {
                              if (nm && (nm->repeatFlags() & RepeatStart))
                                    m->setEndBarLineType(END_START_REPEAT, true);
                              else
                                    m->setEndBarLineType(END_REPEAT, true);
                              }
                        else if (nm && (nm->repeatFlags() & RepeatStart))
                              m->setEndBarLineType(START_REPEAT, true);
                        else if (m->endBarLineGenerated())
                              m->setEndBarLineType(NORMAL_BAR, true);
                        }
                  if (m->createEndBarLines())
                        m->setDirty(true);
                  firstMeasure = false;
                  if (mb == lmb)
                        break;
                  }

            foreach (MeasureBase* mb, ml) {
                  if (mb->type() != MEASURE)
                        continue;
                  Measure* m = static_cast<Measure*>(mb);
                  int nn = m->multiMeasure() - 1;
                  if (nn > 0) {
                        // skip to last rest measure of multi measure rest
                        Measure* mm = m;
                        for (int k = 0; k < nn; ++k)
                              mm = mm->nextMeasure();
                        if (mm) {
                              m->setMmEndBarLineType(mm->endBarLineType());
                              if (m->createEndBarLines())
                                    m->setDirty();
                              }
                        }
                  }
            }

      minWidth          = 0.0;
      qreal totalWeight = 0.0;

      foreach(System* system, sl) {
            foreach (MeasureBase* mb, system->measures()) {
                  if (mb->type() == HBOX)
                        minWidth += point(((Box*)mb)->boxWidth());
                  else if (mb->type() == MEASURE) {
                        Measure* m = (Measure*)mb;
                        if (needRelayout || m->dirty()) {
                              m->setDirty(true);
                              m->layoutX(1.0, true);
                              }
                        minWidth    += m->layoutWidth().stretchable;
                        totalWeight += m->ticks() * m->userStretch();
                        }
                  }
            minWidth += system->leftMargin();
            }

      // qreal rest = (raggedRight ? 0.0 : rowWidth - minWidth) / totalWeight;
      qreal rest = rowWidth - minWidth;
      // rest = (raggedRight ? 0.0 : rest) / totalWeight;
      // stretch incomplete row
      if (raggedRight) {
            if (minWidth > rest)
                  rest = rest * .5;
            else
                  rest = minWidth;
            }
      rest /= totalWeight;
      qreal xx   = 0.0;
      qreal y    = 0.0;

      foreach(System* system, sl) {
            QPointF pos;

            bool firstMeasure = true;
            foreach(MeasureBase* mb, system->measures()) {
                  qreal ww = 0.0;
                  if (mb->type() == MEASURE) {
                        if (firstMeasure) {
                              pos.rx() += system->leftMargin();
                              firstMeasure = false;
                              }
                        mb->setPos(pos);
                        Measure* m    = static_cast<Measure*>(mb);
                        if (styleB(ST_FixMeasureWidth)) {
                              ww = rowWidth / system->measures().size();
                              }
                        else {
                              qreal weight = m->ticks() * m->userStretch();
                              ww            = m->layoutWidth().stretchable + rest * weight;
                              }
                        m->layout(ww);
                        }
                  else if (mb->type() == HBOX) {
                        mb->setPos(pos);
                        ww = point(static_cast<Box*>(mb)->boxWidth());
                        mb->layout();
                        }
                  pos.rx() += ww;
                  }
            system->setPos(xx, y);
            qreal w = pos.x();
            system->setWidth(w);
            system->layout2();
            foreach(MeasureBase* mb, system->measures()) {
                  if (mb->type() == HBOX) {
                        mb->setHeight(system->height());
                        }
                  }
            xx += w;
            }
      return sl;
      }

//---------------------------------------------------------
//   layoutSystems
//   create list of systems
//---------------------------------------------------------

void Score::layoutSystems()
      {
      curMeasure              = first();
      curSystem               = 0;
      bool firstSystem        = true;
      bool startWithLongNames = true;

      qreal w  = pageFormat()->printableWidth() * DPI;

      while (curMeasure) {
            ElementType t = curMeasure->type();
            if (t == VBOX || t == TBOX || t == FBOX) {
                  System* system = getNextSystem(false, true);
                  foreach(SysStaff* ss, *system->staves())
                        delete ss;
                  system->staves()->clear();
                  system->setWidth(w);
                  VBox* vbox = static_cast<VBox*>(curMeasure);
                  vbox->setParent(system);
                  vbox->layout();
                  system->setHeight(vbox->height());
                  system->rxpos() = 0.0;
                  system->setPageBreak(vbox->pageBreak());
                  system->measures().push_back(vbox);
                  curMeasure = curMeasure->next();
                  ++curSystem;
                  }
            else {
                  QList<System*> sl  = layoutSystemRow(w, firstSystem, startWithLongNames);
                  for (int i = 0; i < sl.size(); ++i)
                        sl[i]->setSameLine(i != 0);
                  firstSystem = false;
                  startWithLongNames = false;
                  if (!sl.isEmpty()) {
                        Measure* lm = sl.back()->lastMeasure();
                        firstSystem = lm && lm->sectionBreak() && _layoutMode != LayoutFloat;
                        startWithLongNames = firstSystem && lm->sectionBreak()->startWithLongNames();
                        }
                  else
                        qDebug("empty system!\n");
                  }
            }
      // TODO: make undoable:
      while (_systems.size() > curSystem)
            _systems.takeLast();
      }

//---------------------------------------------------------
//   getEmptyPage
//---------------------------------------------------------

Page* Score::getEmptyPage()
      {
      Page* page = curPage >= _pages.size() ? addPage() : _pages[curPage];
      page->setNo(curPage);
      page->layout();
      qreal x = (curPage == 0) ? 0.0 : _pages[curPage - 1]->pos().x()
         + page->width() + ((curPage & 1) ? 50.0 : 1.0);
      ++curPage;
      page->setPos(x, 0.0);
      page->systems()->clear();
      return page;
      }

//---------------------------------------------------------
//   SystemRow
//---------------------------------------------------------

struct SystemRow {
      QList<System*> systems;

      qreal height() const {
            qreal h = 0.0;
            foreach(System* s, systems) {
                  if (s->height() > h)
                        h = s->height();
                  }
            return h;
            }
      bool isVbox() const {
            return systems.back()->isVbox();
            }
      VBox* vbox() const {
            return systems.back()->vbox();
            }
      bool pageBreak() const {
            return systems.back()->pageBreak();
            }
      qreal tm() const {
            qreal v = 0.0;
            foreach(System* s, systems) {
                  if (!s->isVbox())
                        v = qMax(s->distanceUp(0), v);
                  else
                        qDebug("distance for vbox? n %d\n", systems.size());
                  }
            return v;
            }
      qreal bm() const {
            qreal v = 0.0;
            foreach(System* s, systems) {
                  if (!s->isVbox())
                        v = qMax(s->distanceDown(s->staves()->size() - 1), v);
                  else
                        qDebug("distance for vbox? n %d\n", systems.size());
                  }
            return v;
            }
      };

//---------------------------------------------------------
//   layoutPages
//    create list of pages
//---------------------------------------------------------

void Score::layoutPages()
      {
      const qreal _spatium            = spatium();
      const qreal slb                 = styleS(ST_staffLowerBorder).val()    * _spatium;
      const qreal sub                 = styleS(ST_staffUpperBorder).val()    * _spatium;
      const qreal systemDist          = styleS(ST_systemDistance).val()      * _spatium;
      const qreal systemFrameDistance = styleS(ST_systemFrameDistance).val() * _spatium;
      const qreal frameSystemDistance = styleS(ST_frameSystemDistance).val() * _spatium;

      curPage            = 0;
      Page* page         = getEmptyPage();
      qreal ey           = page->height() - page->bm();
      qreal y            = page->tm();
      int nSystems       = _systems.size();
      System* lastSystem = 0;
      int gaps           = 0;

      qreal prevDist     = .0;

      for (int i = 0; i < nSystems; ++i) {
            //
            // collect system row
            //
            SystemRow sr;
            for (;;) {
                  System* system = _systems[i];
                  sr.systems.append(system);
                  if (i+1 == nSystems)
                        break;
                  if (!_systems[i+1]->sameLine())
                        break;
                  ++i;
                  }

            qreal h  = sr.height();
            qreal tmargin = 0.0;    // top system margin
            qreal bmargin;          // bottom system margin

            if (sr.isVbox()) {
                  VBox* vbox = sr.vbox();
                  bmargin  = vbox->bottomGap();
                  tmargin += vbox->topGap();
                  if (lastSystem) {
                       if (lastSystem->isVbox()) {
                              tmargin += lastSystem->vbox()->bottomGap();
                              }
                        else {
                              tmargin += systemFrameDistance;
                              }
                        }
                  }
            else {
                  if (lastSystem) {
                        if (lastSystem->isVbox())
                              tmargin = lastSystem->vbox()->bottomGap() + frameSystemDistance;
                        else
                              tmargin = qMax(sr.tm(), systemDist);
                        }
                  else {
                        tmargin = qMax(sr.tm(), sub);
                        }
                  bmargin = qMax(sr.bm(), slb);
                  }

            tmargin = qMax(tmargin, prevDist);
            prevDist = bmargin;

            if (lastSystem && (y + h + bmargin + tmargin > ey)) {
                  //
                  // prepare next page
                  //
                  qreal d;
                  if (lastSystem->isVbox())
                        d = lastSystem->vbox()->bottomGap();
                  else
                        d = slb;
                  layoutPage(page, gaps, ey - y - d);
                  page = getEmptyPage();
                  ey   = page->height() - page->bm();
                  gaps = 0;
                  y    = page->tm();
                  if (sr.isVbox())
                        tmargin = sr.vbox()->topGap();
                  else
                        tmargin = qMax(sr.tm(), sub);
                  lastSystem = 0;
                  }

            qreal x = page->lm();
            foreach(System* system, sr.systems) {
                  system->setPos(x, y + tmargin);
                  x += system->width();
                  page->appendSystem(system);
                  system->setAddStretch(false);
                  }

            if (lastSystem) {
                  bool addStretch = !lastSystem->isVbox() && !sr.isVbox();
                  lastSystem->setAddStretch(addStretch);
                  if (addStretch)
                        ++gaps;
                  }

            y += (h + tmargin);
            if (sr.pageBreak() && (_layoutMode != LayoutFloat)) {
                  qreal d;
                  if (sr.isVbox())
                        d = sr.vbox()->bottomGap();
                  else
                        d = slb;
                  layoutPage(page, gaps, ey - y - d);
                  if ((i + 1) == nSystems) {
                        page = 0;
                        break;
                        }
                  page       = getEmptyPage();
                  ey         = page->height() - page->bm();
                  gaps       = 0;
                  y          = page->tm();
                  lastSystem = 0;
                  prevDist   = 0;
                  }
            else {
                  lastSystem = sr.systems.back();
                  }
            }
      if (page) {
            qreal d;
            if (lastSystem->isVbox())
                  d = lastSystem->vbox()->bottomGap();
            else
                  d = slb;
            layoutPage(page, gaps, ey - y - d);
            }

      // TODO: make undoable:
      while (_pages.size() > curPage)
            _pages.takeLast();
      }

//---------------------------------------------------------
//   layoutPage
//    if remaining y space on page is greater (pageHeight*pageFillLimit)
//    then increase system distance to fill page
//---------------------------------------------------------

void Score::layoutPage(Page* page, int gaps, qreal restHeight)
      {
      qreal ph = loHeight() - page->bm() - page->tm();

      if (!gaps && (_layoutMode == LayoutFloat)) {
            qreal y = restHeight * .5;
            int n = page->systems()->size();
            for (int i = 0; i < n; ++i) {
                  System* system = page->systems()->at(i);
                  system->move(0, y);
                  }
            return;
            }

      if (!gaps || (restHeight > (ph * (1.0 - styleD(ST_pageFillLimit)))))
            return;

      qreal extraDist = restHeight / gaps;
      qreal y = 0;
      int n = page->systems()->size();
      for (int i = 0; i < n; ++i) {
            System* system = page->systems()->at(i);
            system->move(0, y);
            if (system->addStretch())
                  y += extraDist;
            }
      }

//---------------------------------------------------------
//   doLayoutSystems
//    layout staves in a system
//    layout pages
//---------------------------------------------------------

void Score::doLayoutSystems()
      {
      /*--*/ {
            QWriteLocker locker(&_layoutLock);
            foreach(System* system, _systems)
                  system->layout2();
            layoutPages();
            rebuildBspTree();
            _updateAll = true;
            }
      foreach(MuseScoreView* v, viewer)
            v->layoutChanged();
      }

//---------------------------------------------------------
//   doLayoutPages
//    small wrapper for layoutPages()
//---------------------------------------------------------

void Score::doLayoutPages()
      {
      /*--*/ {
            QWriteLocker locker(&_layoutLock);
            layoutPages();
            rebuildBspTree();
            _updateAll = true;
            }
      foreach(MuseScoreView* v, viewer)
            v->layoutChanged();
      }

//---------------------------------------------------------
//   sff
//    compute 1/Force for a given Extend
//---------------------------------------------------------

qreal sff(qreal x, qreal xMin, SpringMap& springs)
      {
      if (x <= xMin)
            return 0.0;
      iSpring i = springs.begin();
      qreal c  = i->second.stretch;
      if (c == 0.0)           //DEBUG
            c = 1.1;
      qreal f = 0.0;
      for (; i != springs.end();) {
            xMin -= i->second.fix;
            f = (x - xMin) / c;
            ++i;
            if (i == springs.end() || f <= i->first)
                  break;
            c += i->second.stretch;
            }
      return f;
      }

//---------------------------------------------------------
//   respace
//---------------------------------------------------------

void Score::respace(QList<ChordRest*>* elements)
      {
      ChordRest* cr1 = elements->front();
      ChordRest* cr2 = elements->back();
      int n          = elements->size();
      qreal x1       = cr1->segment()->pos().x();
      qreal x2       = cr2->segment()->pos().x();

      qreal width[n-1];
      int ticksList[n-1];
      int minTick = 100000;

      for (int i = 0; i < n-1; ++i) {
            ChordRest* cr  = (*elements)[i];
            ChordRest* ncr = (*elements)[i+1];
            Space space(cr->space());
            Space nspace(ncr->space());
            width[i] = space.rw() + nspace.lw();
            ticksList[i] = ncr->segment()->tick() - cr->segment()->tick();
            minTick = qMin(ticksList[i], minTick);
            }

      //---------------------------------------------------
      // compute stretches
      //---------------------------------------------------

      SpringMap springs;
      qreal minimum = 0.0;
      for (int i = 0; i < n-1; ++i) {
            qreal w   = width[i];
            int t     = ticksList[i];
            qreal str = 1.0 + .6 * log(qreal(t) / qreal(minTick)) / log(2.0);
            qreal d   = w / str;

            springs.insert(std::pair<qreal, Spring>(d, Spring(i, str, w)));
            minimum += w;
            }

      //---------------------------------------------------
      //    distribute stretch to elements
      //---------------------------------------------------

      qreal force = sff(x2 - x1, minimum, springs);
      for (iSpring i = springs.begin(); i != springs.end(); ++i) {
            qreal stretch = force * i->second.stretch;
            if (stretch < i->second.fix)
                  stretch = i->second.fix;
            width[i->second.seg] = stretch;
            }
      qreal x = x1;
      for (int i = 1; i < n-1; ++i) {
            x += width[i-1];
            ChordRest* cr = (*elements)[i];
            qreal dx = x - cr->segment()->pos().x();
//            cr->setUserXoffset(dx);
            cr->rxpos() += dx;
            }
      }

