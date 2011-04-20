//=============================================================================
//  MuseScore
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

#include "config.h"
#include "mscore.h"
#include "instrdialog.h"
#include "instrtemplate.h"
#include "scoreview.h"
#include "score.h"
#include "system.h"
#include "clef.h"
#include "undo.h"
#include "staff.h"
#include "part.h"
#include "segment.h"
#include "style.h"
#include "editinstrument.h"
#include "drumset.h"
#include "slur.h"
#include "seq.h"
#include "measure.h"
#include "line.h"
#include "beam.h"
#include "excerpt.h"
#include "stafftype.h"

//---------------------------------------------------------
//   StaffListItem
//---------------------------------------------------------

StaffListItem::StaffListItem(PartListItem* li)
   : QTreeWidgetItem(li, STAFF_LIST_ITEM)
      {
      op       = ITEM_KEEP;
      staff    = 0;
      setPartIdx(0);
      staffIdx = 0;
      setLinked(false);
      setClef(CLEF_G);
      setFlags(flags() | Qt::ItemIsUserCheckable);
      }

StaffListItem::StaffListItem()
   : QTreeWidgetItem(STAFF_LIST_ITEM)
      {
      op       = ITEM_KEEP;
      staff    = 0;
      setPartIdx(0);
      staffIdx = 0;
      setClef(CLEF_G);
      setLinked(false);
      }

//---------------------------------------------------------
//   setPartIdx
//---------------------------------------------------------

void StaffListItem::setPartIdx(int val)
      {
      _partIdx = val;
      setText(0, InstrumentsDialog::tr("Staff %1").arg(_partIdx + 1));
      }

//---------------------------------------------------------
//   setClef
//---------------------------------------------------------

void StaffListItem::setClef(ClefType val)
      {
      _clef = val;
      setText(2, qApp->translate("clefTable", clefTable[_clef].name));
      }

//---------------------------------------------------------
//   setLinked
//---------------------------------------------------------

void StaffListItem::setLinked(bool val)
      {
      _linked = val;
      setText(3, _linked ? InstrumentsDialog::tr("linked") : "");
      }

//---------------------------------------------------------
//   setVisible
//---------------------------------------------------------

void StaffListItem::setVisible(bool val)
      {
      setCheckState(1, val ? Qt::Checked : Qt::Unchecked);
      }

//---------------------------------------------------------
//   visible
//---------------------------------------------------------

bool StaffListItem::visible() const
      {
      return checkState(1) == Qt::Checked;
      }

//---------------------------------------------------------
//   PartListItem
//---------------------------------------------------------

PartListItem::PartListItem(Part* p, QTreeWidget* lv)
   : QTreeWidgetItem(lv, PART_LIST_ITEM)
      {
      part = p;
      it   = 0;
      op   = ITEM_KEEP;
      setText(0, p->trackName());
      }

PartListItem::PartListItem(const InstrumentTemplate* i, QTreeWidget* lv)
   : QTreeWidgetItem(lv, PART_LIST_ITEM)
      {
      part = 0;
      it   = i;
      op   = ITEM_ADD;
      setText(0, it->trackName);
      }

//---------------------------------------------------------
//   InstrumentTemplateListItem
//---------------------------------------------------------

InstrumentTemplateListItem::InstrumentTemplateListItem(QString group, QTreeWidget* parent)
   : QTreeWidgetItem(parent) {
      _instrumentTemplate = 0;
      _group = group;
      setText(0, group);
      }

InstrumentTemplateListItem::InstrumentTemplateListItem(InstrumentTemplate* i, InstrumentTemplateListItem* item)
   : QTreeWidgetItem(item) {
      _instrumentTemplate = i;
      setText(0, i->trackName);
      }

InstrumentTemplateListItem::InstrumentTemplateListItem(InstrumentTemplate* i, QTreeWidget* parent)
   : QTreeWidgetItem(parent) {
      _instrumentTemplate = i;
      setText(0, i->trackName);
      }

//---------------------------------------------------------
//   text
//---------------------------------------------------------

QString InstrumentTemplateListItem::text(int col) const
      {
      switch (col) {
            case 0:
                  return _instrumentTemplate ?
                     _instrumentTemplate->trackName : _group;
            default:
                  return QString("");
            }
      }

