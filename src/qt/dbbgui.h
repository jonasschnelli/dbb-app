// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBBDAEMON_QT_DAEMONGUI_H
#define DBBDAEMON_QT_DAEMONGUI_H

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QPushButton>

namespace Ui
{
class MainWindow;
}

class DBBDaemonGui : public QMainWindow
{
    Q_OBJECT

public:
    explicit DBBDaemonGui(QWidget* parent = 0);
    ~DBBDaemonGui();

private:
    Ui::MainWindow* ui;
    QLabel* statusBarLabel;
    QPushButton* statusBarButton;
    bool processComnand;
    std::string sessionPassword; //TODO: needs secure space / mem locking

    bool sendCommand(const std::string& cmd, const std::string& password);

public slots:
    /** Set number of connections shown in the UI */
    void eraseClicked();
    void ledClicked();
    void setResultText(const QString& result);
    void setPasswordClicked();
    void seed();
    void changeConnectedState(bool state);
    void JoinCopayWallet();

signals:
    void showCommandResult(const QString& result);
    void deviceStateHasChanged(bool state);
};

#endif
