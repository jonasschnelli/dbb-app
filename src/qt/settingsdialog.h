#ifndef DBB_APP_SETTINGSDIALOG_H
#define DBB_APP_SETTINGSDIALOG_H

#include <QDialog>

#include "dbb_configdata.h"

namespace Ui {
    class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

    DBB::DBBConfigdata *configData;

public slots:
    void resetDefaults();

public:
    explicit SettingsDialog(QWidget *parent = 0, DBB::DBBConfigdata* configData = NULL);
    ~SettingsDialog();

    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent* event);

signals:
    void settingsDidChange();

private:
    Ui::SettingsDialog *ui;
    void loadSettings();
    void storeSettings();
};

#endif //DBB_APP_SETTINGSDIALOG_H
