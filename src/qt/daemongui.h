// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBBDAEMON_QT_DAEMONGUI_H
#define DBBDAEMON_QT_DAEMONGUI_H

#include <QMainWindow>
#include <QWidget>

namespace Ui {
    class MainWindow;
}

class DBBDaemonGui : public QMainWindow
{
	Q_OBJECT

public:
	explicit DBBDaemonGui(QWidget *parent = 0);
	~DBBDaemonGui();

	private:
		Ui::MainWindow *ui;
		bool processComnand;
		std::string sessionPassword; //TODO: needs secure space / mem locking

		void changeConnectedState(bool state);
		bool sendCommand(const std::string &cmd, const std::string &password);

	public slots:
	    /** Set number of connections shown in the UI */
	    void eraseClicked();
		void ledClicked();
		void setResultText(const QString &result);
		void setPasswordClicked();
		void seed();

	signals:
	    void showCommandResult(const QString &result);
};

#endif
