#ifndef STARTDIALOG_H
#define STARTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QProgressDialog>

class StartDialog : public QDialog
{
    Q_OBJECT

public:
    StartDialog(QWidget *parent = nullptr);
    ~StartDialog() override = default;
    StartDialog(const StartDialog&) = delete;
    StartDialog operator= (const StartDialog&) = delete;
    StartDialog(StartDialog&&) = delete;
    StartDialog& operator= (StartDialog&&) = delete;

signals:
    void closeDataDialogProgressBar();

private:
    void openRegisterDialog();
    void openSurveyMaker();
    void openGruepr();
    enum class GrueprVersion{unknown, old, current, beta};
    GrueprVersion getLatestVersionFromGithub();

    QLabel *registerLabel = nullptr;
    QLabel *upgradeLabel = nullptr;
    QProgressDialog *loadingBar = nullptr;

    QList<QAction *> helpActions;
};

#endif // STARTDIALOG_H
