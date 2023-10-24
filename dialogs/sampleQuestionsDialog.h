#ifndef SAMPLEQUESTIONSDIALOG_H
#define SAMPLEQUESTIONSDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>

namespace Ui {
class SampleQuestionsDialog;
}

class SampleQuestionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SampleQuestionsDialog(QWidget *parent = nullptr);
    ~SampleQuestionsDialog() override;
    SampleQuestionsDialog(const SampleQuestionsDialog&) = delete;
    SampleQuestionsDialog operator= (const SampleQuestionsDialog&) = delete;
    SampleQuestionsDialog(SampleQuestionsDialog&&) = delete;
    SampleQuestionsDialog& operator= (SampleQuestionsDialog&&) = delete;


private:
    Ui::SampleQuestionsDialog *ui;
};

#endif // SAMPLEQUESTIONSDIALOG_H
