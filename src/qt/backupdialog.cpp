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

    //connect buttons to slots
    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addBackupPressed()));
    connect(ui->eraseAllButton, SIGNAL(clicked()), this, SLOT(eraseAllBackupPressed()));
    connect(ui->restoreButton, SIGNAL(clicked()), this, SLOT(restoreBackupPressed()));
}

void BackupDialog::showLoading(bool creatingBackup)
{
    //simple loading through list updating
    QList<QString> loadingList; loadingList << tr((creatingBackup) ? "creating backup..." : "loading...");
    ui->listView->setModel(new QStringListModel(loadingList));
    loadingState = true;
}

void BackupDialog::showList(const std::vector<std::string>& elements)
{
    QVector<QString> stringVector;
    for (const std::string& aItem : elements) {
        stringVector.push_back(QString::fromStdString(aItem).trimmed());
    }
    ui->listView->setModel(new QStringListModel(QList<QString>::fromVector(stringVector)));
    loadingState = false;
}

void BackupDialog::addBackupPressed()
{
    showLoading(true);
    emit addBackup();
}

void BackupDialog::restoreBackupPressed()
{
    if (loadingState)
        return;
    
    QModelIndex index = ui->listView->currentIndex();
    emit restoreFromBackup(index.data().toString());
}

void BackupDialog::eraseAllBackupPressed()
{
    emit eraseAllBackups();
}

BackupDialog::~BackupDialog()
{
    delete ui;
}
