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
    ~whichFilesDialog() override;
    whichFilesDialog(const whichFilesDialog&) = delete;
    whichFilesDialog operator= (const whichFilesDialog&) = delete;
    whichFilesDialog(whichFilesDialog&&) = delete;
    whichFilesDialog& operator= (whichFilesDialog&&) = delete;

    bool studentFiletxt = false;
    bool studentFilepdf = false;
    bool instructorFiletxt = false;
    bool instructorFilepdf = false;
    bool spreadsheetFiletxt = false;

private slots:
    void boxToggled();

private:
    QCheckBox *studentFiletxtCheckBox = nullptr;
    QCheckBox *studentFilepdfCheckBox = nullptr;
    QCheckBox *instructorFiletxtCheckBox = nullptr;
    QCheckBox *instructorFilepdfCheckBox = nullptr;
    QCheckBox *spreadsheetFiletxtCheckBox = nullptr;
    QDialogButtonBox *buttonBox = nullptr;
    QFont previousToolTipFont;
    inline static const int CHECKBOXSIZE = 30;
};


#endif // WHICHFILESDIALOG_H
