#ifndef STARTDIALOG_H
#define STARTDIALOG_H

#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QMenuBar>
#include <QToolButton>

class StartDialog : public QDialog
{
    Q_OBJECT

public:
    StartDialog(QWidget *parent = nullptr);
    ~StartDialog();
    StartDialog(const StartDialog&) = delete;
    StartDialog operator= (const StartDialog&) = delete;
    StartDialog(StartDialog&&) = delete;
    StartDialog& operator= (StartDialog&&) = delete;

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

    QList<QAction *> helpActions;
    QMenu *helpMenu = nullptr;
#if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
    QToolButton *helpButton = nullptr;
#else
    QMenuBar *menuBar = nullptr;
#endif

    void openRegisterDialog();
    void openSurveyMaker();
    void openGruepr();
};

#endif // STARTDIALOG_H
