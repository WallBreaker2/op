/********************************************************************************
** Form generated from reading UI file 'Tool.ui'
**
** Created by: Qt User Interface Compiler version 5.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TOOL_H
#define UI_TOOL_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ToolClass
{
public:
    QWidget *centralWidget;
    QGroupBox *groupBox;
    QLabel *label_3;
    QGroupBox *groupBox_2;
    QLabel *label_4;
    QPushButton *pushButton;
    QPushButton *pushButton_3;
    QGroupBox *groupBox_4;
    QGroupBox *groupBox_5;
    QListView *listView;
    QPushButton *pushButton_2;
    QLineEdit *lineEdit;
    QLabel *label;
    QPushButton *pushButton_7;
    QPushButton *pushButton_8;
    QPushButton *pushButton_9;
    QLineEdit *lineEdit_2;
    QLabel *label_2;
    QPushButton *pushButton_6;
    QPushButton *pushButton_4;
    QPushButton *pushButton_5;
    QGroupBox *groupBox_3;
    QTextEdit *textEdit;
    QGroupBox *groupBox_6;
    QLabel *label_5;
    QLineEdit *lineEdit_3;
    QToolBar *mainToolBar;

    void setupUi(QMainWindow *ToolClass)
    {
        if (ToolClass->objectName().isEmpty())
            ToolClass->setObjectName(QStringLiteral("ToolClass"));
        ToolClass->resize(753, 386);
        centralWidget = new QWidget(ToolClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        groupBox = new QGroupBox(centralWidget);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setGeometry(QRect(120, 0, 171, 81));
        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(10, 20, 151, 51));
        groupBox_2 = new QGroupBox(centralWidget);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        groupBox_2->setGeometry(QRect(120, 120, 171, 91));
        label_4 = new QLabel(groupBox_2);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(10, 20, 151, 61));
        pushButton = new QPushButton(centralWidget);
        pushButton->setObjectName(QStringLiteral("pushButton"));
        pushButton->setGeometry(QRect(130, 90, 75, 23));
        pushButton_3 = new QPushButton(centralWidget);
        pushButton_3->setObjectName(QStringLiteral("pushButton_3"));
        pushButton_3->setGeometry(QRect(210, 90, 75, 23));
        groupBox_4 = new QGroupBox(centralWidget);
        groupBox_4->setObjectName(QStringLiteral("groupBox_4"));
        groupBox_4->setGeometry(QRect(300, 0, 241, 261));
        groupBox_5 = new QGroupBox(centralWidget);
        groupBox_5->setObjectName(QStringLiteral("groupBox_5"));
        groupBox_5->setGeometry(QRect(550, 0, 191, 361));
        listView = new QListView(groupBox_5);
        listView->setObjectName(QStringLiteral("listView"));
        listView->setGeometry(QRect(10, 80, 171, 181));
        pushButton_2 = new QPushButton(groupBox_5);
        pushButton_2->setObjectName(QStringLiteral("pushButton_2"));
        pushButton_2->setGeometry(QRect(10, 300, 81, 23));
        lineEdit = new QLineEdit(groupBox_5);
        lineEdit->setObjectName(QStringLiteral("lineEdit"));
        lineEdit->setGeometry(QRect(90, 270, 71, 20));
        lineEdit->setMaxLength(3);
        label = new QLabel(groupBox_5);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(30, 270, 60, 16));
        pushButton_7 = new QPushButton(groupBox_5);
        pushButton_7->setObjectName(QStringLiteral("pushButton_7"));
        pushButton_7->setGeometry(QRect(10, 20, 75, 23));
        pushButton_8 = new QPushButton(groupBox_5);
        pushButton_8->setObjectName(QStringLiteral("pushButton_8"));
        pushButton_8->setGeometry(QRect(100, 300, 75, 23));
        pushButton_9 = new QPushButton(groupBox_5);
        pushButton_9->setObjectName(QStringLiteral("pushButton_9"));
        pushButton_9->setGeometry(QRect(90, 20, 75, 23));
        lineEdit_2 = new QLineEdit(groupBox_5);
        lineEdit_2->setObjectName(QStringLiteral("lineEdit_2"));
        lineEdit_2->setGeometry(QRect(50, 50, 113, 20));
        lineEdit_2->setReadOnly(true);
        label_2 = new QLabel(groupBox_5);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(10, 50, 31, 16));
        pushButton_6 = new QPushButton(groupBox_5);
        pushButton_6->setObjectName(QStringLiteral("pushButton_6"));
        pushButton_6->setGeometry(QRect(10, 330, 81, 23));
        pushButton_4 = new QPushButton(centralWidget);
        pushButton_4->setObjectName(QStringLiteral("pushButton_4"));
        pushButton_4->setGeometry(QRect(120, 240, 75, 23));
        pushButton_5 = new QPushButton(centralWidget);
        pushButton_5->setObjectName(QStringLiteral("pushButton_5"));
        pushButton_5->setGeometry(QRect(200, 240, 75, 23));
        groupBox_3 = new QGroupBox(centralWidget);
        groupBox_3->setObjectName(QStringLiteral("groupBox_3"));
        groupBox_3->setGeometry(QRect(120, 270, 421, 91));
        textEdit = new QTextEdit(groupBox_3);
        textEdit->setObjectName(QStringLiteral("textEdit"));
        textEdit->setGeometry(QRect(10, 20, 391, 61));
        textEdit->setReadOnly(true);
        groupBox_6 = new QGroupBox(centralWidget);
        groupBox_6->setObjectName(QStringLiteral("groupBox_6"));
        groupBox_6->setGeometry(QRect(10, 0, 101, 361));
        label_5 = new QLabel(groupBox_6);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setGeometry(QRect(13, 40, 81, 301));
        lineEdit_3 = new QLineEdit(centralWidget);
        lineEdit_3->setObjectName(QStringLiteral("lineEdit_3"));
        lineEdit_3->setGeometry(QRect(120, 220, 171, 20));
        ToolClass->setCentralWidget(centralWidget);
        mainToolBar = new QToolBar(ToolClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        ToolClass->addToolBar(Qt::TopToolBarArea, mainToolBar);

        retranslateUi(ToolClass);

        QMetaObject::connectSlotsByName(ToolClass);
    } // setupUi

    void retranslateUi(QMainWindow *ToolClass)
    {
        ToolClass->setWindowTitle(QApplication::translate("ToolClass", "Tool", Q_NULLPTR));
        groupBox->setTitle(QApplication::translate("ToolClass", "\345\216\237\345\233\276\345\203\217", Q_NULLPTR));
        label_3->setText(QApplication::translate("ToolClass", "src rect...", Q_NULLPTR));
        groupBox_2->setTitle(QApplication::translate("ToolClass", "\344\272\214\345\200\274\345\214\226", Q_NULLPTR));
        label_4->setText(QApplication::translate("ToolClass", "binary rect...", Q_NULLPTR));
        pushButton->setText(QApplication::translate("ToolClass", "\345\212\240\350\275\275", Q_NULLPTR));
        pushButton_3->setText(QApplication::translate("ToolClass", "\347\274\226\350\276\221", Q_NULLPTR));
        groupBox_4->setTitle(QApplication::translate("ToolClass", "\347\202\271\351\230\265(32x32)", Q_NULLPTR));
        groupBox_5->setTitle(QApplication::translate("ToolClass", "\345\255\227\345\272\223", Q_NULLPTR));
        pushButton_2->setText(QApplication::translate("ToolClass", "\346\267\273\345\212\240", Q_NULLPTR));
        label->setText(QApplication::translate("ToolClass", "\345\256\232\344\271\211\346\226\207\345\255\227\357\274\232", Q_NULLPTR));
        pushButton_7->setText(QApplication::translate("ToolClass", "\345\212\240\350\275\275\345\255\227\345\272\223", Q_NULLPTR));
        pushButton_8->setText(QApplication::translate("ToolClass", "\344\277\235\345\255\230\345\255\227\345\272\223", Q_NULLPTR));
        pushButton_9->setText(QApplication::translate("ToolClass", "\347\274\226\350\276\221\345\255\227\345\272\223", Q_NULLPTR));
        label_2->setText(QApplication::translate("ToolClass", "\350\267\257\345\276\204:", Q_NULLPTR));
        pushButton_6->setText(QApplication::translate("ToolClass", "\345\210\240\351\231\244", Q_NULLPTR));
        pushButton_4->setText(QApplication::translate("ToolClass", "\344\272\214\345\200\274\345\214\226", Q_NULLPTR));
        pushButton_5->setText(QApplication::translate("ToolClass", "\346\217\220\345\217\226\347\202\271\351\230\265", Q_NULLPTR));
        groupBox_3->setTitle(QApplication::translate("ToolClass", "Ocr\346\265\213\350\257\225", Q_NULLPTR));
        groupBox_6->setTitle(QApplication::translate("ToolClass", "\351\242\234\350\211\262\346\217\217\350\277\260", Q_NULLPTR));
        label_5->setText(QApplication::translate("ToolClass", "Color", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class ToolClass: public Ui_ToolClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TOOL_H
