/********************************************************************************
** Form generated from reading UI file 'daemon.ui'
**
** Created by: Qt User Interface Compiler version 5.4.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DAEMON_H
#define UI_DAEMON_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QPushButton *pushButton;
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *eraseButton;
    QPushButton *seedButton;
    QTextEdit *textEdit;
    QPushButton *connected;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(320, 300);
        MainWindow->setMinimumSize(QSize(320, 300));
        MainWindow->setMaximumSize(QSize(320, 300));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        pushButton = new QPushButton(centralwidget);
        pushButton->setObjectName(QStringLiteral("pushButton"));
        pushButton->setEnabled(false);
        pushButton->setGeometry(QRect(0, 0, 321, 101));
        QIcon icon;
        icon.addFile(QStringLiteral(":/icons/dbb"), QSize(), QIcon::Normal, QIcon::Off);
        icon.addFile(QStringLiteral(":/icons/dbb"), QSize(), QIcon::Disabled, QIcon::Off);
        pushButton->setIcon(icon);
        pushButton->setIconSize(QSize(64, 64));
        pushButton->setFlat(true);
        verticalLayoutWidget = new QWidget(centralwidget);
        verticalLayoutWidget->setObjectName(QStringLiteral("verticalLayoutWidget"));
        verticalLayoutWidget->setGeometry(QRect(0, 100, 321, 141));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(10, 10, 10, 10);
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        eraseButton = new QPushButton(verticalLayoutWidget);
        eraseButton->setObjectName(QStringLiteral("eraseButton"));

        horizontalLayout_2->addWidget(eraseButton);

        seedButton = new QPushButton(verticalLayoutWidget);
        seedButton->setObjectName(QStringLiteral("seedButton"));

        horizontalLayout_2->addWidget(seedButton);


        verticalLayout->addLayout(horizontalLayout_2);

        textEdit = new QTextEdit(verticalLayoutWidget);
        textEdit->setObjectName(QStringLiteral("textEdit"));

        verticalLayout->addWidget(textEdit);

        connected = new QPushButton(centralwidget);
        connected->setObjectName(QStringLiteral("connected"));
        connected->setEnabled(false);
        connected->setGeometry(QRect(274, 0, 51, 41));
        connected->setFlat(true);
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/icons/connected"), QSize(), QIcon::Normal, QIcon::Off);
        icon1.addFile(QStringLiteral(":/icons/connected"), QSize(), QIcon::Disabled, QIcon::Off);
        connected->setIcon(icon1);
        MainWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0));
        pushButton->setText(QString());
        eraseButton->setText(QApplication::translate("MainWindow", "Erase", 0));
        seedButton->setText(QApplication::translate("MainWindow", "Seed", 0));
        connected->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DAEMON_H