//---------------------------------------------------------
//   InstrumentsDialog
//---------------------------------------------------------

InstrumentsDialog::InstrumentsDialog(QWidget* parent)
   : QDialog(parent)
      {
      editInstrument = 0;
      setupUi(this);
      cs = 0;

      QAction* a = getAction("instruments");
      connect(a, SIGNAL(triggered()), SLOT(reject()));
      addAction(a);

      instrumentList->setSelectionMode(QAbstractItemView::SingleSelection);
      partiturList->setSelectionMode(QAbstractItemView::SingleSelection);

      buildTemplateList();

      addButton->setEnabled(false);
      removeButton->setEnabled(false);
      upButton->setEnabled(false);
      downButton->setEnabled(false);
      belowButton->setEnabled(false);
      linkedButton->setEnabled(false);
      connect(showMore, SIGNAL(clicked()), SLOT(buildTemplateList()));
      }

//---------------------------------------------------------
//   buildTemplateList
//---------------------------------------------------------

void InstrumentsDialog::buildTemplateList()
      {
      populateInstrumentList(instrumentList, showMore->isChecked());
      }

//---------------------------------------------------------
//   genPartList
//---------------------------------------------------------

void InstrumentsDialog::genPartList()
      {
      partiturList->clear();

      foreach(Part* p, *cs->parts()) {
            PartListItem* pli = new PartListItem(p, partiturList);
            foreach(Staff* s, *p->staves()) {
                  StaffListItem* sli = new StaffListItem(pli);
                  sli->staff    = s;
                  sli->setPartIdx(s->rstaff());
                  sli->staffIdx = s->idx();
                  if (s->useTablature())
                        sli->setClef(CLEF_TAB);
                  else
                        sli->setClef(s->clef(0));
                  const LinkedStaves* ls = s->linkedStaves();
                  sli->setLinked(ls && !ls->isEmpty());
                  sli->setVisible(s->show());
                  }
            partiturList->setItemExpanded(pli, true);
            }
      }

//---------------------------------------------------------
//   on_instrumentList_itemSelectionChanged
//---------------------------------------------------------

void InstrumentsDialog::on_instrumentList_itemSelectionChanged()
      {
      QList<QTreeWidgetItem*> wi = instrumentList->selectedItems();
      bool flag = !wi.isEmpty();
      addButton->setEnabled(flag);
//      editButton->setEnabled(flag);
      }

//---------------------------------------------------------
//   on_partiturList_itemSelectionChanged
//---------------------------------------------------------

void InstrumentsDialog::on_partiturList_itemSelectionChanged()
      {
      QList<QTreeWidgetItem*> wi = partiturList->selectedItems();
      if (wi.isEmpty())
            return;
      QTreeWidgetItem* item = wi.front();
      bool flag = item != 0;
      removeButton->setEnabled(flag);
      upButton->setEnabled(flag);
      downButton->setEnabled(flag);
      belowButton->setEnabled(item && item->type() == STAFF_LIST_ITEM);
      linkedButton->setEnabled(item && item->type() == STAFF_LIST_ITEM);
      }

//---------------------------------------------------------
//   on_instrumentList
//---------------------------------------------------------

void InstrumentsDialog::on_instrumentList_itemDoubleClicked(QTreeWidgetItem*, int)
      {
      on_addButton_clicked();
      }

//---------------------------------------------------------
//   on_addButton_clicked
//    add instrument to partitur
//---------------------------------------------------------

void InstrumentsDialog::on_addButton_clicked()
      {
      QList<QTreeWidgetItem*> wi = instrumentList->selectedItems();
      if (wi.isEmpty())
            return;
      InstrumentTemplateListItem* item = (InstrumentTemplateListItem*)wi.front();
      const InstrumentTemplate* it     = item->instrumentTemplate();
      if (it == 0)
            return;
      PartListItem* pli                = new PartListItem(it, partiturList);
      pli->op = ITEM_ADD;

      int n = it->staves;
      for (int i = 0; i < n; ++i) {
            StaffListItem* sli = new StaffListItem(pli);
            sli->op       = ITEM_ADD;
            sli->staff    = 0;
            sli->setPartIdx(i);
            sli->staffIdx = -1;
            sli->setClef(it->clefIdx[i]);
            }
      partiturList->setItemExpanded(pli, true);
      partiturList->clearSelection();     // should not be necessary
      partiturList->setItemSelected(pli, true);
      }

