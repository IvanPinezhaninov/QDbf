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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAbstractButton;
QT_END_NAMESPACE


namespace Example {
namespace Internal {
class MainWindowPrivate;
} // namespace Internal

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void openFile();

private slots:
    void openArgFile();
    void processSelectionChanged();
    void processButtonClicked(QAbstractButton *button);
    void setLastUpdate();

protected:
    void showEvent(QShowEvent *event) final;

private:
    const std::unique_ptr<Internal::MainWindowPrivate> d;

    friend class Internal::MainWindowPrivate;
};

} // namespace Example

#endif // MAINWINDOW_H
