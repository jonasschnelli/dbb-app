// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBB_BACKUPDIALOG_H
#define DBB_BACKUPDIALOG_H

#include <QWidget>

namespace Ui {
class BackupDialog;
}

class BackupDialog : public QWidget
{
    Q_OBJECT

public:
    explicit BackupDialog(QWidget *parent = 0);
    ~BackupDialog();
    void showList(const std::vector<std::string>& elements);
    void showLoading(bool creatingBackup = false);

signals:
    void addBackup();
    void restoreFromBackup(QString filename);
    void eraseAllBackups();

public slots:
    void addBackupPressed();
    void eraseAllBackupPressed();
    void restoreBackupPressed();

private:
    Ui::BackupDialog *ui;
    bool loadingState;
};

#endif // DBB_BACKUPDIALOG_H