//---------------------------------------------------------
//   on_removeButton_clicked
//    remove instrument from partitur
//---------------------------------------------------------

void InstrumentsDialog::on_removeButton_clicked()
      {
      QList<QTreeWidgetItem*> wi = partiturList->selectedItems();
      if (wi.isEmpty())
            return;
      QTreeWidgetItem* item = wi.front();
      QTreeWidgetItem* parent = item->parent();

      if (parent) {
            if (((StaffListItem*)item)->op == ITEM_ADD) {
                  if (parent->childCount() == 1) {
                        partiturList->takeTopLevelItem(partiturList->indexOfTopLevelItem(parent));
                        delete parent;
                        }
                  else {
                        parent->takeChild(parent->indexOfChild(item));
                        delete item;
                        }
                  }
            else {
                  ((StaffListItem*)item)->op = ITEM_DELETE;
                  partiturList->setItemHidden(item, true);
                  }
            }
      else {
            if (((PartListItem*)item)->op == ITEM_ADD)
                  delete item;
            else {
                  ((PartListItem*)item)->op = ITEM_DELETE;
                  partiturList->setItemHidden(item, true);
                  }
            }
      partiturList->clearSelection();
      }

//---------------------------------------------------------
//   on_upButton_clicked
//    move instrument up in partitur
//---------------------------------------------------------

void InstrumentsDialog::on_upButton_clicked()
      {
      QList<QTreeWidgetItem*> wi = partiturList->selectedItems();
      if (wi.isEmpty())
            return;
      QTreeWidgetItem* item = wi.front();

      if (item->type() == PART_LIST_ITEM) {
            bool isExpanded = partiturList->isItemExpanded(item);
            int idx = partiturList->indexOfTopLevelItem(item);
            if (idx) {
                  partiturList->selectionModel()->clear();
                  QTreeWidgetItem* item = partiturList->takeTopLevelItem(idx);
                  partiturList->insertTopLevelItem(idx-1, item);
                  partiturList->setItemExpanded(item, isExpanded);
                  partiturList->setItemSelected(item, true);
                  }
            }
      else {
            QTreeWidgetItem* parent = item->parent();
            int idx = parent->indexOfChild(item);
            if (idx) {
                  partiturList->selectionModel()->clear();
                  QTreeWidgetItem* item = parent->takeChild(idx);
                  parent->insertChild(idx-1, item);
                  partiturList->setItemSelected(item, true);
                  }
            else {
                  int parentIdx = partiturList->indexOfTopLevelItem(parent);
                  if (parentIdx) {
                        partiturList->selectionModel()->clear();
                        QTreeWidgetItem* item = parent->takeChild(idx);
                        QTreeWidgetItem* prevParent = partiturList->topLevelItem(parentIdx - 1);
                        prevParent->addChild(item);
                        partiturList->setItemSelected(item, true);
                        PartListItem* pli = static_cast<PartListItem*>(prevParent);
                        StaffListItem* sli = static_cast<StaffListItem*>(item);
                        int idx = pli->part->nstaves();
                        cs->undo()->push(new MoveStaff(sli->staff, pli->part, idx));
                        }
                  }
            }
      }

//---------------------------------------------------------
//   on_downButton_clicked
//    move instrument down in partitur
//---------------------------------------------------------

