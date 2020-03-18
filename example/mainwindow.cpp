/***************************************************************************
**
** Copyright (C) 2020 Ivan Pinezhaninov <ivan.pinezhaninov@gmail.com>
**
** This file is part of the QDbf - Qt DBF library.
**
** The QDbf is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** The QDbf is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with the QDbf.  If not, see <http://www.gnu.org/licenses/>.
**
***************************************************************************/

#include <QCoreApplication>
#include <QDate>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "mainwindow.h"
#include "qdbftablemodel.h"

const int MESSAGE_TIMEOUT = 1000;

const char *const ADD_ROW_BUTTON_TOOL_TIP = "Add row to the end of the table";
const char *const ADD_ROW_ERROR_TEXT = "Add row failed";
const char *const ERROR_MESSAGE_TITLE = "Error";
const char *const FILE_LOCATION_EDITOR_TEXT = "Select dbf file by clicking on the button ----->";
const char *const FILE_OPENED_MESSAGE = "File opened";
const char *const OPEN_FILE_DIALOG_CAPTION = "Select dbf file";
const char *const OPEN_FILE_DIALOG_FILTER = "xBase files (*.dbf)";
const char *const OPEN_FILE_DIALOG_ERROR_TEXT = "Can not open file %1";
const char *const REMOVE_ROW_BUTTON_TOOL_TIP = "Remove selected row";
const char *const REMOVE_ROW_ERROR_TEXT = "Remove row failed";
const char *const ROW_ADDED_MESSAGE = "Row added";
const char *const ROW_REMOVED_MESSAGE = "Row removed";
const char *const TABLE_LAST_UPDATE_LABEL_TOOLTIP = "Table last update";


namespace Example {
namespace Internal {

class MainWindowPrivate
{
    Q_DECLARE_TR_FUNCTIONS(MainWindow)
public:
    explicit MainWindowPrivate(MainWindow *parent);

    void init();
    void openFile();
    void openFile(const QString &file);

