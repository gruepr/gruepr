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

    /**
     * @brief closeDataDialogProgressBar Creates a progress bar for the "close data dialog".
     */
    void closeDataDialogProgressBar();

private:

    /**
     * @brief openRegisterDialog Opens the dialog responsible for registering this version of Gruepr to the user. METHOD STRUCTURE UNCHANGED.
     */
    void openRegisterDialog();

    /**
     * @brief openSurveyMaker Opens the GUI responsible for creating a new survey.
     */
    void openSurveyMaker();

    /**
     * @brief openGruepr Opens the survey importing page (other option on the introductory Gruepr window).
     */
    void openGruepr();

    /**
     * @brief The GrueprVersion enum are the different versions for Gruepr software.
     */
    enum class GrueprVersion{unknown, old, current, beta};

    /**
     * @brief getLatestVersionFromGithub Obtains the latest version of the Gruepr software from its Github page.
     * @return An enumeration of which version this Gruepr copy corresponds to (GrueprVersion enumeration).
     */
    GrueprVersion getLatestVersionFromGithub();

    QLabel *registerLabel = nullptr;
    QLabel *upgradeLabel = nullptr;
    QProgressDialog *loadingBar = nullptr;
    QList<QAction *> helpActions;

    inline static const int BASEWINDOWWIDTH = 800;
    inline static const int BASEWINDOWHEIGHT = 456;
    inline static const int LEFTRIGHTSPACERWIDTH = 74;
    inline static const int MIDDLESPACERWIDTH = 42;
    inline static const int TOPSPACERHEIGHT = 30;
    inline static const int MIDDLESPACERHEIGHT = 36;
    inline static const int BOTTOMSPACERHEIGHT = 46;
    inline static const QSize TOOLBUTTONSIZE = QSize(300, 256);
#if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
    inline static const QSize INFOBUTTONSIZE = QSize(25, 25);
#endif
    inline static const int ICONHEIGHT = 117;
    inline static const int BIGFONTSIZE = 24;
    inline static const int LITTLEFONTSIZE = 12;
};

#endif // STARTDIALOG_H
