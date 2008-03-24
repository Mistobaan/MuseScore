//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id:$
//
//  Copyright (C) 2008 Werner Schweer and others
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

#include <iostream>

// #include <QComboBox>
// #include <QSpinBox>
// #include <QVariant>

#include "chordedit.h"
#include "harmony.h"

//---------------------------------------------------------
//   ChordEdit
//---------------------------------------------------------

ChordEdit::ChordEdit(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      rootGroup = new QButtonGroup(this);
      rootGroup->addButton(rootC,   1);
      rootGroup->addButton(rootDb,  2);
      rootGroup->addButton(rootD,   3);
      rootGroup->addButton(rootEb,  4);
      rootGroup->addButton(rootE,   5);
      rootGroup->addButton(rootF,   6);
      rootGroup->addButton(rootFis, 7);
      rootGroup->addButton(rootG,   8);
      rootGroup->addButton(rootAb,  9);
      rootGroup->addButton(rootA,  10);
      rootGroup->addButton(rootBb, 11);
      rootGroup->addButton(rootB,  12);

      extensionGroup = new QButtonGroup(this);
      extensionGroup->addButton(extMaj,    2);
      extensionGroup->addButton(ext2,     15);
      extensionGroup->addButton(extMaj7,   6);
      extensionGroup->addButton(extMaj9,   7);
      extensionGroup->addButton(ext6,      5);
      extensionGroup->addButton(ext69,    14);

      extensionGroup->addButton(extm,     16);
      extensionGroup->addButton(extm7,    19);
      extensionGroup->addButton(extm6,    23);
      extensionGroup->addButton(extm9,    20);
      extensionGroup->addButton(extmMaj7, 18);
      extensionGroup->addButton(extm7b5,  32);
      extensionGroup->addButton(extdim,   33);
      extensionGroup->addButton(ext7,     64);
      extensionGroup->addButton(ext9,     70);
      extensionGroup->addButton(ext13,    65);
      extensionGroup->addButton(ext7b9,   76);
      extensionGroup->addButton(extsus,  184);
      extensionGroup->addButton(ext7Sus, 128);
      extensionGroup->addButton(extOther,  0);

      extOtherCombo->clear();
      for (int i = 0; i < 185; ++i) {           // HACK
            const char* p = Harmony::getExtensionName(i);
            if (p)
                  extOtherCombo->addItem(p, i);
            }
      connect(rootGroup, SIGNAL(buttonClicked(int)), SLOT(chordChanged()));
      connect(extensionGroup, SIGNAL(buttonClicked(int)), SLOT(chordChanged()));
      connect(extOtherCombo, SIGNAL(currentIndexChanged(int)), SLOT(chordChanged()));
      connect(bassNote, SIGNAL(currentIndexChanged(int)), SLOT(chordChanged()));
      connect(extOther, SIGNAL(toggled(bool)), SLOT(otherToggled(bool)));
      connect(addButton, SIGNAL(clicked()), SLOT(addButtonClicked()));
      connect(deleteButton, SIGNAL(clicked()), SLOT(deleteButtonClicked()));

      extOtherCombo->setEnabled(false);

      model = new QStandardItemModel(0, 3);
      model->setHeaderData(0, Qt::Horizontal, "Type");
      model->setHeaderData(1, Qt::Horizontal, "Value");
      model->setHeaderData(2, Qt::Horizontal, "Alter");

      connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                     SLOT(modelDataChanged(QModelIndex,QModelIndex)));

      degreeTable->setModel(model);
      delegate = new DegreeTabDelegate;
      degreeTable->setItemDelegate(delegate);
      degreeTable->setColumnWidth(0, 80);
      degreeTable->setColumnWidth(1, 71);
      degreeTable->setColumnWidth(2, 71);

      chordChanged();
      }

//---------------------------------------------------------
//   setRoot
//---------------------------------------------------------

void ChordEdit::setRoot(int val)
      {
      QAbstractButton* button = rootGroup->button(val);
      if (button)
            button->setChecked(true);
      else
            printf("root button %d not found\n", val);
      chordChanged();
      }

//---------------------------------------------------------
//   setExtension
//---------------------------------------------------------

