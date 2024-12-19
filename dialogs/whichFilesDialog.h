#ifndef WHICHFILESDIALOG_H
#define WHICHFILESDIALOG_H

#include <QDialog>
#include "dataOptions.h"

namespace Ui {
class WhichFilesDialog;
}


class WhichFilesDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Action {save, print};

    WhichFilesDialog(const Action saveOrPrint, const DataOptions *const dataOptions, const QStringList &previews, QWidget *parent = nullptr);
    ~WhichFilesDialog() override;
    WhichFilesDialog(const WhichFilesDialog&) = delete;
    WhichFilesDialog operator= (const WhichFilesDialog&) = delete;
    WhichFilesDialog(WhichFilesDialog&&) = delete;
    WhichFilesDialog& operator= (WhichFilesDialog&&) = delete;

    enum class FileType {student, instructor, spreadsheet, custom} fileType = FileType::student;
    bool pdf = false;
    struct CustomFileOptions {
        bool includeFileData = false;
        bool includeTeamingData = false;
        bool includeTeamScore = false;
        bool includeFirstName = false;
        bool includeLastName = false;
        bool includeEmail = false;
        bool includeGender = false;
        bool includeURM = false;
        bool includeSect = false;
        QList<bool> includeMultiChoice;
        bool includeTimezone = false;
        bool includeSechedule = false;
    } customFileOptions;

private:
    Ui::WhichFilesDialog *ui;
    QFont previousToolTipFont;
};


#endif // WHICHFILESDIALOG_H
