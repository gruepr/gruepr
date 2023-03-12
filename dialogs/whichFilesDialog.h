#ifndef WHICHFILESDIALOG_H
#define WHICHFILESDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>

class whichFilesDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Action {save, print};

    whichFilesDialog(const Action saveOrPrint, const QStringList &previews = {}, QWidget *parent = nullptr);
    ~whichFilesDialog();
    whichFilesDialog(const whichFilesDialog&) = delete;
    whichFilesDialog operator= (const whichFilesDialog&) = delete;
    whichFilesDialog(whichFilesDialog&&) = delete;
    whichFilesDialog& operator= (whichFilesDialog&&) = delete;

    QCheckBox *studentFiletxt;
    QCheckBox *studentFilepdf;
    QCheckBox *instructorFiletxt;
    QCheckBox *instructorFilepdf;
    QCheckBox *spreadsheetFiletxt;

private slots:
    void boxToggled();

private:
    bool saveDialog;
    QGridLayout *theGrid;
    QLabel *explanation;
    QLabel *textfile;
    QLabel *pdffile;
    QPushButton *studentFileLabel;
    QPushButton *instructorFileLabel;
    QPushButton *spreadsheetFileLabel;
    QDialogButtonBox *buttonBox;
    QFont previousToolTipFont;
    inline static const int CHECKBOXSIZE = 30;
};


#endif // WHICHFILESDIALOG_H
