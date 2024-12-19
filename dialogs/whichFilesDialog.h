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
        bool includeFileData;
        bool includeTeamingData;
        bool includeFirstName;
        bool includeLastName;
        bool includeEmail;
        bool includeGender;
        bool includeURM;
        bool includeSect;
        QList<bool> includeMultiChoice;
        bool includeTimezone;
        bool includeSechedule;
    } customFileOptions;

private:
    Ui::WhichFilesDialog *ui;
    QFont previousToolTipFont;
};


#endif // WHICHFILESDIALOG_H