void ChordEdit::setExtension(int val)
      {
      QAbstractButton* button = extensionGroup->button(val);
      if (button)
            button->setChecked(true);
      else {
            extOther->setChecked(true);
            int idx = extOtherCombo->findData(val);
            if (idx != -1)
                  extOtherCombo->setCurrentIndex(idx);
            }
      chordChanged();
      }

//---------------------------------------------------------
//   setBase
//---------------------------------------------------------

void ChordEdit::setBase(int val)
      {
      bassNote->setCurrentIndex(val);
      chordChanged();
      }

//---------------------------------------------------------
//   extension
//---------------------------------------------------------

int ChordEdit::extension()
      {
      int id = extensionGroup->checkedId();
      if (id == -1)
            return 0;
      else if (id == 0) {
            int idx = extOtherCombo->currentIndex();
            return extOtherCombo->itemData(idx).toInt();
            }
      else
            return id;
      }

//---------------------------------------------------------
//   root
//---------------------------------------------------------

int ChordEdit::root()
      {
      return rootGroup->checkedId();
      }

//---------------------------------------------------------
//   base
//---------------------------------------------------------

int ChordEdit::base()
      {
      return bassNote->currentIndex();
      }

//---------------------------------------------------------
//   otherToggled
//---------------------------------------------------------

void ChordEdit::otherToggled(bool val)
      {
      extOtherCombo->setEnabled(val);
      }

//---------------------------------------------------------
//   chordChanged
//---------------------------------------------------------

void ChordEdit::chordChanged()
      {
      QString s = Harmony::harmonyName(root(), extension(), base());
      chordLabel->setText(s);
      }

//---------------------------------------------------------
//   addButtonClicked
//---------------------------------------------------------

// The "add degree" button was clicked: add a new row to the model
// As value is zero, the row does not yet contain an valid degree

void ChordEdit::addButtonClicked()
      {
      int rowCount = model->rowCount();
      if (model->insertRow(model->rowCount())) {
            QModelIndex index = model->index(rowCount, 0, QModelIndex());
            model->setData(index, QVariant("add"));
            index = model->index(rowCount, 1, QModelIndex());
            model->setData(index, QVariant(0));
            index = model->index(rowCount, 2, QModelIndex());
            model->setData(index, QVariant(0));
            }
      }

//---------------------------------------------------------
//   deleteButtonClicked
//---------------------------------------------------------

// The "delete degree" button was clicked: delete the current row from the model

void ChordEdit::deleteButtonClicked()
      {
      if (degreeTable->currentIndex().isValid()) {
            model->removeRow(degreeTable->currentIndex().row());
            }
      }

//---------------------------------------------------------
//   modelDataChanged
//---------------------------------------------------------

// Call-back, called when the model was changed. Happens three times in a row
// when a new degree is added and once when an individual cell is changed.
// Debug only. Not used for data handling, as degree() directly reads from the model.

void ChordEdit::modelDataChanged(const QModelIndex & /* topLeft */, const QModelIndex & /* bottomRight */)
      {
/*
      std::cout << "ChordEdit::modelDataChanged()" << std::endl;
      for (int row = 0; row < model->rowCount(); ++row) {
            for (int column = 0; column < model->columnCount(); ++column) {
                  QModelIndex index = model->index(row, column, QModelIndex());
                  if (index.isValid()) {
                        std::cout << "r=" << row << " c=" << column << " ";
                        if (column == 0)
                              std::cout << qPrintable(index.data().toString());
                        else
                              std::cout << index.data().toInt();
                        std::cout << std::endl;
                        }
                  }
            }
*/
      }

//---------------------------------------------------------
//   addDegree
//---------------------------------------------------------

// Add degree d to the chord

void ChordEdit::addDegree(HDegree d)
      {
      if (model->insertRow(model->rowCount())
          && (d.type() == ADD || d.type() == ALTER || d.type() == SUBTRACT)) {
            int rowCount = model->rowCount();
            QModelIndex index = model->index(rowCount - 1, 0, QModelIndex());
            switch (d.type()) {
                  case ADD:      model->setData(index, QVariant("add"));      break;
                  case ALTER:    model->setData(index, QVariant("alter"));    break;
                  case SUBTRACT: model->setData(index, QVariant("subtract")); break;
                  default:       /* do nothing */                             break;
                  }
            index = model->index(rowCount - 1, 1, QModelIndex());
            model->setData(index, QVariant(d.value()));
            index = model->index(rowCount - 1, 2, QModelIndex());
            model->setData(index, QVariant(d.alter()));
            }
      }