void InstrumentsDialog::on_downButton_clicked()
      {
      QList<QTreeWidgetItem*> wi = partiturList->selectedItems();
      if (wi.isEmpty())
            return;
      QTreeWidgetItem* item = wi.front();
      if (item->type() == PART_LIST_ITEM) {
            bool isExpanded = partiturList->isItemExpanded(item);
            int idx = partiturList->indexOfTopLevelItem(item);
            int n = partiturList->topLevelItemCount();
            if (idx < (n-1)) {
                  partiturList->selectionModel()->clear();
                  QTreeWidgetItem* item = partiturList->takeTopLevelItem(idx);
                  partiturList->insertTopLevelItem(idx+1, item);
                  partiturList->setItemExpanded(item, isExpanded);
                  partiturList->setItemSelected(item, true);
                  }
            }
      else {
            QTreeWidgetItem* parent = item->parent();
            int idx = parent->indexOfChild(item);
            int n = parent->childCount();
            if (idx < (n-1)) {
                  partiturList->selectionModel()->clear();
                  QTreeWidgetItem* item = parent->takeChild(idx);
                  parent->insertChild(idx+1, item);
                  partiturList->setItemSelected(item, true);
                  }
            else {
                  int parentIdx = partiturList->indexOfTopLevelItem(parent);
                  int n = partiturList->topLevelItemCount();
                  if (parentIdx < (n-1)) {
                        partiturList->selectionModel()->clear();
                        QTreeWidgetItem* item = parent->takeChild(idx);
                        QTreeWidgetItem* nextParent = partiturList->topLevelItem(parentIdx - 1);
                        nextParent->addChild(item);
                        partiturList->setItemSelected(item, true);
                        PartListItem* pli = static_cast<PartListItem*>(nextParent);
                        StaffListItem* sli = static_cast<StaffListItem*>(item);
                        cs->undo()->push(new MoveStaff(sli->staff, pli->part, 0));
                        }
                  }
            }
      }

//---------------------------------------------------------
//   on_editButton_clicked
//    start instrument editor for selected instrument
//---------------------------------------------------------

#if 0
void InstrumentsDialog::on_editButton_clicked()
      {
      QList<QTreeWidgetItem*> wi = instrumentList->selectedItems();
      if (wi.isEmpty())
            return;
      QTreeWidgetItem* item = wi.front();

      InstrumentTemplateListItem* ti = (InstrumentTemplateListItem*) item;
      InstrumentTemplate* tp         = ti->instrumentTemplate();
      if (tp == 0)
            return;

      if (editInstrument == 0)
            editInstrument = new EditInstrument(this);

      editInstrument->setInstrument(tp);
      editInstrument->show();
      }
#endif

//---------------------------------------------------------
//   on_belowButton_clicked
//---------------------------------------------------------

void InstrumentsDialog::on_belowButton_clicked()
      {
      QList<QTreeWidgetItem*> wi = partiturList->selectedItems();
      if (wi.isEmpty())
            return;
      QTreeWidgetItem* item = wi.front();
      if (item->type() != STAFF_LIST_ITEM)
            return;

      StaffListItem* sli  = (StaffListItem*)item;
      Staff* staff        = sli->staff;
      PartListItem* pli   = (PartListItem*)sli->parent();
      StaffListItem* nsli = new StaffListItem();
      nsli->staff         = staff;
      nsli->setClef(sli->clef());
      if (staff)
            nsli->op = ITEM_ADD;
      pli->insertChild(pli->indexOfChild(sli)+1, nsli);
      partiturList->clearSelection();     // should not be necessary
      partiturList->setItemSelected(nsli, true);
      }

//---------------------------------------------------------
//   on_linkedButton_clicked
//---------------------------------------------------------

