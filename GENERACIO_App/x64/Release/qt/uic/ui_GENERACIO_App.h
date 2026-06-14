/********************************************************************************
** Form generated from reading UI file 'GENERACIO_App.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GENERACIO_APP_H
#define UI_GENERACIO_APP_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_GENERACIO_AppClass
{
public:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QWidget *centralWidget;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *GENERACIO_AppClass)
    {
        if (GENERACIO_AppClass->objectName().isEmpty())
            GENERACIO_AppClass->setObjectName("GENERACIO_AppClass");
        GENERACIO_AppClass->resize(600, 400);
        menuBar = new QMenuBar(GENERACIO_AppClass);
        menuBar->setObjectName("menuBar");
        GENERACIO_AppClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(GENERACIO_AppClass);
        mainToolBar->setObjectName("mainToolBar");
        GENERACIO_AppClass->addToolBar(mainToolBar);
        centralWidget = new QWidget(GENERACIO_AppClass);
        centralWidget->setObjectName("centralWidget");
        GENERACIO_AppClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(GENERACIO_AppClass);
        statusBar->setObjectName("statusBar");
        GENERACIO_AppClass->setStatusBar(statusBar);

        retranslateUi(GENERACIO_AppClass);

        QMetaObject::connectSlotsByName(GENERACIO_AppClass);
    } // setupUi

    void retranslateUi(QMainWindow *GENERACIO_AppClass)
    {
        GENERACIO_AppClass->setWindowTitle(QCoreApplication::translate("GENERACIO_AppClass", "GENERACIO_App", nullptr));
    } // retranslateUi

};

namespace Ui {
    class GENERACIO_AppClass: public Ui_GENERACIO_AppClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GENERACIO_APP_H
