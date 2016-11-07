#include "settingsdialog.h"
#include "ui/ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent, DBB::DBBConfigdata* configDataIn, bool deviceLocked) :
QDialog(parent), configData(configDataIn),
ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    connect(this->ui->resetDefaultsButton, SIGNAL(clicked()), this, SLOT(resetDefaults()));
    connect(this->ui->saveButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(this->ui->cancelButton, SIGNAL(clicked()), this, SLOT(cancel()));
    connect(this->ui->setHiddenPasswordButton, SIGNAL(clicked()), this, SLOT(setHiddenPassword()));

    connect(this->ui->useDefaultProxy, SIGNAL(stateChanged(int)), this, SLOT(useDefaultProxyToggled(int)));

    updateDeviceLocked(deviceLocked);
}

void SettingsDialog::updateDeviceLocked(bool deviceLocked)
{
    ui->tabResetPW->setEnabled(!deviceLocked);
    ui->setHiddenPasswordButton->setEnabled(!deviceLocked);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::resetDefaults()
{
    ui->walletServiceURL->setText(QString::fromStdString(configData->getDefaultBWSULR()));
    ui->smartVerificationURL->setText(QString::fromStdString(configData->getDefaultComServerURL()));
}

void SettingsDialog::cancel()
{
    cancleOnClose = true;
    close();
}

void SettingsDialog::showEvent(QShowEvent* event)
{
    cancleOnClose = false;
    loadSettings();
    QWidget::showEvent(event);
}

void SettingsDialog::closeEvent(QCloseEvent *event)
{
    if (!cancleOnClose)
        storeSettings();

    ui->setHiddenPasswordTextField->setText("");
    QWidget::closeEvent(event);
}

void SettingsDialog::useDefaultProxyToggled(int newState)
{
    updateVisibility();
}

void SettingsDialog::updateVisibility()
{
    bool vState = ui->useDefaultProxy->isChecked();
    ui->useDBBProxy->setVisible(vState);
    ui->lineSpacer->setVisible(vState);
    ui->Socks5ProxyLabel->setVisible(vState);
    ui->socks5ProxyURL->setVisible(vState);
}

void SettingsDialog::loadSettings()
{
    ui->walletServiceURL->setText(QString::fromStdString(configData->getBWSBackendURLInternal()));
    ui->smartVerificationURL->setText(QString::fromStdString(configData->getComServerURL()));
    ui->socks5ProxyURL->setText(QString::fromStdString(configData->getSocks5ProxyURLInternal()));
    ui->useDBBProxy->setChecked(configData->getDBBProxy());
    ui->useDefaultProxy->setChecked(configData->getUseDefaultProxy());

    updateVisibility();
}

void SettingsDialog::storeSettings()
{
    configData->setBWSBackendURL(ui->walletServiceURL->text().toStdString());
    configData->setComServerURL(ui->smartVerificationURL->text().toStdString());
    configData->setSocks5ProxyURL(ui->socks5ProxyURL->text().toStdString());
    configData->setDBBProxy(ui->useDBBProxy->isChecked());
    configData->setUseDefaultProxy(ui->useDefaultProxy->isChecked());
    configData->write();
    emit settingsDidChange();
}

void SettingsDialog::setHiddenPassword()
{
    emit settingsShouldChangeHiddenPassword(ui->setHiddenPasswordTextField->text());
    ui->setHiddenPasswordTextField->setText("");
    close();
}
