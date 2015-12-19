// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBB_MODALVIEW_H
#define DBB_MODALVIEW_H

#include <QKeyEvent>
#include <QWidget>

#include "ui/ui_modalview.h"

class ModalView : public QWidget
{
    Q_OBJECT

public:
    explicit ModalView(QWidget* parent = 0);
    ~ModalView();
    Ui::ModalView *ui;

signals:
    void newPasswordAvailable(const QString&, bool);

public slots:
    void showOrHide(bool state = false);
    void showSetPasswordInfo(bool showCleanInfo = false);
    void showModalInfo(const QString &info, int helpType);
    void setPasswordProvided();
    void cleanse();
    void setText(const QString& text);
    void updateIcon(const QIcon& icon);
protected:
    virtual void keyPressEvent(QKeyEvent *event);
private:
    bool visible;
};


#endif // DBB_MODALVIEW_H
