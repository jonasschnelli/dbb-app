// Copyright (c) 2015 Jonas Schnelli / Shift Devices AG
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "modalview.h"

#include "dbb_gui.h"

ModalView::ModalView(QWidget* parent) : QWidget(parent), ui(new Ui::ModalView)
{
    ui->setupUi(this);

    connect(this->ui->setPassword0, SIGNAL(returnPressed()), this->ui->setPassword1, SLOT(setFocus()));
    connect(this->ui->setPassword1, SIGNAL(returnPressed()), this->ui->setPassword, SIGNAL(clicked()));
    connect(this->ui->setPassword, SIGNAL(clicked()), this, SLOT(setPasswordProvided()));

    connect(this->ui->okButton, SIGNAL(clicked()), this, SLOT(showOrHide()));
}

ModalView::~ModalView()
{
    delete ui;
}

void ModalView::setText(const QString& text)
{
    ui->modalInfoLabel->setText(text);
}

void ModalView::cleanse()
{
    ui->setPassword0->clear();
    ui->setPassword1->clear();
}

void ModalView::setPasswordProvided()
{
    if (ui->setPassword0->text() != ui->setPassword1->text())
    {
        //: translation: password not identical text
        // showAlert(tr("Error"), tr("Passwords not identical"));
        //TODO
        return;
    }

    emit newPasswordAvailable(ui->setPassword0->text(), !ui->passwordInfo->isVisible());
}

void ModalView::showOrHide(bool state)
{
    if (state)
        setGeometry(-this->width(), 0, this->width(), this->height());

    QPropertyAnimation* animation = new QPropertyAnimation(this, "pos");
    animation->setDuration(300);
    animation->setStartValue(this->pos());
    animation->setEndValue(QPoint((state ? 0 : -this->width()), 0));
    animation->setEasingCurve(QEasingCurve::OutQuad);
    // to slide in call
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void ModalView::showSetPasswordInfo(bool showCleanInfo)
{
    ui->okButton->setVisible(false);
    ui->setPasswordWidget->setVisible(true);
    ui->passwordInfo->setVisible(showCleanInfo);
    ui->modalInfoLabel->setText("");
    ui->setPassword0->setFocus();
    ui->modalIcon->setIcon(QIcon());

    showOrHide(true);
}

void ModalView::showModalInfo(const QString &info, int helpType)
{
    setVisible(true);

    ui->setPasswordWidget->setVisible(false);
    ui->okButton->setVisible(false);

    ui->modalInfoLabel->setText(info);
    QWidget* slide = this;
    // setup slide
    slide->setGeometry(-slide->width(), 0, slide->width(), slide->height());

    if (helpType == DBB_PROCESS_INFOLAYER_STYLE_TOUCHBUTTON)
    {
        QIcon newIcon;
        newIcon.addPixmap(QPixmap(":/icons/touchhelp"), QIcon::Normal);
        newIcon.addPixmap(QPixmap(":/icons/touchhelp"), QIcon::Disabled);
        ui->modalIcon->setIcon(newIcon);

        if (info.isNull() || info.size() == 0)
            ui->modalInfoLabel->setText(tr(""));
    }
    else if (helpType == DBB_PROCESS_INFOLAYER_STYLE_REPLUG)
    {
        QIcon newIcon;
        newIcon.addPixmap(QPixmap(":/icons/touchhelp_replug"), QIcon::Normal);
        newIcon.addPixmap(QPixmap(":/icons/touchhelp_replug"), QIcon::Disabled);
        ui->modalIcon->setIcon(newIcon);

        if (info.isNull() || info.size() == 0)
            ui->modalInfoLabel->setText(tr(""));
    }
    else if (helpType == DBB_PROCESS_INFOLAYER_CONFIRM_WITH_BUTTON || helpType == DBB_PROCESS_INFOLAYER_CONFIRM_WITH_BUTTON_WARNING)
    {
        if (DBB_PROCESS_INFOLAYER_CONFIRM_WITH_BUTTON_WARNING)
        {
            QIcon newIcon;
            newIcon.addPixmap(QPixmap(":/icons/modal_warning"), QIcon::Normal);
            newIcon.addPixmap(QPixmap(":/icons/modal_warning"), QIcon::Disabled);
            ui->modalIcon->setIcon(newIcon);
        }
        else
            ui->modalIcon->setIcon(QIcon());

        ui->okButton->setVisible(true);
    }
    else
    {
        ui->modalIcon->setIcon(QIcon());
    }


    // then a animation:
    QPropertyAnimation* animation = new QPropertyAnimation(slide, "pos");
    animation->setDuration(300);
    animation->setStartValue(slide->pos());
    animation->setEndValue(QPoint(0, 0));
    animation->setEasingCurve(QEasingCurve::OutQuad);
    // to slide in call
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}