void InstrumentsDialog::on_linkedButton_clicked()
      {
      QList<QTreeWidgetItem*> wi = partiturList->selectedItems();
      if (wi.isEmpty())
            return;
      QTreeWidgetItem* item = wi.front();
      if (item->type() != STAFF_LIST_ITEM)
            return;

      StaffListItem* sli  = (StaffListItem*)item;
      Staff* staff        = sli->staff;
      PartListItem* pli   = (PartListItem*)sli->parent();
      StaffListItem* nsli = new StaffListItem();
      nsli->staff         = staff;
      nsli->setClef(sli->clef());
      nsli->setLinked(true);
      nsli->setVisible(true);
      if (staff)
            nsli->op = ITEM_ADD;
      pli->insertChild(pli->indexOfChild(sli)+1, nsli);
      partiturList->clearSelection();     // should not be necessary
      partiturList->setItemSelected(nsli, true);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void InstrumentsDialog::accept()
      {
      done(1);
      }

//---------------------------------------------------------
//   editInstrList
//---------------------------------------------------------

void MuseScore::editInstrList()
      {
      if (cs == 0)
            return;
      if (!instrList)
            instrList = new InstrumentsDialog(this);
      else if (instrList->isVisible()) {
            instrList->done(0);
            return;
            }

      instrList->setScore(cs);
      instrList->genPartList();
      cs->startCmd();
  	cs->deselectAll();
      int rv = instrList->exec();

      if (rv == 0) {
            cs->endCmd();
            return;
            }
  	cs->inputState().setTrack(-1);
      //
      // process modified partitur list
      //

      QTreeWidget* pl = instrList->partiturList;
      Part* part   = 0;
      int staffIdx = 0;
      int rstaff   = 0;

      QTreeWidgetItem* item = 0;
      for (int idx = 0; (item = pl->topLevelItem(idx)); ++idx) {
            PartListItem* pli = static_cast<PartListItem*>(item);
            // check if the part contains any remaining staves
            // mark to remove part if not
            QTreeWidgetItem* ci = 0;
            int staves = 0;
            for (int cidx = 0; (ci = pli->child(cidx)); ++cidx) {
                  StaffListItem* sli = static_cast<StaffListItem*>(ci);
                  if (sli->op != ITEM_DELETE)
                        ++staves;
                  }
            if (staves == 0)
                  pli->op = ITEM_DELETE;
            }

      item = 0;
      for (int idx = 0; (item = pl->topLevelItem(idx)); ++idx) {
            rstaff = 0;
            PartListItem* pli = static_cast<PartListItem*>(item);
            if (pli->op == ITEM_DELETE)
                  cs->cmdRemovePart(pli->part);
            else if (pli->op == ITEM_ADD) {
                  const InstrumentTemplate* t = ((PartListItem*)item)->it;
                  part = new Part(cs);
                  part->initFromInstrTemplate(t);

                  pli->part = part;
                  QTreeWidgetItem* ci = 0;
                  rstaff = 0;
                  for (int cidx = 0; (ci = pli->child(cidx)); ++cidx) {
                        StaffListItem* sli = static_cast<StaffListItem*>(ci);
                        Staff* staff       = new Staff(cs, part, rstaff);
                        sli->staff         = staff;
                        staff->setRstaff(rstaff);

                        staff->init(t, cidx);

                        cs->undoInsertStaff(staff, staffIdx + rstaff);
                        if (sli->linked()) {
                              // TODO: link staff
                              printf("TODO: link staff\n");
                              }

                        ++rstaff;
                        }
                  part->staves()->front()->setBarLineSpan(part->nstaves());
                  cs->cmdInsertPart(part, staffIdx);
                  staffIdx += rstaff;
                  }
            else {
                  part = pli->part;
                  QTreeWidgetItem* ci = 0;
                  for (int cidx = 0; (ci = pli->child(cidx)); ++cidx) {
                        StaffListItem* sli = (StaffListItem*)ci;
                        if (sli->op == ITEM_DELETE) {
                              cs->systems()->clear();
                              Staff* staff = sli->staff;
                              int sidx = staff->idx();
                              int eidx = sidx + 1;
                              for (MeasureBase* mb = cs->measures()->first(); mb; mb = mb->next()) {
                                    if (mb->type() != MEASURE)
                                          continue;
                                    Measure* m = (Measure*)mb;
                                    m->cmdRemoveStaves(sidx, eidx);
                                    }
/*                              foreach(Beam* e, cs->beams()) {
                                    int staffIdx = e->staffIdx();
                                    if (staffIdx >= sidx && staffIdx < eidx)
                                          cs->undoRemoveElement(e);
                                    }
 */
                              cs->cmdRemoveStaff(sidx);
                              }
                        else if (sli->op == ITEM_ADD) {
                              Staff* staff = new Staff(cs, part, rstaff);
                              sli->staff   = staff;
                              staff->setRstaff(rstaff);
                              ++rstaff;

                              cs->undoInsertStaff(staff, staffIdx);

                              for (Measure* m = cs->firstMeasure(); m; m = m->nextMeasure()) {
                                    // do not create whole measure rests for linked staves
                                    m->cmdAddStaves(staffIdx, staffIdx+1, !sli->linked());
                                    }

                              cs->adjustBracketsIns(staffIdx, staffIdx+1);

                              KeySigEvent nKey = part->staff(0)->key(0);
                              staff->setKey(0, nKey);

                              if (sli->linked()) {
                                    int idx = cs->staffIdx(staff);
                                    if (idx > 0) {
                                          Staff* ostaff = cs->staff(idx - 1);
                                          staff->setStaffType(ostaff->staffType());
                                          cloneStaff(ostaff, staff);
                                          }
                                    }
                              ++staffIdx;
                              }
                        else {
                              Staff* staff = sli->staff;
                              if (sli->visible() != staff->show()) {
                                    cs->undo()->push(new ChangeStaff(staff, staff->small(), staff->invisible(),
                                       sli->visible(), staff->staffType()));
                                    }
                              ++staffIdx;
                              ++rstaff;
                              }
                        }
                  }
            }

      //
      //    sort staves
      //
      QList<Staff*> dst;
      for (int idx = 0; idx < pl->topLevelItemCount(); ++idx) {
            PartListItem* pli = (PartListItem*)pl->topLevelItem(idx);
            if (pli->op == ITEM_DELETE)
                  continue;
            QTreeWidgetItem* ci = 0;
            for (int cidx = 0; (ci = pli->child(cidx)); ++cidx) {
                  StaffListItem* sli = (StaffListItem*) ci;
                  if (sli->op == ITEM_DELETE)
                        continue;
                  dst.push_back(sli->staff);
                  }
            }

      QList<int> dl;
      foreach(Staff* staff, dst) {
            int idx = cs->staves().indexOf(staff);
            if (idx == -1)
                  printf("staff in dialog(%p) not found in score\n", staff);
            else
                  dl.push_back(idx);
            }

//      bool sort = false;
//      for (int i = 0; i < dl.size(); ++i) {
//            if (dl[i] != i) {
//                  sort = true;
//                  break;
//                  }
//            }

//      if (sort)
            cs->undo()->push(new SortStaves(cs, dl));

      //
      // check for valid barLineSpan and bracketSpan
      // in all staves
      //

      int n = cs->nstaves();
      for (int i = 0; i < n; ++i) {
            Staff* staff = cs->staff(i);
            if (staff->barLineSpan() > (n - i))
                  cs->undoChangeBarLineSpan(staff, n - i);
            QList<BracketItem> brackets = staff->brackets();
            int nn = brackets.size();
            for (int ii = 0; ii < nn; ++ii) {
                  if ((brackets[ii]._bracket != -1) && (brackets[ii]._bracketSpan > (n - i)))
                        cs->undoChangeBracketSpan(staff, ii, n - i);
                  }
            }
      //
      // there should be at least one measure
      //
      if (cs->measures()->size() == 0)
            cs->appendMeasures(1, MEASURE);

      cs->setLayoutAll(true);
      cs->endCmd();
      cs->rebuildMidiMapping();
      seq->initInstruments();
      }

//---------------------------------------------------------
//   cmdInsertPart
//    insert before staffIdx
//---------------------------------------------------------

void Score::cmdInsertPart(Part* part, int staffIdx)
      {
      undoInsertPart(part, staffIdx);

      int sidx = this->staffIdx(part);
      int eidx = sidx + part->nstaves();
      for (Measure* m = firstMeasure(); m; m = m->nextMeasure())
            m->cmdAddStaves(sidx, eidx, true);
      adjustBracketsIns(sidx, eidx);
      }

//---------------------------------------------------------
//   cmdRemovePart
//---------------------------------------------------------

void Score::cmdRemovePart(Part* part)
      {
      int sidx   = staffIdx(part);
      int n      = part->nstaves();
      int eidx   = sidx + n;

// printf("cmdRemovePart %d-%d\n", sidx, eidx);

      //
      //    adjust measures
      //
      for (Measure* m = firstMeasure(); m; m = m->nextMeasure())
            m->cmdRemoveStaves(sidx, eidx);

      for (int i = 0; i < n; ++i)
            cmdRemoveStaff(sidx);
      undoRemovePart(part, sidx);
      }

//---------------------------------------------------------
//   insertPart
//---------------------------------------------------------

void Score::insertPart(Part* part, int idx)
      {
      int staff = 0;
      for (QList<Part*>::iterator i = _parts.begin(); i != _parts.end(); ++i) {
            if (staff >= idx) {
                  _parts.insert(i, part);
                  return;
                  }
            staff += (*i)->nstaves();
            }
      _parts.push_back(part);
      }

//---------------------------------------------------------
//   removePart
//---------------------------------------------------------

void Score::removePart(Part* part)
      {
      _parts.removeAt(_parts.indexOf(part));
      }

//---------------------------------------------------------
//   insertStaff
//---------------------------------------------------------

void Score::insertStaff(Staff* staff, int idx)
      {
      _staves.insert(idx, staff);

      staff->part()->insertStaff(staff);
#if 0
      int track = idx * VOICES;
      foreach (Beam* b, _beams) {
            if (b->track() >= track)
                  b->setTrack(b->track() + VOICES);
            }
#endif
      }

//---------------------------------------------------------
//   adjustBracketsDel
//---------------------------------------------------------

void Score::adjustBracketsDel(int sidx, int eidx)
      {
      for (int staffIdx = 0; staffIdx < _staves.size(); ++staffIdx) {
            Staff* staff = _staves[staffIdx];
            for (int i = 0; i < staff->bracketLevels(); ++i) {
                  int span = staff->bracketSpan(i);
                  if ((span == 0) || ((staffIdx + span) < sidx) || (staffIdx > eidx))
                        continue;
                  if ((sidx >= staffIdx) && (eidx <= (staffIdx + span)))
                        undoChangeBracketSpan(staff, i, span - (eidx-sidx));
//                  else {
//                        printf("TODO: adjust brackets, span %d\n", span);
//                        }
                  }
            int span = staff->barLineSpan();
            if ((sidx >= staffIdx) && (eidx <= (staffIdx + span)))
                  undoChangeBarLineSpan(staff, span - (eidx-sidx));
            }
      }

//---------------------------------------------------------
//   adjustBracketsIns
//---------------------------------------------------------

void Score::adjustBracketsIns(int sidx, int eidx)
      {
      for (int staffIdx = 0; staffIdx < _staves.size(); ++staffIdx) {
            Staff* staff = _staves[staffIdx];
            int bl = staff->bracketLevels();
            for (int i = 0; i < bl; ++i) {
                  int span = staff->bracketSpan(i);
                  if ((span == 0) || ((staffIdx + span) < sidx) || (staffIdx > eidx))
                        continue;
                  if ((sidx >= staffIdx) && (eidx < (staffIdx + span)))
                        undoChangeBracketSpan(staff, i, span + (eidx-sidx));
//                  else {
//                        printf("TODO: adjust brackets\n");
//                        }
                  }
            int span = staff->barLineSpan();
            if ((sidx >= staffIdx) && (eidx < (staffIdx + span)))
                  undoChangeBarLineSpan(staff, span + (eidx-sidx));
            }
      }

//---------------------------------------------------------
//   cmdRemoveStaff
//---------------------------------------------------------

void Score::cmdRemoveStaff(int staffIdx)
      {
      adjustBracketsDel(staffIdx, staffIdx+1);
      Staff* s = staff(staffIdx);
      undoRemoveStaff(s, staffIdx);
      }

//---------------------------------------------------------
//   removeStaff
//---------------------------------------------------------

void Score::removeStaff(Staff* staff)
      {
//      int idx = staff->idx();
      _staves.removeAll(staff);
      staff->part()->removeStaff(staff);
      // int track = idx * VOICES;
/*      foreach(Beam* e, beams()) {
            if (e->track() > track)
                  e->setTrack(e->track() - VOICES);
            }
 */
      }

//---------------------------------------------------------
//   sortStaves
//---------------------------------------------------------

void Score::sortStaves(QList<int>& dst)
      {
      systems()->clear();  //??
      _parts.clear();
      Part* curPart = 0;
      QList<Staff*> dl;
      foreach(int idx, dst) {
            Staff* staff = _staves[idx];
            if (staff->part() != curPart) {
                  curPart = staff->part();
                  curPart->staves()->clear();
                  _parts.push_back(curPart);
                  }
            curPart->staves()->push_back(staff);
            dl.push_back(staff);
            }
      _staves = dl;

      for (MeasureBase* mb = _measures.first(); mb; mb = mb->next()) {
            if (mb->type() != MEASURE)
                  continue;
            Measure* m = static_cast<Measure*>(mb);
            m->sortStaves(dst);
            }

/*      foreach(Beam* beam, _beams) {
            int staffIdx = beam->staffIdx();
            int voice    = beam->voice();
            int idx      = dst.indexOf(staffIdx);
            beam->setTrack(idx * VOICES + voice);
            }
*/
#if 0 // imlementation changed
      foreach(Element* e, _gel) {
            switch(e->type()) {
                  case SLUR:
                        {
                        Slur* slur    = static_cast<Slur*>(e);
                        int staffIdx  = slur->startElement()->staffIdx();
                        int voice     = slur->startElement()->voice();
                        int staffIdx2 = slur->endElement()->staffIdx();
                        int voice2    = slur->endElement()->voice();
                        slur->setTrack(dst[staffIdx] * VOICES + voice);
                        slur->setTrack2(dst[staffIdx2] * VOICES + voice2);
                        }
                    break;
                  case VOLTA:  //volta always attached to top staff
                        break;
                  case OTTAVA:
                  case TRILL:
                  case PEDAL:
                  case HAIRPIN:
                  case TEXTLINE:
                        {
                        SLine* line = static_cast<SLine*>(e);
                        int voice    = line->voice();
                        int staffIdx = line->staffIdx();
                        int idx = dst.indexOf(staffIdx);
                        line->setTrack(idx * VOICES + voice);
                        }
                        break;
                  default:
                        break;
                }
            }
#endif
      }

//---------------------------------------------------------
//   on_saveButton_clicked
//---------------------------------------------------------

void InstrumentsDialog::on_saveButton_clicked()
      {
      QString name = QFileDialog::getSaveFileName(
         this,
         tr("MuseScore: Save Instrument List"),
         ".",
         tr("MuseScore Instruments (*.xml);;")
         );
      if (name.isEmpty())
            return;
      QString ext(".xml");
      QFileInfo info(name);

      if (info.suffix().isEmpty())
            info.setFile(info.filePath() + ext);
      QFile f(info.filePath());
      if (!f.open(QIODevice::WriteOnly)) {
            QString s = tr("Open Instruments File\n") + f.fileName() + tr("\nfailed: ")
               + QString(strerror(errno));
            QMessageBox::critical(mscore, tr("MuseScore: Open Instruments file"), s);
            return;
            }

      Xml xml(&f);
      xml.header();
      xml.stag("museScore version=\"" MSC_VERSION "\"");
      foreach(InstrumentGroup* g, instrumentGroups) {
            xml.stag(QString("InstrumentGroup name=\"%1\" extended=\"%2\"").arg(g->name).arg(g->extended));
            foreach(InstrumentTemplate* t, g->instrumentTemplates)
                  t->write(xml);
            xml.etag();
            }
      xml.etag();
      if (f.error() != QFile::NoError) {
            QString s = tr("Write Style failed: ") + f.errorString();
            QMessageBox::critical(this, tr("MuseScore: Write Style"), s);
            }
      }

//---------------------------------------------------------
//   on_loadButton_clicked
//---------------------------------------------------------

void InstrumentsDialog::on_loadButton_clicked()
      {
      QString fn = QFileDialog::getOpenFileName(
         this, tr("MuseScore: Load Instrument List"),
          mscoreGlobalShare + "/templates",
         tr("MuseScore Instruments (*.xml);;"
            "All files (*)"
            )
         );
      if (fn.isEmpty())
            return;
      QFile f(fn);
      if (!loadInstrumentTemplates(fn)) {
            QMessageBox::warning(0,
               QWidget::tr("MuseScore: load Style failed:"),
               QString(strerror(errno)),
               QString::null, QWidget::tr("Quit"), QString::null, 0, 1);
            return;
            }
      buildTemplateList();
      }