    MainWindow *const q;
    const std::unique_ptr<QWidget> m_centralWidget;
    const std::unique_ptr<QStatusBar> m_statusBar;
    const std::unique_ptr<QLabel> m_tableLastUpdateLabel;
    const std::unique_ptr<QVBoxLayout> m_baseLayout;
    const std::unique_ptr<QHBoxLayout> m_fileLocationLayout;
    const std::unique_ptr<QLineEdit> m_fileLocationEditor;
    const std::unique_ptr<QToolButton> m_selectFileButton;
    const std::unique_ptr<QTableView> m_tableView;
    const std::unique_ptr<QDbf::QDbfTableModel> m_model;
    const std::unique_ptr<QDialogButtonBox> m_buttonBox;
    const std::unique_ptr<QToolButton> m_addRowButton;
    const std::unique_ptr<QToolButton> m_removeRowButton;
    QDir m_dir;
};


MainWindowPrivate::MainWindowPrivate(MainWindow *parent) :
    q(parent),
    m_centralWidget(new QWidget()),
    m_statusBar(new QStatusBar()),
    m_tableLastUpdateLabel(new QLabel()),
    m_baseLayout(new QVBoxLayout(m_centralWidget.get())),
    m_fileLocationLayout(new QHBoxLayout()),
    m_fileLocationEditor(new QLineEdit()),
    m_selectFileButton(new QToolButton()),
    m_tableView(new QTableView()),
    m_model(new QDbf::QDbfTableModel()),
    m_buttonBox(new QDialogButtonBox()),
    m_addRowButton(new QToolButton()),
    m_removeRowButton(new QToolButton()),
    m_dir(QCoreApplication::applicationDirPath())
{
}


void MainWindowPrivate::init()
{
    q->setCentralWidget(m_centralWidget.get());

    m_tableLastUpdateLabel->setToolTip(tr(TABLE_LAST_UPDATE_LABEL_TOOLTIP));
    m_statusBar->addPermanentWidget(m_tableLastUpdateLabel.get());
    q->setStatusBar(m_statusBar.get());

    m_baseLayout->setContentsMargins(0, 0, 0, 0);

    m_fileLocationLayout->setContentsMargins(2, 2, 2, 2);
    m_baseLayout->addLayout(m_fileLocationLayout.get());

    m_fileLocationEditor->setReadOnly(true);
    m_fileLocationEditor->setText(tr(FILE_LOCATION_EDITOR_TEXT));
    m_fileLocationLayout->addWidget(m_fileLocationEditor.get());

    m_selectFileButton->setText(QLatin1String("..."));
    q->connect(m_selectFileButton.get(), SIGNAL(clicked()), SLOT(openFile()));
    m_fileLocationLayout->addWidget(m_selectFileButton.get());

    q->connect(m_model.get(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), q, SLOT(setLastUpdate()));
    m_tableView->setModel(m_model.get());
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_baseLayout->addWidget(m_tableView.get());

    m_addRowButton->setText(QLatin1String("+"));
    m_buttonBox->addButton(m_addRowButton.get(), QDialogButtonBox::ActionRole);

    m_removeRowButton->setText(QLatin1String("-"));
    m_buttonBox->addButton(m_removeRowButton.get(), QDialogButtonBox::ActionRole);

    q->connect(m_buttonBox.get(), SIGNAL(clicked(QAbstractButton*)), SLOT(processButtonClicked(QAbstractButton*)));
    m_baseLayout->addWidget(m_buttonBox.get());

    m_addRowButton->setToolTip(tr(ADD_ROW_BUTTON_TOOL_TIP));
    m_addRowButton->setEnabled(false);

    m_removeRowButton->setToolTip(tr(REMOVE_ROW_BUTTON_TOOL_TIP));
    m_removeRowButton->setEnabled(false);

    QTimer::singleShot(100, q, SLOT(openArgFile()));
}


void MainWindowPrivate::openFile()
{
    const auto &caption = tr(OPEN_FILE_DIALOG_CAPTION);
    const auto &filter = tr(OPEN_FILE_DIALOG_FILTER);
    const auto &file = QFileDialog::getOpenFileName(q, caption, m_dir.absolutePath(), filter);

    if (!file.isEmpty()) {
        openFile(file);
    }
}


void MainWindowPrivate::openFile(const QString &file)
{
    QFileInfo fileInfo(file);
    m_dir = fileInfo.dir();

    m_tableView->setModel(nullptr);
    m_addRowButton->setEnabled(false);
    m_removeRowButton->setEnabled(false);

    if (!m_model->open(file)) {
        m_fileLocationEditor->setText(QLatin1String(FILE_LOCATION_EDITOR_TEXT));
        const auto &title = tr(ERROR_MESSAGE_TITLE);
        const auto &text = tr(OPEN_FILE_DIALOG_ERROR_TEXT).arg(file);
        QMessageBox::warning(q, title, text, QMessageBox::Ok);
    } else {
        m_tableView->setModel(m_model.get());
        m_addRowButton->setEnabled(true);
        m_fileLocationEditor->setText(file);
        m_statusBar->showMessage(tr(FILE_OPENED_MESSAGE), MESSAGE_TIMEOUT);
        q->setLastUpdate();
        q->connect(m_tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(processSelectionChanged()));
    }
}

} // namespace Internal


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    d(new Internal::MainWindowPrivate(this))
{
    d->init();
}


MainWindow::~MainWindow() = default;


void MainWindow::openFile()
{
    d->openFile();
}


void MainWindow::openArgFile()
{
    const QStringList &args = QCoreApplication::arguments();
    if (1 < args.size()) {
        d->openFile(args.last());
    }
}


void MainWindow::processSelectionChanged()
{
    Q_ASSERT(d->m_tableView && d->m_tableView->selectionModel());
    d->m_removeRowButton->setEnabled(d->m_tableView->selectionModel()->currentIndex().isValid());
}


void MainWindow::processButtonClicked(QAbstractButton *button)
{
    Q_ASSERT(button);

    const auto &errorMessageTitle = tr(ERROR_MESSAGE_TITLE);

    if (button == d->m_addRowButton.get()) {
      if (!d->m_model->insertRow(d->m_model->rowCount())) {
          QMessageBox::warning(this, errorMessageTitle, tr(ADD_ROW_ERROR_TEXT), QMessageBox::Ok);
      } else {
          d->m_statusBar->showMessage(tr(ROW_ADDED_MESSAGE), MESSAGE_TIMEOUT);
          setLastUpdate();
      }
    } else if (button == d->m_removeRowButton.get()) {
        Q_ASSERT(d->m_tableView->selectionModel()->currentIndex().isValid());
        if (!d->m_model->removeRow(d->m_tableView->selectionModel()->currentIndex().row())) {
            QMessageBox::warning(this, errorMessageTitle, tr(REMOVE_ROW_ERROR_TEXT), QMessageBox::Ok);
        } else {
            d->m_statusBar->showMessage(tr(ROW_REMOVED_MESSAGE), MESSAGE_TIMEOUT);
            setLastUpdate();
        }
    } else {
        Q_ASSERT(false);
    }
}


void MainWindow::setLastUpdate()
{
    Q_ASSERT(d->m_model);
    d->m_tableLastUpdateLabel->setText(d->m_model->lastUpdate().toString(Qt::SystemLocaleShortDate));
}


void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    static auto firstShow = true;
    if (firstShow) {
        auto sideLength = d->m_fileLocationEditor->height();
        d->m_selectFileButton->setFixedSize(sideLength, sideLength);
        d->m_addRowButton->setFixedSize(sideLength, sideLength);
        d->m_removeRowButton->setFixedSize(sideLength, sideLength);
        firstShow = false;
    }
}

} // namespace Example
