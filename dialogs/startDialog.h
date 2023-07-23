#ifndef STARTDIALOG_H
#define STARTDIALOG_H

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

#if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
    QToolButton *helpButton = nullptr;
#endif
    QList<QAction *> helpActions;

    void openRegisterDialog();
    void openSurveyMaker();
};

#endif // STARTDIALOG_H
