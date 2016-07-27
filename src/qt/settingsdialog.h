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
    void cancel();
    void setHiddenPassword();
    
public:
    explicit SettingsDialog(QWidget *parent = 0, DBB::DBBConfigdata* configData = NULL);
    ~SettingsDialog();

    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent* event);

signals:
    void settingsDidChange();
    void settingsShouldChangeHiddenPassword(const QString&);

private:
    Ui::SettingsDialog *ui;
    void loadSettings();
    void storeSettings();
    
    bool cancleOnClose;
};

#endif //DBB_APP_SETTINGSDIALOG_H
