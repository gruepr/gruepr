#ifndef STARTDIALOG_H
#define STARTDIALOG_H

#include "gruepr_globals.h"
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>

class startDialog : public QDialog
{
    Q_OBJECT

public:
    startDialog(QWidget *parent = nullptr);
    ~startDialog();
    startDialog(const startDialog&) = delete;
    startDialog operator= (const startDialog&) = delete;
    startDialog(startDialog&&) = delete;
    startDialog& operator= (startDialog&&) = delete;

    enum Result{makeSurvey = 100, makeGroups};

private:
    QFont *mainBoxFont = nullptr;
    QFont *labelFont = nullptr;
    QGridLayout *theGrid = nullptr;
    QLabel *topLabel = nullptr;
    QToolButton *survMakeButton = nullptr;
    QToolButton *grueprButton = nullptr;
    QLabel *registerLabel = nullptr;
    QLabel *upgradeLabel = nullptr;
    enum class GrueprVersion{unknown, old, current, beta};
    GrueprVersion getLatestVersionFromGithub();
    void openRegisterDialog();

    const int BASEWINDOWWIDTH = 800;
    const int BASEWINDOWHEIGHT = 456;
    const QString BUTTONSTYLE = QString("QToolButton {border-style: solid; border-width: 3px; border-radius: 8px; border-color: #") + GRUEPRDARKBLUEHEX +";"
                                                     "color: #" + GRUEPRDARKBLUEHEX + "; background-color: white;} "
                                        "QToolButton:hover {border-color: #" + GRUEPRMEDBLUEHEX + "; background-color: #" + GRUEPRVERYLIGHTBLUEHEX + "}";
};

#endif // STARTDIALOG_H