//---------------------------------------------------------
//   isValidDegree
//---------------------------------------------------------

// determine if row r in the model contains a valid degree
// all degrees with value > 0 are considered valid
// notes:
// - addDegree() and the "type" delegate make sure the type is always valid
// - alter is considered "don't care" here and ignored

bool ChordEdit::isValidDegree(int r)
      {
      QModelIndex index = model->index(r, 1, QModelIndex());
      if (index.isValid()) {
            if (index.data().toInt() > 0)
                  return true;
            }
      return false;
      }

//---------------------------------------------------------
//   numberOfDegrees
//---------------------------------------------------------

// return number of valid degrees in the model
// note: may be lower than the number of rows

int ChordEdit::numberOfDegrees()
      {
      int count = 0;
      for (int row = 0; row < model->rowCount(); ++row) {
            if (isValidDegree(row)) {
                  count++;
            }
      }
      return count;
      }

//---------------------------------------------------------
//   degree
//---------------------------------------------------------

// return valid degree i, where i = 0 corresponds to the first one
// note: must skips rows with invalid data

HDegree ChordEdit::degree(int i)
      {
      int count = -1;
      for (int row = 0; row < model->rowCount(); ++row) {
            if (isValidDegree(row)) {
                  count++;
                  if (count == i) {
                        QModelIndex index = model->index(row, 0, QModelIndex());
                        QString strType = index.data().toString();
                        int iType = 0;
                        if (strType == "add")           iType = ADD;
                        else if (strType == "alter")    iType = ALTER;
                        else if (strType == "subtract") iType = SUBTRACT;
                        index = model->index(row, 1, QModelIndex());
                        int value = index.data().toInt();
                        index = model->index(row, 2, QModelIndex());
                        int alter = index.data().toInt();
                        return HDegree(value, alter, iType);
                        }
                  }
            }
      return HDegree();
      }

//---------------------------------------------------------
//   DegreeTabDelegate constructor
//---------------------------------------------------------

DegreeTabDelegate::DegreeTabDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}

//---------------------------------------------------------
//   createEditor
//---------------------------------------------------------

// Create a combobox with add, alter and subtract as editor for column 0,
// a spinbox for all others

QWidget *DegreeTabDelegate::createEditor(QWidget *parent,
    const QStyleOptionViewItem &/* option */,
    const QModelIndex & index) const
{
    if (index.column() == 0) {
        QComboBox *editor = new QComboBox(parent);
        editor->insertItem(0, "add");
        editor->insertItem(1, "alter");
        editor->insertItem(2, "subtract");
        return editor;
    } else if (index.column() == 1) {
        QSpinBox *editor = new QSpinBox(parent);
        editor->setMinimum(1);
        editor->setMaximum(13);
        return editor;
    } else {
        QSpinBox *editor = new QSpinBox(parent);
        editor->setMinimum(-2);
        editor->setMaximum( 2);
        return editor;
    }
    return 0;
}

//---------------------------------------------------------
//   setEditorData
//---------------------------------------------------------

void DegreeTabDelegate::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
    int value = index.model()->data(index, Qt::DisplayRole).toInt();

    if (index.column() == 0) {
        QComboBox *spinBox = static_cast<QComboBox*>(editor);
        spinBox->setCurrentIndex(value);
    } else {
        QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
        spinBox->setValue(value);
    }
}

//---------------------------------------------------------
//   setModelData
//---------------------------------------------------------

void DegreeTabDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    if (index.column() == 0) {
        QComboBox *spinBox = static_cast<QComboBox*>(editor);
        model->setData(index, spinBox->currentText());
    } else {
        QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
        spinBox->interpretText();
        model->setData(index, spinBox->value());
    }
}

//---------------------------------------------------------
//   updateEditorGeometry
//---------------------------------------------------------

void DegreeTabDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}
