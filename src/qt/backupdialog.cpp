// Copyright (c) 2015 Jonas Schnelli / Shift Devices AG
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "backupdialog.h"
#include "ui/ui_backupdialog.h"

#include <QStringListModel>
#include <QStringList>

BackupDialog::BackupDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BackupDialog)
{
    ui->setupUi(this);

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addBackupPressed()));
}

void BackupDialog::showLoading(bool creatingBackup)
{
    QList<QString> loadingList; loadingList << tr((creatingBackup) ? "creating backup..." : "loading...");
    ui->listView->setModel(new QStringListModel(loadingList));
}

void BackupDialog::showList(const std::vector<std::string>& elements)
{
    QVector<QString> stringVector;
    for (const std::string& aItem : elements) {
        stringVector.push_back(QString::fromStdString(aItem).trimmed());
    }
    ui->listView->setModel(new QStringListModel(QList<QString>::fromVector(stringVector)));
}

void BackupDialog::addBackupPressed()
{
    showLoading(true);
    emit addBackup();
}

void BackupDialog::restoreBackupPressed()
{
    emit restoreFromBackup("");
}

void BackupDialog::eraseAllBackupPressed()
{
    emit eraseAllBackups();
}

BackupDialog::~BackupDialog()
{
    delete ui;
}
