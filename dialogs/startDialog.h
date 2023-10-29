#ifndef STARTDIALOG_H
#define STARTDIALOG_H

#include <QDialog>
#include <QLabel>

class StartDialog : public QDialog
{
    Q_OBJECT

public:
    StartDialog(QWidget *parent = nullptr);
    ~StartDialog() = default;
    StartDialog(const StartDialog&) = delete;
    StartDialog operator= (const StartDialog&) = delete;
    StartDialog(StartDialog&&) = delete;
    StartDialog& operator= (StartDialog&&) = delete;

private:
    void openRegisterDialog();
    void openSurveyMaker();
    void openGruepr();
    enum class GrueprVersion{unknown, old, current, beta};
    GrueprVersion getLatestVersionFromGithub();

    QLabel *registerLabel = nullptr;
    QLabel *upgradeLabel = nullptr;

    QList<QAction *> helpActions;
};

#endif // STARTDIALOG_H
