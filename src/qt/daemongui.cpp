#include "daemongui.h"

#include <QApplication>
#include <QPushButton>
#include <QDebug>
#include "ui/ui_daemon.h"
#include <dbb.h>

DBBDaemonGui::DBBDaemonGui(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->eraseButton, SIGNAL(clicked()), this, SLOT(eraseClicked()));
    changeConnectedState(DBB::openConnection());

    setWindowTitle("The Digital Bitbox");
}

DBBDaemonGui::~DBBDaemonGui()
{

}

void DBBDaemonGui::changeConnectedState(bool state)
{
    if (state)
        ui->connected->setVisible(true);
    else
        ui->connected->setVisible(false);
}

void DBBDaemonGui::eraseClicked()
{
    std::string base64str;
    std::string cmdOut;

    DBB::encryptAndEncodeCommand("{\"reset\": \"__ERASE__\"}", "0000", base64str);
    //DBB::sendCommand(base64str, cmdOut);
    DBB::sendCommand("{\"reset\": \"__ERASE__\"}", cmdOut);


    ui->textEdit->setText(QString::fromStdString(cmdOut));
    qDebug() << "command sent";
}
