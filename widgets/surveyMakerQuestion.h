#ifndef SURVEYMAKERQUESTION_H
#define SURVEYMAKERQUESTION_H

#include "switchButton.h"
#include "widgets/comboBoxWithElidedContents.h"
#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

class SurveyMakerQuestionWithSwitch : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(bool value READ getValue WRITE setValue NOTIFY valueChanged)

public:
    explicit SurveyMakerQuestionWithSwitch(QWidget *parent = nullptr, const QString &textLabel = "", bool startingValue = false);

    void setLabel(const QString &text);
    void setValue(bool value);
    bool getValue() const;

    void addWidget(QWidget *widget, int row, int column, bool expandToRestOfRow, Qt::Alignment horizontalAlignment = Qt::Alignment());
    void moveWidget(QWidget *widget, int newRow, int newColumn, bool expandToRestOfRow, Qt::Alignment horizontalAlignment = Qt::Alignment());

    void mousePressEvent(QMouseEvent *event) override;
    void setEnabled(bool);

signals:
    void valueChanged(bool newvalue);

private slots:
    void valueChange(bool newvalue);

private:
    QLabel *label = nullptr;
    SwitchButton *switchButton = nullptr;
    QGridLayout *layout = nullptr;
};


//////////////////////////////////////////////////////////////////////////////////////////////////


class SurveyMakerMultichoiceQuestion : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QString question READ getQuestion WRITE setQuestion NOTIFY questionChanged)
    Q_PROPERTY(QStringList responses READ getResponses WRITE setResponses NOTIFY responsesChanged)
    Q_PROPERTY(bool multi READ getMulti WRITE setMulti NOTIFY multiChanged)

public:
    explicit SurveyMakerMultichoiceQuestion(int questionNum, QWidget *parent = nullptr);

    void resizeEvent(QResizeEvent *event) override;

    void setNumber(const int questionNum);
    QWidget *previewWidget = nullptr;

    void setQuestion(const QString &newQuestion);
    QString getQuestion() const;
    void setResponses(const QStringList &newResponses);
    QStringList getResponses() const;
    void setMulti(const bool newMulti);
    bool getMulti() const;

signals:
    void deleteRequested();
    void questionChanged(const QString &newQuestion);
    void responsesChanged(const QStringList &newResponses);
    void multiChanged(const bool newMulti);

private slots:
    void deleteRequest();
    void responsesComboBoxActivated(int index);
    void questionChange();
    void multiChange(const bool newMulti);

private:
    int numOfResponseOptions;
    QLabel *label = nullptr;
    QPushButton *deleteButton = nullptr;
    QPlainTextEdit *questionPlainTextEdit = nullptr;
    void resizeQuestionPlainTextEdit();
    QLabel *responsesLabel = nullptr;
    ComboBoxWithElidedContents *responsesComboBox = nullptr;
    QCheckBox *multiAllowed = nullptr;
    QVBoxLayout *previewLayout = nullptr;
    void updatePreviewWidget();
};


//////////////////////////////////////////////////////////////////////////////////////////////////


class SurveyMakerPreviewSection : public QFrame
{
    Q_OBJECT

public:
    explicit SurveyMakerPreviewSection(const int pageNum, const QString &titleText, const int numQuestions, QWidget *parent = nullptr);
    QList<QSpacerItem *> preQuestionSpacer;
    QList<QLabel *> questionLabel;
    QList<QLineEdit *> questionLineEdit;
    QList<QComboBox *> questionComboBox;
    QList<QGroupBox *> questionGroupBox;
    QList<QVBoxLayout *> questionGroupLayout;

    void addWidget(QWidget *widget);
    void setTitle(const QString &newTitle);

signals:
    void editRequested(const int pageNum);

private:
    int row = 0;
    QGridLayout *layout = nullptr;
    QLabel *title = nullptr;
};

#endif // SURVEYMAKERQUESTION_H
