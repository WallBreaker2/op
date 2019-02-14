/********************************************************************************
** Form generated from reading UI file 'op_test.ui'
**
** Created by: Qt User Interface Compiler version 5.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OP_TEST_H
#define UI_OP_TEST_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_op_testClass
{
public:
    QWidget *centralWidget;
    QTabWidget *tabWidget;
    QWidget *tabl;
    QWidget *tab_2;
    QWidget *tab_3;
    QWidget *tab;
    QWidget *tab_4;
    QWidget *tab_5;
    QLabel *label;
    QLineEdit *lineEdit;
    QPushButton *pushButton;
    QTextEdit *textEdit;
    QPushButton *pushButton_2;
    QPushButton *pushButton_3;
    QGroupBox *groupBox;
    QTextEdit *textEdit_2;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *op_testClass)
    {
        if (op_testClass->objectName().isEmpty())
            op_testClass->setObjectName(QStringLiteral("op_testClass"));
        op_testClass->resize(687, 473);
        centralWidget = new QWidget(op_testClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        tabWidget = new QTabWidget(centralWidget);
        tabWidget->setObjectName(QStringLiteral("tabWidget"));
        tabWidget->setGeometry(QRect(10, 0, 651, 301));
        tabl = new QWidget();
        tabl->setObjectName(QStringLiteral("tabl"));
        tabWidget->addTab(tabl, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QStringLiteral("tab_2"));
        tabWidget->addTab(tab_2, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName(QStringLiteral("tab_3"));
        tabWidget->addTab(tab_3, QString());
        tab = new QWidget();
        tab->setObjectName(QStringLiteral("tab"));
        tabWidget->addTab(tab, QString());
        tab_4 = new QWidget();
        tab_4->setObjectName(QStringLiteral("tab_4"));
        tabWidget->addTab(tab_4, QString());
        tab_5 = new QWidget();
        tab_5->setObjectName(QStringLiteral("tab_5"));
        label = new QLabel(tab_5);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(10, 10, 54, 12));
        lineEdit = new QLineEdit(tab_5);
        lineEdit->setObjectName(QStringLiteral("lineEdit"));
        lineEdit->setGeometry(QRect(70, 10, 441, 20));
        lineEdit->setReadOnly(true);
        pushButton = new QPushButton(tab_5);
        pushButton->setObjectName(QStringLiteral("pushButton"));
        pushButton->setGeometry(QRect(520, 10, 75, 23));
        textEdit = new QTextEdit(tab_5);
        textEdit->setObjectName(QStringLiteral("textEdit"));
        textEdit->setGeometry(QRect(30, 40, 571, 201));
        pushButton_2 = new QPushButton(tab_5);
        pushButton_2->setObjectName(QStringLiteral("pushButton_2"));
        pushButton_2->setGeometry(QRect(10, 250, 75, 23));
        pushButton_3 = new QPushButton(tab_5);
        pushButton_3->setObjectName(QStringLiteral("pushButton_3"));
        pushButton_3->setGeometry(QRect(100, 250, 75, 23));
        tabWidget->addTab(tab_5, QString());
        groupBox = new QGroupBox(centralWidget);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setGeometry(QRect(10, 300, 651, 121));
        textEdit_2 = new QTextEdit(groupBox);
        textEdit_2->setObjectName(QStringLiteral("textEdit_2"));
        textEdit_2->setGeometry(QRect(30, 20, 581, 81));
        textEdit_2->setReadOnly(true);
        op_testClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(op_testClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 687, 23));
        op_testClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(op_testClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        op_testClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(op_testClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        op_testClass->setStatusBar(statusBar);

        retranslateUi(op_testClass);

        tabWidget->setCurrentIndex(5);


        QMetaObject::connectSlotsByName(op_testClass);
    } // setupUi

    void retranslateUi(QMainWindow *op_testClass)
    {
        op_testClass->setWindowTitle(QApplication::translate("op_testClass", "op_test", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tabl), QApplication::translate("op_testClass", "\345\237\272\346\234\254\350\256\276\347\275\256", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QApplication::translate("op_testClass", "\347\273\221\345\256\232\350\256\276\347\275\256", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tab_3), QApplication::translate("op_testClass", "\345\233\276\350\211\262", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("op_testClass", "\351\224\256\347\233\230\351\274\240\346\240\207", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tab_4), QApplication::translate("op_testClass", "\346\226\207\346\234\254\346\265\213\350\257\225", Q_NULLPTR));
        label->setText(QApplication::translate("op_testClass", "\346\211\223\345\274\200\350\204\232\346\234\254:", Q_NULLPTR));
        pushButton->setText(QApplication::translate("op_testClass", "\346\265\217\350\247\210", Q_NULLPTR));
        pushButton_2->setText(QApplication::translate("op_testClass", "\344\277\235\345\255\230", Q_NULLPTR));
        pushButton_3->setText(QApplication::translate("op_testClass", "\350\277\220\350\241\214", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tab_5), QApplication::translate("op_testClass", "\346\265\213\350\257\225\350\204\232\346\234\254", Q_NULLPTR));
        groupBox->setTitle(QApplication::translate("op_testClass", "\350\276\223\345\207\272", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class op_testClass: public Ui_op_testClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OP_TEST_H
