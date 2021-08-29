#ifndef WHICHFILESDIALOG_H
#define WHICHFILESDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>

class whichFilesDialog : public QDialog
{
    Q_OBJECT

public:
    enum action{save, print};

    whichFilesDialog(const action saveOrPrint, const QStringList &previews = {}, QWidget *parent = nullptr);
    ~whichFilesDialog();

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
    const int CHECKBOXSIZE = 30;
};


#endif // WHICHFILESDIALOG_H
