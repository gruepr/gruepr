#include "surveyMakerWizard.h"
#include "gruepr_globals.h"
#include <QGroupBox>
#include <QApplication>
#include <QMessageBox>
#include <QRadioButton>

//finish previewpage: upload and save functionality
//add load functionality to intro page
//make multichoice sample questions dialog
//make multichoice custom response options dialog
//make upload roster ui

//Create RegEx for punctuation not allowed within a URL
QRegularExpressionValidator SurveyMakerWizard::noInvalidPunctuation = QRegularExpressionValidator(QRegularExpression("[^,&<>/]*"));

SurveyMakerWizard::SurveyMakerWizard(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(tr("Make a survey"));
    setWizardStyle(QWizard::ModernStyle);
    setMinimumWidth(800);
    setMinimumHeight(600);

    auto palette = this->palette();
    palette.setColor(QPalette::Window, Qt::white);
    palette.setColor(QPalette::Mid, palette.color(QPalette::Base));
    setPalette(palette);

    setPage(Page::intro, new IntroPage);
    setPage(Page::demographics, new DemographicsPage);
    setPage(Page::multichoice, new MultipleChoicePage);
    setPage(Page::schedule, new SchedulePage);
    setPage(Page::courseinfo, new CourseInfoPage);
    setPage(Page::previewexport, new PreviewAndExportPage);

    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton << QWizard::FinishButton;
    setButtonLayout(buttonLayout);

    button(QWizard::CancelButton)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: #" GRUEPRDARKBLUEHEX "; "));
    setButtonText(QWizard::CancelButton, "\u00AB  Cancel");
    button(QWizard::BackButton)->setStyleSheet(STDBUTTONSTYLE);
    setButtonText(QWizard::BackButton, "\u2B60  Previous Step");
    setOption(QWizard::NoBackButtonOnStartPage);
    button(QWizard::NextButton)->setStyleSheet(INVISBUTTONSTYLE);
    setButtonText(QWizard::NextButton, "Next Step  \u2B62");
    button(QWizard::FinishButton)->setStyleSheet(NEXTBUTTONSTYLE);
}

void SurveyMakerWizard::badExpression(QWidget *textWidget, QString &currText, QWidget *parent)
{
    auto *lineEdit = qobject_cast<QLineEdit*>(textWidget);
    if(lineEdit != nullptr)
    {
        lineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>').remove('/'));
    }
    /*else
    {
        auto *textEdit = qobject_cast<QTextEdit*>(textWidget);
        if(textEdit != nullptr)
        {
            textEdit->setText(currText.remove(',').remove('&').remove('<').remove('>').remove('/'));
        }
    }*/
    QApplication::beep();
    QMessageBox::warning(parent, tr("Format error"), tr("Sorry, the following punctuation is not allowed:\n"
                                                      "    ,  &  <  > / \n"
                                                      "Other punctuation is allowed."));
}


SurveyMakerPage::SurveyMakerPage(SurveyMakerWizard::Page page, int numQuestions, QWidget *parent)
    : QWizardPage(parent),
    numQuestions(numQuestions)
{
    QString title = "<span style=\"color: #" GRUEPRDARKBLUEHEX "\">";
    for(int i = 0; i < SurveyMakerWizard::pageNames.count(); i++) {
        if(i > 0) {
            title += " &emsp;/&emsp; ";
        }
        title += SurveyMakerWizard::pageNames[i];
        if(i == page) {
            title += "</span><span style=\"color: #" GRUEPRMEDBLUEHEX "\">";
        }
    }
    title += "</span>";
    QString label = "  " + SurveyMakerWizard::pageNames[page];
    if(page != SurveyMakerWizard::Page::previewexport) {
        label += " " + tr("Questions");
    }

    layout = new QGridLayout(this);
    layout->setSpacing(0);

    pageTitle = new QLabel(title, this);
    pageTitle->setStyleSheet(TITLESTYLE);
    pageTitle->setAlignment(Qt::AlignCenter);
    pageTitle->setMinimumHeight(40);
    pageTitle->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    layout->addWidget(pageTitle, 0, 0, 1, -1);

    topLabel = new QLabel(label, this);
    topLabel->setStyleSheet(TOPLABELSTYLE);
    topLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    topLabel->setMinimumHeight(40);
    topLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    layout->addWidget(topLabel, 1, 0, 1, -1);

    questionWidget = new QWidget;
    questionLayout = new QVBoxLayout;
    questionLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    questionLayout->setSpacing(0);
    questionArea = new QScrollArea;
    questionArea->setWidget(questionWidget);
    questionArea->setStyleSheet("QScrollArea{border: none;}");
    questionArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    questionArea->setWidgetResizable(true);
    questionWidget->setLayout(questionLayout);

    if(page != SurveyMakerWizard::Page::previewexport) {
        layout->addWidget(questionArea, 2, 0, -1, 1);

        previewWidget = new QWidget;
        previewWidget->setStyleSheet("background-color: #ebebeb;");
        previewLayout = new QVBoxLayout;
        previewLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
        previewLayout->setSpacing(0);
        previewArea = new QScrollArea;
        previewArea->setWidget(previewWidget);
        previewArea->setStyleSheet("QScrollArea{background-color: #ebebeb; border: none;}");
        previewArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        previewArea->setWidgetResizable(true);
        previewWidget->setLayout(previewLayout);
        layout->addWidget(previewArea, 2, 1, -1, 1);

        layout->setColumnStretch(0,1);
        layout->setColumnStretch(1,1);

        for(int i = 0; i < numQuestions; i++) {
            questions << new SurveyMakerQuestionWithSwitch;
            questionPreviews << new QWidget;
            questionPreviewLayouts << new QVBoxLayout;
            questionPreviewTopLabels << new QLabel;
            questionPreviewBottomLabels << new QLabel;

            questionLayout->addSpacing(10);
            questionLayout->addWidget(questions.last());

            questionPreviews.last()->setLayout(questionPreviewLayouts.last());
            previewLayout->addWidget(questionPreviews.last());
            questionPreviews.last()->setAttribute(Qt::WA_TransparentForMouseEvents);
            questionPreviews.last()->setFocusPolicy(Qt::NoFocus);
            questionPreviewTopLabels.last()->setStyleSheet(LABELSTYLE);
            questionPreviewBottomLabels.last()->setStyleSheet(LABELSTYLE);
            connect(questions.last(), &SurveyMakerQuestionWithSwitch::valueChanged, questionPreviews.last(), &QWidget::setVisible);
        }
        questionLayout->addStretch(1);
        previewLayout->addStretch(1);
    }
    else {
        layout->addWidget(questionArea, 2, 0, -1, -1);
        layout->setColumnStretch(0,1);
    }
    layout->addItem(new QSpacerItem(0,0), 2, 0);
    layout->setRowStretch(2, 1);
}


IntroPage::IntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    QString title = "<span style=\"color: #" GRUEPRDARKBLUEHEX "\">";
    for(int i = 0, j = SurveyMakerWizard::pageNames.count(); i < j; i++) {
        if(i > 0) {
            title += " &emsp;/&emsp; ";
        }
        title += SurveyMakerWizard::pageNames[i];
        if(i == 0) {
            title += "</span><span style=\"color: #" GRUEPRMEDBLUEHEX "\">";
        }
    }
    pageTitle = new QLabel(title, this);
    pageTitle->setStyleSheet(TITLESTYLE);
    pageTitle->setAlignment(Qt::AlignCenter);
    pageTitle->setScaledContents(true);
    pageTitle->setMinimumHeight(40);
    pageTitle->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    bannerLeft = new QLabel;
    bannerLeft->setStyleSheet("QLabel {background-color: #" GRUEPRDARKBLUEHEX ";}");
    QPixmap leftPixmap(":/icons_new/BannerLeft.png");
    bannerLeft->setPixmap(leftPixmap.scaledToHeight(120, Qt::SmoothTransformation));
    bannerLeft->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    bannerLeft->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    bannerRight = new QLabel;
    bannerRight->setStyleSheet("QLabel {background-color: #" GRUEPRDARKBLUEHEX ";}");
    QPixmap rightPixmap(":/icons_new/BannerRight.png");
    bannerRight->setPixmap(rightPixmap.scaledToHeight(120, Qt::SmoothTransformation));
    bannerRight->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    bannerRight->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    banner = new QLabel;
    QString labelText = "<html><body>"
                        "<span style=\"font-family: 'Paytone One'; font-size: 24pt; color: white;\">"
                        "Make a survey with gruepr</span><br>"
                        "<span style=\"font-family: 'DM Sans'; font-size: 16pt; color: white;\">"
                        "Creating optimal grueps is easy! Get started with our five step survey-making flow below.</span>"
                        "</body></html>";
    banner->setText(labelText);
    banner->setAlignment(Qt::AlignCenter);
    banner->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    banner->setWordWrap(true);
    banner->setStyleSheet("QLabel {background-color: #" GRUEPRDARKBLUEHEX "; color: white;}");
    auto *bannerLayout = new QHBoxLayout;
    bannerLayout->setSpacing(0);
    bannerLayout->setContentsMargins(0, 0, 0, 0);
    bannerLayout->addWidget(bannerLeft);
    bannerLayout->addWidget(banner);
    bannerLayout->addWidget(bannerRight);

    topLabel = new QLabel(this);
    topLabel->setText("<span style=\"color: #" GRUEPRDARKBLUEHEX "; font-size: 12pt; font-family: DM Sans;\">" +
                      tr("Survey Name") + "</span>");
    topLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    surveyTitle = new QLineEdit(this);
    surveyTitle->setPlaceholderText(tr("Enter Text"));
    surveyTitle->setStyleSheet("color: #" GRUEPRDARKBLUEHEX "; font-size: 14pt; font-family: DM Sans; "
                               "border-style: outset; border-width: 2px; border-color: #" GRUEPRDARKBLUEHEX "; ");
    surveyTitle->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    registerField("SurveyTitle", surveyTitle);

    bottomLabel = new QLabel(this);
    bottomLabel->setText("<span style=\"color: #" GRUEPRDARKBLUEHEX "; font-size: 10pt; font-family: DM Sans\">" +
                         tr("This will be the name of the survey you send to your students!") + "</span>");
    bottomLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    getStartedButton = new QPushButton("Get Started  \u2B62", this);
    getStartedButton->setStyleSheet(GETSTARTEDBUTTONSTYLE);
    getStartedButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    layout = new QGridLayout;
    layout->setSpacing(0);
    int row = 0;
    layout->addWidget(pageTitle, row++, 0, 1, -1);
    layout->addLayout(bannerLayout, row++, 0, 1, -1);
    layout->setRowMinimumHeight(row++, 20);
    layout->addWidget(topLabel, row++, 1, Qt::AlignLeft | Qt::AlignBottom);
    layout->setRowMinimumHeight(row++, 10);
    layout->addWidget(surveyTitle, row++, 1);
    layout->setRowMinimumHeight(row++, 10);
    layout->addWidget(bottomLabel, row++, 1, Qt::AlignLeft | Qt::AlignTop);
    layout->setRowMinimumHeight(row++, 20);
    layout->addWidget(getStartedButton, row++, 1);
    layout->setRowMinimumHeight(row, 0);
    layout->setRowStretch(row, 1);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 3);
    layout->setColumnStretch(2, 1);
    setLayout(layout);
}

void IntroPage::initializePage()
{
    auto *wiz = wizard();
    connect(getStartedButton, &QPushButton::clicked, wiz, &QWizard::next);
}


DemographicsPage::DemographicsPage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::demographics, 5, parent)
{
    questions[firstname]->setLabel(tr("First name"));
    questionPreviewTopLabels[firstname]->setText(tr("First name"));
    questionPreviewLayouts[firstname]->addWidget(questionPreviewTopLabels[firstname]);
    fn = new QLineEdit(FIRSTNAMEQUESTION);
    fn->setCursorPosition(0);
    fn->setStyleSheet(LINEEDITSTYLE);
    questionPreviewLayouts[firstname]->addWidget(fn);
    questionPreviewBottomLabels[firstname]->hide();
    questionPreviewLayouts[firstname]->addWidget(questionPreviewBottomLabels[firstname]);
    questionPreviews[firstname]->hide();
    registerField("FirstName", questions[firstname], "value", "valueChanged");

    questions[lastname]->setLabel(tr("Last name"));
    questionPreviewTopLabels[lastname]->setText(tr("Last name"));
    questionPreviewLayouts[lastname]->addWidget(questionPreviewTopLabels[lastname]);
    ln = new QLineEdit(LASTNAMEQUESTION);
    ln->setCursorPosition(0);
    ln->setStyleSheet(LINEEDITSTYLE);
    questionPreviewLayouts[lastname]->addWidget(ln);
    questionPreviewBottomLabels[lastname]->hide();
    questionPreviewLayouts[lastname]->addWidget(questionPreviewBottomLabels[lastname]);
    questionPreviews[lastname]->hide();
    registerField("LastName", questions[lastname], "value", "valueChanged");

    questions[email]->setLabel(tr("Email"));
    questionPreviewTopLabels[email]->setText(tr("Email"));
    questionPreviewLayouts[email]->addWidget(questionPreviewTopLabels[email]);
    em = new QLineEdit(EMAILQUESTION);
    em->setCursorPosition(0);
    em->setStyleSheet(LINEEDITSTYLE);
    questionPreviewLayouts[email]->addWidget(em);
    questionPreviewBottomLabels[email]->hide();
    questionPreviewLayouts[email]->addWidget(questionPreviewBottomLabels[email]);
    questionPreviews[email]->hide();
    registerField("Email", questions[email], "value", "valueChanged");

    questions[gender]->setLabel(tr("Gender"));
    connect(questions[gender], &SurveyMakerQuestionWithSwitch::valueChanged, this, &DemographicsPage::update);
    auto *genderResponses = new QWidget;
    auto *genderResponsesLayout = new QHBoxLayout(genderResponses);
    genderResponsesLabel = new QLabel(tr("Ask as: "));
    genderResponsesLabel->setStyleSheet(LABELSTYLE);
    genderResponsesLabel->setEnabled(false);
    genderResponsesLayout->addWidget(genderResponsesLabel);
    genderResponsesComboBox = new QComboBox;
    genderResponsesComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    genderResponsesComboBox->installEventFilter(new MouseWheelBlocker(genderResponsesComboBox));
    genderResponsesComboBox->addItems({tr("Biological Sex"), tr("Adult Identity"), tr("Child Identity"), tr("Pronouns")});
    genderResponsesComboBox->setStyleSheet(COMBOBOXSTYLE);
    genderResponsesComboBox->setEnabled(false);
    genderResponsesComboBox->setCurrentIndex(3);
    genderResponsesLayout->addWidget(genderResponsesComboBox);
    genderResponsesLayout->addStretch(1);
    questions[gender]->addWidget(genderResponses, 1, 0, true);
    connect(genderResponsesComboBox, &QComboBox::currentIndexChanged, this, &DemographicsPage::update);
    questionPreviewTopLabels[gender]->setText(tr("Gender"));
    questionPreviewLayouts[gender]->addWidget(questionPreviewTopLabels[gender]);
    ge = new QComboBox;
    ge->addItem(PRONOUNQUESTION);
    ge->setStyleSheet(COMBOBOXSTYLE);
    questionPreviewLayouts[gender]->addWidget(ge);
    questionPreviewBottomLabels[gender]->setText(tr("Options: ") + QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    questionPreviewLayouts[gender]->addWidget(questionPreviewBottomLabels[gender]);
    questionPreviews[gender]->hide();
    registerField("Gender", questions[gender], "value", "valueChanged");
    registerField("genderOptions", genderResponsesComboBox);

    questions[urm]->setLabel(tr("Race / ethnicity"));
    questionPreviewTopLabels[urm]->setText(tr("Race / ethnicity"));
    questionPreviewLayouts[urm]->addWidget(questionPreviewTopLabels[urm]);
    re = new QLineEdit(URMQUESTION);
    re->setCursorPosition(0);
    re->setStyleSheet(LINEEDITSTYLE);
    questionPreviewLayouts[urm]->addWidget(re);
    questionPreviewBottomLabels[urm]->hide();
    questionPreviewLayouts[urm]->addWidget(questionPreviewBottomLabels[urm]);
    questionPreviews[urm]->hide();
    registerField("RaceEthnicity", questions[urm], "value", "valueChanged");

    update();
}

void DemographicsPage::initializePage()
{
    auto *wiz = wizard();
    auto palette = wiz->palette();
    palette.setColor(QPalette::Window, GRUEPRDARKBLUE);
    wiz->setPalette(palette);
    wiz->button(QWizard::NextButton)->setStyleSheet(NEXTBUTTONSTYLE);
    wiz->button(QWizard::CancelButton)->setStyleSheet(STDBUTTONSTYLE);
}

void DemographicsPage::cleanupPage()
{
    auto *wiz = wizard();
    auto palette = wiz->palette();
    palette.setColor(QPalette::Window, Qt::white);
    wiz->setPalette(palette);
    wiz->button(QWizard::NextButton)->setStyleSheet(INVISBUTTONSTYLE);
    wiz->button(QWizard::CancelButton)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: #" GRUEPRDARKBLUEHEX "; "));
    wiz->button(QWizard::CustomButton1)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: #" GRUEPRDARKBLUEHEX "; "));
}

void DemographicsPage::update()
{
    genderResponsesLabel->setEnabled(questions[gender]->getValue());
    genderResponsesComboBox->setEnabled(questions[gender]->getValue());
    ge->clear();
    GenderType genderType = static_cast<GenderType>(genderResponsesComboBox->currentIndex());
    if(genderType == GenderType::biol)
    {
        ge->addItem(GENDERQUESTION);
        questionPreviewBottomLabels[gender]->setText(tr("Options: ") + QString(BIOLGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else if(genderType == GenderType::adult)
    {
        ge->addItem(GENDERQUESTION);
        questionPreviewBottomLabels[gender]->setText(tr("Options: ") + QString(ADULTGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else if(genderType == GenderType::child)
    {
        ge->addItem(GENDERQUESTION);
        questionPreviewBottomLabels[gender]->setText(tr("Options: ") + QString(CHILDGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else //if(genderType == GenderType::pronoun)
    {
        ge->addItem(PRONOUNQUESTION);
        questionPreviewBottomLabels[gender]->setText(tr("Options: ") + QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
}


MultipleChoicePage::MultipleChoicePage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::multichoice, 0, parent)
{
    auto stretch = questionLayout->takeAt(0);   // will put this back at the end of the layout after adding everything
    sampleQuestionsFrame = new QFrame(this);
    sampleQuestionsFrame->setStyleSheet("background-color: " + (QColor::fromString("#"+QString(GRUEPRYELLOWHEX)).lighter(133).name()) + "; color: #" GRUEPRDARKBLUEHEX ";");
    sampleQuestionsIcon = new QLabel;
    sampleQuestionsIcon->setPixmap(QPixmap(":/icons_new/lightbulb.png").scaled(20,20,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    sampleQuestionsLabel = new QLabel(tr("Unsure of what to ask? Take a look at some example questions!"));
    sampleQuestionsLabel->setStyleSheet(LABELSTYLE);
    sampleQuestionsLabel->setWordWrap(true);
    sampleQuestionsLayout = new QHBoxLayout(sampleQuestionsFrame);
    sampleQuestionsButton = new QPushButton(tr("View Examples"));
    sampleQuestionsButton->setStyleSheet(EXAMPLEBUTTONSTYLE);
    sampleQuestionsDialog = new QDialog(this);
    connect(sampleQuestionsButton, &QPushButton::clicked, sampleQuestionsDialog, &QDialog::show);
    sampleQuestionsLayout->addWidget(sampleQuestionsIcon, 0, Qt::AlignLeft | Qt::AlignVCenter);
    sampleQuestionsLayout->addWidget(sampleQuestionsLabel, 1, Qt::AlignVCenter);
    sampleQuestionsLayout->addWidget(sampleQuestionsButton, 0, Qt::AlignRight | Qt::AlignVCenter);
    questionLayout->addSpacing(10);
    questionLayout->addWidget(sampleQuestionsFrame);

    questionTexts.reserve(MAX_ATTRIBUTES);
    questionResponses.reserve(MAX_ATTRIBUTES);
    questionMultis.reserve(MAX_ATTRIBUTES);
    registerField("multiChoiceNumQuestions", this, "numQuestions", "numQuestionsChanged");
    registerField("multiChoiceQuestionTexts", this, "questionTexts", "questionTextsChanged");
    registerField("multiChoiceQuestionResponses", this, "questionResponses", "questionResponsesChanged");
    registerField("multiChoiceQuestionMultis", this, "questionMultis", "questionMultisChanged");

    for(int i = 0; i < (MAX_ATTRIBUTES - 1); i++) {
        //add the question
        spacers << new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        questionLayout->addSpacerItem(spacers.last());
        multichoiceQuestions << new SurveyMakerMultichoiceQuestion(i + 1);
        questionLayout->addWidget(multichoiceQuestions.last());
        multichoiceQuestions.last()->hide();

        //add the preview
        questionPreviews << new QWidget;
        questionPreviews.last()->setAttribute(Qt::WA_TransparentForMouseEvents);
        questionPreviews.last()->setFocusPolicy(Qt::NoFocus);
        questionPreviewLayouts << new QVBoxLayout;
        questionPreviews.last()->setLayout(questionPreviewLayouts.last());
        QString fillInQuestion = tr("Question") + " " + QString::number(i + 1);
        questionPreviewTopLabels << new QLabel(fillInQuestion);
        questionPreviewTopLabels.last()->setStyleSheet(LABELSTYLE);
        questionPreviewLayouts.last()->addWidget(questionPreviewTopLabels.last());
        qu << new QComboBox;
        qu.last()->setStyleSheet(COMBOBOXSTYLE);
        qu.last()->setEditable(true);
        qu.last()->setCurrentText("[" + fillInQuestion + "]");
        questionPreviewLayouts.last()->addWidget(qu.last());
        questionPreviewBottomLabels << new QLabel(tr("Options") + ": ---");
        questionPreviewBottomLabels.last()->setStyleSheet(LABELSTYLE);
        questionPreviewLayouts.last()->addWidget(questionPreviewBottomLabels.last());
        previewLayout->insertWidget(i, questionPreviews.last());
        questionPreviews.last()->hide();

        //connect question to delete action and to updating the wizard fields and the preview
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::deleteRequested, this, [this, i]{deleteAQuestion(i);});
        questionTexts << "";
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::questionChanged, this, [this, i, fillInQuestion](const QString &newText)
                                                                                                     {questionTexts[i] = newText;
                                                                                                      qu[i]->setCurrentText(newText.isEmpty()? "[" + fillInQuestion + "]" : newText);});
        questionResponses << QStringList({""});
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::responsesChanged, this, [this, i](const QStringList &newResponses){questionResponses[i] = newResponses;});
        questionMultis << false;
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::multiChanged, this, [this, i](const bool newMulti){questionMultis[i] = newMulti;});
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::responsesAsStringChanged, questionPreviewBottomLabels.last(), &QLabel::setText);
    }

    addQuestionButtonFrame = new QFrame(this);
    addQuestionButtonFrame->setStyleSheet("background-color: #" GRUEPRVERYLIGHTBLUEHEX "; color: #" GRUEPRDARKBLUEHEX ";");
    addQuestionButtonLayout = new QHBoxLayout(addQuestionButtonFrame);
    addQuestionButton = new QPushButton;
    addQuestionButton->setStyleSheet(ADDBUTTONSTYLE);
    addQuestionButton->setText(tr("Create another question"));
    addQuestionButton->setIcon(QIcon(":/icons_new/addButton.png"));
    connect(addQuestionButton, &QPushButton::clicked, this, &MultipleChoicePage::addQuestion);
    addQuestionButtonLayout->addWidget(addQuestionButton, 0, Qt::AlignVCenter);
    questionLayout->addSpacing(10);
    questionLayout->addWidget(addQuestionButtonFrame);
    questionLayout->addItem(stretch);

    addQuestion();
    addQuestion();
}

void MultipleChoicePage::cleanupPage()
{
}

void MultipleChoicePage::setNumQuestions(const int newNumQuestions)
{
    numQuestions = newNumQuestions;
    emit numQuestionsChanged(numQuestions);
}

int MultipleChoicePage::getNumQuestions() const
{
    return numQuestions;
}

void MultipleChoicePage::setQuestionTexts(const QList<QString> &newQuestionTexts)
{
    questionTexts = newQuestionTexts;
    emit questionTextsChanged(questionTexts);
}

QList<QString> MultipleChoicePage::getQuestionTexts() const
{
    return questionTexts;
}

void MultipleChoicePage::setQuestionResponses(const QList<QList<QString>> &newQuestionResponses)
{
    questionResponses = newQuestionResponses;
    emit questionResponsesChanged(questionResponses);
}

QList<QList<QString>> MultipleChoicePage::getQuestionResponses() const
{
    return questionResponses;
}

void MultipleChoicePage::setQuestionMultis(const QList<bool> &newQuestionMultis)
{
    questionMultis = newQuestionMultis;
    emit questionMultisChanged(questionMultis);
}

QList<bool> MultipleChoicePage::getQuestionMultis() const
{
    return questionMultis;
}

void MultipleChoicePage::addQuestion()
{
    spacers[numQuestions]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
    multichoiceQuestions[numQuestions]->show();
    questionPreviews[numQuestions]->show();

    numQuestions++;

    if(numQuestions == (MAX_ATTRIBUTES - 1)) {
        addQuestionButton->setEnabled(false);
        addQuestionButton->setToolTip(tr("Maximum number of questions reached."));
    }
}

void MultipleChoicePage::deleteAQuestion(int questionNum)
{
    //bump the data from every subsequent question up one
    for(int i = questionNum; i < (numQuestions - 1); i++) {
        multichoiceQuestions[i]->setQuestion(multichoiceQuestions[i+1]->getQuestion());
        multichoiceQuestions[i]->setResponses(multichoiceQuestions[i+1]->getResponses());
        multichoiceQuestions[i]->setMulti(multichoiceQuestions[i+1]->getMulti());
    }

    //clear the last question currently displayed, then hide it
    multichoiceQuestions[numQuestions - 1]->setQuestion("");
    multichoiceQuestions[numQuestions - 1]->setResponses(QStringList({""}));
    multichoiceQuestions[numQuestions - 1]->setMulti(false);

    spacers[numQuestions - 1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    multichoiceQuestions[numQuestions - 1]->hide();
    questionPreviews[numQuestions - 1]->hide();

    numQuestions--;

    if(numQuestions < (MAX_ATTRIBUTES - 1)) {
        addQuestionButton->setEnabled(true);
        addQuestionButton->setToolTip("");
    }
}


SchedulePage::SchedulePage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::schedule, 2, parent)
{
    questions[timezone]->setLabel(tr("Timezone"));
    connect(questions[timezone], &SurveyMakerQuestionWithSwitch::valueChanged, this, &SchedulePage::update);
    questionPreviewTopLabels[timezone]->setText(tr("Timezone"));
    questionPreviewLayouts[timezone]->addWidget(questionPreviewTopLabels[timezone]);
    tz = new QComboBox;
    tz->addItem(TIMEZONEQUESTION);
    tz->setStyleSheet(COMBOBOXSTYLE);
    questionPreviewLayouts[timezone]->addWidget(tz);
    questionPreviewBottomLabels[timezone]->setText(tr("Dropdown options: List of global timezones"));
    questionPreviewLayouts[timezone]->addWidget(questionPreviewBottomLabels[timezone]);
    questionPreviews[timezone]->hide();
    registerField("Timezone", questions[timezone], "value", "valueChanged");

    questions[schedule]->setLabel(tr("Schedule"));
    connect(questions[schedule], &SurveyMakerQuestionWithSwitch::valueChanged, this, &SchedulePage::update);
    questionPreviewTopLabels[schedule]->setText(QString(SCHEDULEQUESTION1) + SCHEDULEQUESTION2FREE + SCHEDULEQUESTION3);
    questionPreviewLayouts[schedule]->addWidget(questionPreviewTopLabels[schedule]);
    sc = new QWidget;
    scLayout = new QGridLayout(sc);
    for(int hr = 0; hr < 24; hr++) {
        auto *rowLabel = new QLabel(SurveyMakerWizard::sunday.time().addSecs(hr * 3600).toString("h A"));
        rowLabel->setStyleSheet(LABELSTYLE);
        scLayout->addWidget(rowLabel, hr+1, 0);
    }
    for(int day = 0; day < 7; day++) {
        auto *colLabel = new QLabel(SurveyMakerWizard::sunday.addDays(day).toString("ddd"));
        colLabel->setStyleSheet(LABELSTYLE);
        scLayout->addWidget(colLabel, 0, day+1);
    }
    for(int hr = 1; hr <= 24; hr++) {
        for(int day = 1; day <= 7; day++) {
            auto check = new QCheckBox;
            check->setChecked(true);
            check->setStyleSheet(CHECKBOXSTYLE);
            scLayout->addWidget(check, hr, day);
        }
    }
    scLayout->setSpacing(5);
    scLayout->setColumnStretch(7, 1);
    scLayout->setRowStretch(24, 1);
    questionPreviewLayouts[schedule]->addWidget(sc);
    questionPreviewBottomLabels[schedule]->hide();
    questionPreviewLayouts[schedule]->addWidget(questionPreviewBottomLabels[schedule]);
    questionPreviews[schedule]->hide();
    registerField("Schedule", questions[schedule], "value", "valueChanged");

    //subItems inside schedule question
    int row = 1;

    auto *busyOrFree = new QWidget;
    auto *busyOrFreeLayout = new QHBoxLayout(busyOrFree);
    busyOrFreeLabel = new QLabel(tr("Ask as: "));
    busyOrFreeLabel->setStyleSheet(LABELSTYLE);
    busyOrFreeLabel->setEnabled(false);
    busyOrFreeLayout->addWidget(busyOrFreeLabel);
    busyOrFreeComboBox = new QComboBox;
    busyOrFreeComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    busyOrFreeComboBox->installEventFilter(new MouseWheelBlocker(busyOrFreeComboBox));
    busyOrFreeComboBox->addItems({tr("Free"), tr("Busy")});
    busyOrFreeComboBox->setStyleSheet(COMBOBOXSTYLE);
    busyOrFreeComboBox->setEnabled(false);
    busyOrFreeComboBox->setCurrentIndex(0);
    busyOrFreeLayout->addWidget(busyOrFreeComboBox);
    busyOrFreeLayout->addStretch(1);
    questions[schedule]->addWidget(busyOrFree, row++, 0, true);
    connect(busyOrFreeComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleBusyOrFree", busyOrFreeComboBox);

    baseTimezoneLabel = new QLabel(tr("Select timezone"));
    baseTimezoneLabel->setStyleSheet(LABELSTYLE);
    baseTimezoneLabel->hide();
    questions[schedule]->addWidget(baseTimezoneLabel, row++, 0, false);
    timeZoneNames = QString(TIMEZONENAMES).split(";");
    for(auto &timeZoneName : timeZoneNames)
    {
        timeZoneName.remove('"');
    }
    baseTimezoneComboBox = new ComboBoxWithElidedContents("Pacific: US and Canada, Tijuana [GMT-08:00]", this);
    baseTimezoneComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    baseTimezoneComboBox->installEventFilter(new MouseWheelBlocker(baseTimezoneComboBox));
    baseTimezoneComboBox->setStyleSheet(COMBOBOXSTYLE);
    baseTimezoneComboBox->setToolTip(tr("<html>Description of the timezone students should use to interpret the times in the grid.&nbsp;"
                                        "<b>Be aware how the meaning of the times in the grid changes depending on this setting.</b></html>"));
    baseTimezoneComboBox->addItem(tr("[student's home timezone]"));
    baseTimezoneComboBox->addItem(tr("Custom timezone:"));
    baseTimezoneComboBox->insertSeparator(2);
    for(int zone = 0; zone < timeZoneNames.size(); zone++)
    {
        const QString &zonename = timeZoneNames.at(zone);
        baseTimezoneComboBox->addItem(zonename);
        baseTimezoneComboBox->setItemData(3 + zone, zonename, Qt::ToolTipRole);
    }
    questions[schedule]->addWidget(baseTimezoneComboBox, row++, 0, true);
    baseTimezoneComboBox->hide();
    connect(baseTimezoneComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    customBaseTimezone = new QLineEdit;
    customBaseTimezone->setStyleSheet(LINEEDITSTYLE);
    customBaseTimezone->setPlaceholderText(tr("Custom timezone"));
    questions[schedule]->addWidget(customBaseTimezone, row++, 0, true);
    customBaseTimezone->hide();
    connect(customBaseTimezone, &QLineEdit::textEdited, this, &SchedulePage::update);
    registerField("ScheduleQuestion", this, "scheduleQuestion", "scheduleQuestionChanged");

    timespanLabel = new QLabel(tr("Timespan:"));
    timespanLabel->setStyleSheet(LABELSTYLE);
    timespanLabel->setEnabled(false);
    questions[schedule]->addWidget(timespanLabel, row++, 0, false);
    daysComboBox = new QComboBox;
    daysComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    daysComboBox->installEventFilter(new MouseWheelBlocker(daysComboBox));
    daysComboBox->addItems({tr("All days"), tr("Weekdays"), tr("Weekends"), tr("Custom days/daynames")});
    daysComboBox->setStyleSheet(COMBOBOXSTYLE);
    daysComboBox->setEnabled(false);
    daysComboBox->setCurrentIndex(0);
    questions[schedule]->addWidget(daysComboBox, row++, 0, true, Qt::AlignLeft);
    connect(daysComboBox, &QComboBox::activated, this, &SchedulePage::daysComboBox_activated);

    //load in local (default) day names, and connect subwindow ui to slots
    defaultDayNames.reserve(MAX_DAYS);
    dayNames.reserve(MAX_DAYS);
    dayCheckBoxes.reserve(MAX_DAYS);
    dayLineEdits.reserve(MAX_DAYS);
    for(int day = 0; day < MAX_DAYS; day++)
    {
        defaultDayNames << SurveyMakerWizard::sunday.addDays(day).toString("dddd");
        dayNames << defaultDayNames.at(day);
        dayCheckBoxes << new QCheckBox;
        dayLineEdits <<  new QLineEdit;
        dayCheckBoxes.last()->setStyleSheet(CHECKBOXSTYLE);
        dayCheckBoxes.last()->setChecked(true);
        dayLineEdits.last()->setStyleSheet(LINEEDITSTYLE);
        dayLineEdits.last()->setText(dayNames.at(day));
        dayLineEdits.last()->setPlaceholderText(tr("Day ") + QString::number(day + 1) + tr(" name"));
        connect(dayLineEdits.last(), &QLineEdit::textChanged, this, [this, day](const QString &text) {day_LineEdit_textChanged(text, dayLineEdits[day], dayNames[day]);});
        connect(dayLineEdits.last(), &QLineEdit::editingFinished, this, [this, day] {if(dayNames.at(day).isEmpty()){dayCheckBoxes[day]->setChecked(false);};});
        connect(dayCheckBoxes.last(), &QCheckBox::toggled, this, [this, day](bool checked) {day_CheckBox_toggled(checked, dayLineEdits[day], defaultDayNames.at(day));});
    }
    daysWindow = new dayNamesDialog(dayCheckBoxes, dayLineEdits, this);
    registerField("scheduleDayNames", this, "dayNames", "dayNamesChanged");

    auto *fromTo = new QWidget;
    auto *fromToLayout = new QHBoxLayout(fromTo);
    fromLabel = new QLabel(tr("From"));
    fromLabel->setStyleSheet(LABELSTYLE);
    fromLabel->setEnabled(false);
    fromToLayout->addWidget(fromLabel, 0, Qt::AlignCenter);
    fromComboBox = new QComboBox;
    fromComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    fromComboBox->installEventFilter(new MouseWheelBlocker(fromComboBox));
    fromComboBox->setStyleSheet(COMBOBOXSTYLE);
    fromComboBox->setEnabled(false);
    fromToLayout->addWidget(fromComboBox, 0, Qt::AlignCenter);
    toLabel = new QLabel(tr("to"));
    toLabel->setStyleSheet(LABELSTYLE);
    toLabel->setEnabled(false);
    fromToLayout->addWidget(toLabel, 0, Qt::AlignCenter);
    toComboBox = new QComboBox;
    toComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    toComboBox->installEventFilter(new MouseWheelBlocker(toComboBox));
    toComboBox->setStyleSheet(COMBOBOXSTYLE);
    toComboBox->setEnabled(false);
    for(int hr = 0; hr < 24; hr++) {
        QString time = SurveyMakerWizard::sunday.time().addSecs(hr * 3600).toString("h A");
        fromComboBox->addItem(time);
        toComboBox->addItem(time);
    }
    fromComboBox->setCurrentIndex(STANDARDSCHEDSTARTTIME);
    toComboBox->setCurrentIndex(STANDARDSCHEDENDTIME);
    fromToLayout->addWidget(toComboBox, 0, Qt::AlignCenter);
    fromToLayout->addStretch(1);
    questions[schedule]->addWidget(fromTo, row++, 0, true);
    connect(fromComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleFrom", fromComboBox);
    connect(toComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleTo", toComboBox);

    update();
}

void SchedulePage::cleanupPage()
{
}

void SchedulePage::setDayNames(const QStringList &newDayNames)
{
    dayNames = newDayNames;
}

QStringList SchedulePage::getDayNames() const
{
    return dayNames;
}

void SchedulePage::setScheduleQuestion(const QString &newScheduleQuestion)
{
    scheduleQuestion = newScheduleQuestion;
}

QString SchedulePage::getScheduleQuestion() const
{
    return scheduleQuestion;
}

void SchedulePage::daysComboBox_activated(int index)
{
    daysComboBox->blockSignals(true);
    if(index == 0)
    {
        //All Days
        for(auto &dayCheckBox : dayCheckBoxes)
        {
            dayCheckBox->setChecked(true);
        }
    }
    else if(index == 1)
    {
        //Weekdays
        dayCheckBoxes[Sun]->setChecked(false);
        for(int day = Mon; day <= Fri; day++)
        {
            dayCheckBoxes[day]->setChecked(true);
        }
        dayCheckBoxes[Sat]->setChecked(false);
    }
    else if(index == 2)
    {
        //Weekends
        dayCheckBoxes[Sun]->setChecked(true);
        for(int day = Mon; day <= Fri; day++)
        {
            dayCheckBoxes[day]->setChecked(false);
        }
        dayCheckBoxes[Sat]->setChecked(true);
    }
    else
    {
        //Custom Days, open subwindow
        daysWindow->exec();
        checkDays();
    }
    daysComboBox->blockSignals(false);
    update();
}

void SchedulePage::day_CheckBox_toggled(bool checked, QLineEdit *dayLineEdit, const QString &dayname)
{
    dayLineEdit->setText(checked? dayname : "");
    dayLineEdit->setEnabled(checked);
    checkDays();
    update();
}

void SchedulePage::checkDays()
{
    bool weekends = dayCheckBoxes[Sun]->isChecked() && dayCheckBoxes[Sat]->isChecked();
    bool noWeekends = !(dayCheckBoxes[Sun]->isChecked() || dayCheckBoxes[Sat]->isChecked());
    bool weekdays = dayCheckBoxes[Mon]->isChecked() && dayCheckBoxes[Tue]->isChecked() &&
                    dayCheckBoxes[Wed]->isChecked() && dayCheckBoxes[Thu]->isChecked() && dayCheckBoxes[Fri]->isChecked();
    bool noWeekdays = !(dayCheckBoxes[Mon]->isChecked() || dayCheckBoxes[Tue]->isChecked() ||
                        dayCheckBoxes[Wed]->isChecked() || dayCheckBoxes[Thu]->isChecked() || dayCheckBoxes[Fri]->isChecked());
    if(weekends && weekdays)
    {
        daysComboBox->setCurrentIndex(0);
    }
    else if(weekdays && noWeekends)
    {
        daysComboBox->setCurrentIndex(1);
    }
    else if(weekends && noWeekdays)
    {
        daysComboBox->setCurrentIndex(2);
    }
    else
    {
        daysComboBox->setCurrentIndex(3);
    }
}

void SchedulePage::day_LineEdit_textChanged(const QString &text, QLineEdit *dayLineEdit, QString &dayname)
{
    //validate entry
    QString currText = text;
    int currPos = 0;
    if(SurveyMakerWizard::noInvalidPunctuation.validate(currText, currPos) != QValidator::Acceptable)
    {
        SurveyMakerWizard::badExpression(dayLineEdit, currText, this);
    }
    dayname = currText.trimmed();
    update();
}

void SchedulePage::update()
{
    //update the schedule grid
    QList<QWidget *> widgets = sc->findChildren<QWidget *>();
    for(auto &widget : widgets) {
        int row, col, rowSpan, colSpan;
        scLayout->getItemPosition(scLayout->indexOf(widget), &row, &col, &rowSpan, &colSpan);
        widget->setVisible(((row == 0) || ((row-1) >= fromComboBox->currentIndex() && (row-1) <= toComboBox->currentIndex())) &&
                           ((col == 0) || dayCheckBoxes[col-1]->isChecked()));
        if((row == 0) && (col > 0)) {
            auto *dayLabel = qobject_cast<QLabel *>(widget);
            if(dayLabel != nullptr) {
                dayLabel->setText((dayNames[col-1]).left(3));
            }
        }
    }

    if(fromComboBox->currentIndex() > toComboBox->currentIndex()) {
        fromComboBox->setStyleSheet(ERRORCOMBOBOXSTYLE);
        toComboBox->setStyleSheet(ERRORCOMBOBOXSTYLE);
    }
    else {
        fromComboBox->setStyleSheet(COMBOBOXSTYLE);
        toComboBox->setStyleSheet(COMBOBOXSTYLE);
    }

    bool scheduleOn = questions[schedule]->getValue();
    bool timezoneOn = questions[timezone]->getValue();

    baseTimezoneLabel->setVisible(timezoneOn);
    baseTimezoneLabel->setEnabled(scheduleOn);
    baseTimezoneComboBox->setVisible(timezoneOn);
    baseTimezoneComboBox->setEnabled(scheduleOn);
    customBaseTimezone->setVisible(baseTimezoneComboBox->currentIndex() == 1);
    customBaseTimezone->setEnabled(scheduleOn);
    switch(baseTimezoneComboBox->currentIndex()) {
    case 0:
        baseTimezone = SCHEDULEQUESTIONHOME;
        break;
    case 1:
        baseTimezone = customBaseTimezone->text();
        break;
    default:
        baseTimezone = baseTimezoneComboBox->currentText();
        break;
    }
    QString previewlabelText = SCHEDULEQUESTION1;
    previewlabelText += ((busyOrFreeComboBox->currentIndex() == 0)? SCHEDULEQUESTION2FREE : SCHEDULEQUESTION2BUSY);
    previewlabelText += SCHEDULEQUESTION3;
    if(timezoneOn && scheduleOn) {
        previewlabelText += SCHEDULEQUESTION4 + baseTimezone;
        if(baseTimezoneComboBox->currentIndex() != 1)
        {
            previewlabelText += SCHEDULEQUESTION5;
        }
    }
    questionPreviewTopLabels[schedule]->setText(previewlabelText);
    scheduleQuestion = previewlabelText;

    busyOrFreeLabel->setEnabled(scheduleOn);
    busyOrFreeComboBox->setEnabled(scheduleOn);
    timespanLabel->setEnabled(scheduleOn);
    daysComboBox->setEnabled(scheduleOn);
    fromLabel->setEnabled(scheduleOn);
    fromComboBox->setEnabled(scheduleOn);
    toLabel->setEnabled(scheduleOn);
    toComboBox->setEnabled(scheduleOn);
}

CourseInfoPage::CourseInfoPage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::courseinfo, 4, parent)
{
    questions[section]->setLabel(tr("Section"));
    connect(questions[section], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    sectionLineEdits.reserve(10);
    deleteSectionButtons.reserve(10);
    sectionNames.reserve(10);
    sectionLineEdits.append(new QLineEdit);
    sectionLineEdits.append(new QLineEdit);
    deleteSectionButtons.append(new QPushButton);
    deleteSectionButtons.append(new QPushButton);
    addSectionButton = new QPushButton;
    sectionLineEdits[0]->setStyleSheet(LINEEDITSTYLE);
    sectionLineEdits[1]->setStyleSheet(LINEEDITSTYLE);
    deleteSectionButtons[0]->setStyleSheet(DELBUTTONSTYLE);
    deleteSectionButtons[1]->setStyleSheet(DELBUTTONSTYLE);
    addSectionButton->setStyleSheet(ADDBUTTONSTYLE);
    sectionLineEdits[0]->setPlaceholderText(tr("Section name"));
    sectionLineEdits[1]->setPlaceholderText(tr("Section name"));
    deleteSectionButtons[0]->setText(tr("Delete"));
    deleteSectionButtons[1]->setText(tr("Delete"));
    deleteSectionButtons[0]->setIcon(QIcon(":/icons_new/trashButton.png"));
    deleteSectionButtons[1]->setIcon(QIcon(":/icons_new/trashButton.png"));
    addSectionButton->setText(tr("Add section"));
    addSectionButton->setIcon(QIcon(":/icons_new/addButton.png"));
    sectionLineEdits[0]->setEnabled(false);
    sectionLineEdits[1]->setEnabled(false);
    deleteSectionButtons[0]->setEnabled(false);
    deleteSectionButtons[1]->setEnabled(false);
    addSectionButton->setEnabled(false);
    questions[section]->addWidget(sectionLineEdits[0], 1, 0, false);
    questions[section]->addWidget(sectionLineEdits[1], 2, 0, false);
    questions[section]->addWidget(deleteSectionButtons[0], 1, 1, false);
    questions[section]->addWidget(deleteSectionButtons[1], 2, 1, false);
    questions[section]->addWidget(addSectionButton, 3, 0, false, Qt::AlignLeft);
    connect(sectionLineEdits[0], &QLineEdit::textChanged, this, &CourseInfoPage::update);
    connect(sectionLineEdits[1], &QLineEdit::textChanged, this, &CourseInfoPage::update);
    connect(deleteSectionButtons[0], &QPushButton::clicked, this, [this]{deleteASection(0);});
    connect(deleteSectionButtons[1], &QPushButton::clicked, this, [this]{deleteASection(1);});
    connect(addSectionButton, &QPushButton::clicked, this, &CourseInfoPage::addASection);

    questionPreviewTopLabels[section]->setText(tr("Section"));
    questionPreviewLayouts[section]->addWidget(questionPreviewTopLabels[section]);
    sc = new QComboBox;
    sc->addItem(SECTIONQUESTION);
    sc->setStyleSheet(COMBOBOXSTYLE);
    questionPreviewLayouts[section]->addWidget(sc);
    questionPreviewLayouts[section]->addWidget(questionPreviewBottomLabels[section]);
    questionPreviews[section]->hide();
    registerField("Section", questions[section], "value", "valueChanged");
    registerField("SectionNames", this, "sectionNames", "sectionNamesChanged");

    questions[wantToWorkWith]->setLabel(tr("Classmates I want to work with"));
    connect(questions[wantToWorkWith], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    questionPreviewTopLabels[wantToWorkWith]->setText(tr("Classmates"));
    questionPreviewLayouts[wantToWorkWith]->addWidget(questionPreviewTopLabels[wantToWorkWith]);
    ww = new QLineEdit(PREF1TEAMMATEQUESTION);
    ww->setCursorPosition(0);
    ww->setStyleSheet(LINEEDITSTYLE);
    wwc = new QComboBox;
    wwc->addItem(tr("Select a classmate you would like to work with"));
    wwc->setStyleSheet(COMBOBOXSTYLE);
    questionPreviewLayouts[wantToWorkWith]->addWidget(ww);
    questionPreviewLayouts[wantToWorkWith]->addWidget(wwc);
    questionPreviewBottomLabels[wantToWorkWith]->setText(tr(""));
    questionPreviewLayouts[wantToWorkWith]->addWidget(questionPreviewBottomLabels[wantToWorkWith]);
    wwc->hide();
    questionPreviews[wantToWorkWith]->hide();
    questionPreviewBottomLabels[wantToWorkWith]->hide();
    registerField("PrefTeammate", questions[wantToWorkWith], "value", "valueChanged");

    questionLayout->removeItem(questionLayout->itemAt(4));

    questions[wantToAvoid]->setLabel(tr("Classmates I want to avoid"));
    connect(questions[wantToAvoid], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    questionPreviewTopLabels[wantToAvoid]->setText(tr("Classmates"));
    questionPreviewLayouts[wantToAvoid]->addWidget(questionPreviewTopLabels[wantToAvoid]);
    wa = new QLineEdit(PREF1NONTEAMMATEQUESTION);
    wa->setCursorPosition(0);
    wa->setStyleSheet(LINEEDITSTYLE);
    wac = new QComboBox;
    wac->addItem(tr("Select a classmate you would like to avoid working with"));
    wac->setStyleSheet(COMBOBOXSTYLE);
    questionPreviewLayouts[wantToAvoid]->addWidget(wa);
    questionPreviewLayouts[wantToAvoid]->addWidget(wac);
    questionPreviewBottomLabels[wantToAvoid]->setText(tr(""));
    questionPreviewLayouts[wantToAvoid]->addWidget(questionPreviewBottomLabels[wantToAvoid]);
    wac->hide();
    questionPreviewBottomLabels[wantToWorkWith]->hide();
    questionPreviews[wantToAvoid]->hide();
    registerField("PrefNonTeammate", questions[wantToAvoid], "value", "valueChanged");

    questionLayout->removeItem(questionLayout->itemAt(5));

    questions[selectFromList]->setLabel(tr("Select from list of classmates"));
    connect(questions[selectFromList], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    uploadExplainer = new QLabel(tr("You can upload a list of classmates so that students select names rather than typing as a free response question"));
    uploadExplainer->setWordWrap(true);
    uploadExplainer->setStyleSheet(LABELSTYLE);
    uploadButton = new QPushButton;
    uploadButton->setStyleSheet(ADDBUTTONSTYLE);
    uploadButton->setText(tr("Upload class roster"));
    uploadButton->setIcon(QIcon(":/icons_new/addButton.png"));
    uploadButton->setEnabled(false);
    connect(uploadButton, &QPushButton::clicked, this, &CourseInfoPage::uploadRoster);
    questions[selectFromList]->addWidget(uploadExplainer, 1, 0, true);
    questions[selectFromList]->addWidget(uploadButton, 2, 0, false, Qt::AlignLeft);
    questionPreviews[selectFromList]->hide();
    registerField("StudentNames", this, "studentNames", "studentNamesChanged");

    update();
}

void CourseInfoPage::initializePage()
{
}

void CourseInfoPage::cleanupPage()
{
}

void CourseInfoPage::setSectionNames(const QStringList &newSectionNames)
{
    sectionNames = newSectionNames;
}

QStringList CourseInfoPage::getSectionNames() const
{
    return sectionNames;
}

void CourseInfoPage::setStudentNames(const QStringList &newStudentNames)
{
    studentNames = newStudentNames;
}

QStringList CourseInfoPage::getStudentNames() const
{
    return studentNames;
}

void CourseInfoPage::update()
{
    int lastFilledSection = sectionLineEdits.size()-1;
    sectionNames.clear();
    int i = 0;
    for(auto &sectionLineEdit : sectionLineEdits) {
        if(!(sectionLineEdit->text().isEmpty())) {
            sectionNames.append(sectionLineEdit->text());
        }
        sectionLineEdit->setEnabled(questions[section]->getValue());
        deleteSectionButtons[i++]->setEnabled((questions[section]->getValue()) && (lastFilledSection > 1));
    }
    addSectionButton->setEnabled(questions[section]->getValue());
    questionPreviewBottomLabels[section]->setText(tr("Options: ") + sectionNames.join("  |  "));
    emit sectionNamesChanged(sectionNames);

    uploadButton->setEnabled(questions[selectFromList]->isEnabled() && questions[selectFromList]->getValue());

    questionPreviewTopLabels[wantToAvoid]->setHidden(questions[wantToWorkWith]->getValue() && questions[wantToAvoid]->getValue());
    if(!questions[selectFromList]->getValue() || studentNames.isEmpty()) {
        ww->show();
        wa->show();
        wwc->hide();
        wac->hide();
        questionPreviewBottomLabels[wantToWorkWith]->hide();
        questionPreviewBottomLabels[wantToAvoid]->hide();
    }
    else {
        ww->hide();
        wa->hide();
        wwc->show();
        wac->show();
        questionPreviewBottomLabels[wantToWorkWith]->show();
        questionPreviewBottomLabels[wantToAvoid]->show();
        questionPreviewBottomLabels[wantToWorkWith]->setHidden(questions[wantToWorkWith]->getValue() && questions[wantToAvoid]->getValue());
        questionPreviewBottomLabels[wantToWorkWith]->setText(tr("Options: ") + studentNames.join("  |  "));
        questionPreviewBottomLabels[wantToAvoid]->setText(tr("Options: ") + studentNames.join("  |  "));
    }
}

void CourseInfoPage::addASection()
{
    static int numSectionsEntered = 2;  // used to set the row in the grid layout where the new lineedit goes (since rows can be added but never removed)
    numSectionsEntered++;
    int nextSectionNum = sectionLineEdits.size();   // used to set the location in the QLists of lineedits and pushbuttons
    sectionLineEdits.append(new QLineEdit);
    deleteSectionButtons.append(new QPushButton);
    sectionLineEdits[nextSectionNum]->setStyleSheet(LINEEDITSTYLE);
    deleteSectionButtons[nextSectionNum]->setStyleSheet(DELBUTTONSTYLE);
    sectionLineEdits[nextSectionNum]->setPlaceholderText(tr("Section name"));
    deleteSectionButtons[nextSectionNum]->setText(tr("Delete"));
    deleteSectionButtons[nextSectionNum]->setIcon(QIcon(":/icons_new/trashButton.png"));
    sectionLineEdits[nextSectionNum]->setEnabled(questions[section]->getValue());
    questions[section]->moveWidget(addSectionButton, numSectionsEntered + 1, 0, false, Qt::AlignLeft);
    questions[section]->addWidget(sectionLineEdits[nextSectionNum], numSectionsEntered, 0, false);
    questions[section]->addWidget(deleteSectionButtons[nextSectionNum], numSectionsEntered, 1, false);
    connect(sectionLineEdits[nextSectionNum], &QLineEdit::textChanged, this, &CourseInfoPage::update);
    connect(deleteSectionButtons[nextSectionNum], &QPushButton::clicked, this, [this, nextSectionNum]{deleteASection(nextSectionNum);});
    sectionLineEdits[nextSectionNum]->setFocus();

    update();
}

void CourseInfoPage::deleteASection(int sectionNum)
{
    sectionLineEdits[sectionNum]->setParent(nullptr);
    deleteSectionButtons[sectionNum]->setParent(nullptr);
    delete sectionLineEdits[sectionNum];
    delete deleteSectionButtons[sectionNum];
    sectionLineEdits.removeAt(sectionNum);
    deleteSectionButtons.removeAt(sectionNum);
    for(int i = 0; i < sectionLineEdits.size(); i++) {
        deleteSectionButtons[i]->disconnect();
        connect(deleteSectionButtons[i], &QPushButton::clicked, this, [this, i]{deleteASection(i);});
    }

    update();
}

void CourseInfoPage::uploadRoster()
{
    ////////////////////////get csv file, parse into names
    studentNames = {"Joshua Hertz", "Jasmine Lellock", "Cora Hertz", "Charlie Hertz"};

    emit studentNamesChanged(studentNames);

    update();
}


PreviewAndExportPage::PreviewAndExportPage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::previewexport, NULL, parent)
{
    QList<int> questionNums({1, 5, MAX_ATTRIBUTES, 2, 3});
    for(int sectionNum = 0; sectionNum < 5; sectionNum++) {
        preSectionSpacer << new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        questionLayout->addItem(preSectionSpacer.last());
        section << new SurveyMakerPreviewSection(sectionNum, SurveyMakerWizard::pageNames[sectionNum], questionNums[sectionNum], this);
        questionLayout->addWidget(section.last());
        connect(section.last(), &SurveyMakerPreviewSection::editRequested, this, [this](int pageNum){while(pageNum < 5){wizard()->back(); pageNum++;}});
    }

    auto *saveExportFrame = new QFrame;
    saveExportFrame->setStyleSheet("background-color: #" GRUEPRDARKBLUEHEX "; color: white; font-family:'DM Sans'; font-size:12pt;");
    auto *saveExportlayout = new QVBoxLayout(saveExportFrame);
    auto *saveExporttitle = new QLabel("<span style=\"color: white; font-family:'DM Sans'; font-size:14pt;\">" + tr("Export Survey As:") + "</span>");
    auto *destination = new QGroupBox("");
    destination->setStyleSheet("border-style:none;");
    auto *radio1 = new QRadioButton(tr("Google Form"));
    auto *radio2 = new QRadioButton(tr("Canvas Survey"));
    auto *radio3 = new QRadioButton(tr("Text Files"));
    radio1->setChecked(true);
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(radio1);
    vbox->addWidget(radio2);
    vbox->addWidget(radio3);
    destination->setLayout(vbox);
    auto *exportButton = new QPushButton;
    exportButton->setStyleSheet(NEXTBUTTONSTYLE);
    exportButton->setText(tr("Export Survey"));
    connect(exportButton, &QPushButton::clicked, this, &PreviewAndExportPage::exportSurvey);
    auto *saveButton = new QPushButton;
    saveButton->setStyleSheet(NEXTBUTTONSTYLE);
    saveButton->setText(tr("Save Survey"));
    connect(saveButton, &QPushButton::clicked, this, &PreviewAndExportPage::saveSurvey);
    saveExportlayout->addWidget(saveExporttitle);
    saveExportlayout->addWidget(destination);
    saveExportlayout->addWidget(exportButton);
    saveExportlayout->addWidget(saveButton);
    preSectionSpacer << new QSpacerItem(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
    questionLayout->addItem(preSectionSpacer.last());
    questionLayout->addWidget(saveExportFrame);

    questionLayout->addStretch(1);

    section[0]->questionLineEdit[0]->show();

    section[1]->questionLabel[0]->setText(tr("First Name"));
    section[1]->questionLineEdit[0]->setText(FIRSTNAMEQUESTION);
    section[1]->questionLabel[1]->setText(tr("Last Name"));
    section[1]->questionLineEdit[1]->setText(LASTNAMEQUESTION);
    section[1]->questionLabel[2]->setText(tr("Email"));
    section[1]->questionLineEdit[2]->setText(EMAILQUESTION);
    section[1]->questionLabel[3]->setText(tr("Gender"));
    section[1]->questionLabel[4]->setText(tr("Race / ethnicity"));
    section[1]->questionLineEdit[4]->setText(URMQUESTION);

    section[3]->questionLabel[0]->setText(tr("Timezone"));
    section[3]->questionComboBox[0]->addItem(TIMEZONEQUESTION);
    section[3]->questionComboBox[0]->insertSeparator(1);
    auto timeZoneNames = QString(TIMEZONENAMES).split(";");
    for(auto &timeZoneName : timeZoneNames)
    {
        timeZoneName.remove('"');
    }
    section[3]->questionComboBox[0]->addItems(timeZoneNames);
    section[3]->questionBottomLabel[0]->setText(tr("Dropdown options: List of global timezones"));
    section[3]->questionLabel[1]->setText(tr("Schedule"));
    schedGrid = new QWidget;
    schedGridLayout = new QGridLayout(schedGrid);
    for(int hr = 0; hr < 24; hr++) {
        auto *rowLabel = new QLabel(SurveyMakerWizard::sunday.time().addSecs(hr * 3600).toString("h A"));
        rowLabel->setStyleSheet(LABELSTYLE);
        schedGridLayout->addWidget(rowLabel, hr+1, 0);
    }
    for(int day = 0; day < 7; day++) {
        auto *colLabel = new QLabel(SurveyMakerWizard::sunday.addDays(day).toString("ddd"));
        colLabel->setStyleSheet(LABELSTYLE);
        schedGridLayout->addWidget(colLabel, 0, day+1);
        schedGridLayout->setColumnStretch(day, 1);
    }
    for(int hr = 1; hr <= 24; hr++) {
        for(int day = 1; day <= 7; day++) {
            auto check = new QCheckBox;
            check->setChecked(true);
            check->setStyleSheet(CHECKBOXSTYLE);
            schedGridLayout->addWidget(check, hr, day);
        }
    }
    schedGridLayout->setSpacing(5);
    schedGridLayout->setRowStretch(24, 1);
    section[3]->addWidget(schedGrid);

    section[4]->questionLabel[0]->setText(tr("Section"));
    section[4]->questionLabel[1]->setText(PREF1TEAMMATEQUESTION);
    section[4]->questionLabel[2]->setText(PREF1NONTEAMMATEQUESTION);
}

void PreviewAndExportPage::initializePage()
{
    auto *wiz = wizard();
    auto palette = wiz->palette();
    palette.setColor(QPalette::Window, GRUEPRDARKBLUE);
    wiz->setPalette(palette);
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::FinishButton;
    wiz->setButtonLayout(buttonLayout);
    wiz->button(QWizard::NextButton)->setStyleSheet(NEXTBUTTONSTYLE);
    wiz->button(QWizard::CancelButton)->setStyleSheet(STDBUTTONSTYLE);

    //Survey title
    preSectionSpacer[0]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
    section[0]->questionLineEdit[0]->setText(field("SurveyTitle").toString());

    //Demographics
    auto firstname = field("FirstName").toBool();
    auto lastname = field("LastName").toBool();
    auto email = field("Email").toBool();
    auto gender = field("Gender").toBool();
    auto urm = field("RaceEthnicity").toBool();

    if(firstname) {
        section[1]->preQuestionSpacer[0]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[1]->questionLabel[0]->show();
        section[1]->questionLineEdit[0]->show();
    }
    else {
        section[1]->preQuestionSpacer[0]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[1]->questionLabel[0]->hide();
        section[1]->questionLineEdit[0]->hide();
    }

    if(lastname) {
        section[1]->preQuestionSpacer[1]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[1]->questionLabel[1]->show();
        section[1]->questionLineEdit[1]->show();
    }
    else {
        section[1]->preQuestionSpacer[1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[1]->questionLabel[1]->hide();
        section[1]->questionLineEdit[1]->hide();
    }

    if(email) {
        section[1]->preQuestionSpacer[2]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[1]->questionLabel[2]->show();
        section[1]->questionLineEdit[2]->show();
    }
    else {
        section[1]->preQuestionSpacer[2]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[1]->questionLabel[2]->hide();
        section[1]->questionLineEdit[2]->hide();
    }

    if(gender) {
        section[1]->preQuestionSpacer[3]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[1]->questionLabel[3]->show();
        section[1]->questionComboBox[3]->show();
        section[1]->questionBottomLabel[3]->show();
        QString text;
        QStringList options;
        switch(static_cast<GenderType>(field("genderOptions").toInt())) {
        case GenderType::biol:
            text = GENDERQUESTION;
            options = QString(BIOLGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
            break;
        case GenderType::adult:
            text = GENDERQUESTION;
            options = QString(ADULTGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
            break;
        case GenderType::child:
            text = GENDERQUESTION;
            options = QString(CHILDGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
            break;
        case GenderType::pronoun:
            text = PRONOUNQUESTION;
            options = QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
            break;
        }
        section[1]->questionComboBox[3]->clear();
        section[1]->questionComboBox[3]->addItem(text);
        section[1]->questionComboBox[3]->insertSeparator(1);
        section[1]->questionComboBox[3]->addItems(options);
        section[1]->questionBottomLabel[3]->setText(tr("Dropdown options: ") + options.join(", "));
    }
    else {
        section[1]->preQuestionSpacer[3]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[1]->questionLabel[3]->hide();
        section[1]->questionComboBox[3]->hide();
        section[1]->questionBottomLabel[3]->hide();
    }

    if(urm) {
        section[1]->preQuestionSpacer[4]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[1]->questionLabel[4]->show();
        section[1]->questionLineEdit[4]->show();
    }
    else {
        section[1]->preQuestionSpacer[4]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[1]->questionLabel[4]->hide();
        section[1]->questionLineEdit[4]->hide();
    }

    if(firstname || lastname || email || gender || urm) {
        preSectionSpacer[1]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[1]->show();
    }
    else {
        preSectionSpacer[1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[1]->hide();
    }

    //Multiple Choice
    auto multiChoiceNumQuestions = field("multiChoiceNumQuestions").toInt();
    auto multiQuestionTexts = field("multiChoiceQuestionTexts").toList();
    auto multiQuestionResponses = field("multiChoiceQuestionResponses").toList();
    auto multiQuestionMultis = field("multiChoiceQuestionMultis").toList();
    int actualNumMultiQuestions = 0;
    for(int questionNum = 0; questionNum < multiChoiceNumQuestions; questionNum++) {
        if(!multiQuestionTexts[questionNum].toString().isEmpty()) {
            actualNumMultiQuestions++;
            section[2]->preQuestionSpacer[questionNum]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
            section[2]->questionLabel[questionNum]->setText(multiQuestionTexts[questionNum].toString());
            section[2]->questionLabel[questionNum]->show();
            section[2]->questionComboBox[questionNum]->clear();
            section[2]->questionComboBox[questionNum]->addItems(multiQuestionResponses[questionNum].toStringList());
            section[2]->questionComboBox[questionNum]->show();
            QString botLabel = tr("Dropdown options: ") + multiQuestionResponses[questionNum].toStringList().join(", ");
            if((multiQuestionMultis[questionNum]).toBool()) {
                botLabel += "\n   {Multiple selections allowed}";
            }
            section[2]->questionBottomLabel[questionNum]->setText(botLabel);
            section[2]->questionBottomLabel[questionNum]->show();
        }
        else {
            section[2]->preQuestionSpacer[questionNum]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
            section[2]->questionLabel[questionNum]->hide();
            section[2]->questionComboBox[questionNum]->hide();
            section[2]->questionBottomLabel[questionNum]->hide();
        }
    }
    for(int i = actualNumMultiQuestions; i < MAX_ATTRIBUTES; i++) {
        section[2]->preQuestionSpacer[i]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[2]->questionLabel[i]->hide();
        section[2]->questionComboBox[i]->hide();
        section[2]->questionBottomLabel[i]->hide();
    }

    if(actualNumMultiQuestions > 0) {
        preSectionSpacer[2]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[2]->show();
    }
    else {
        preSectionSpacer[2]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[2]->hide();
    }

    //Schedule
    auto timezone = field("Timezone").toBool();
    auto schedule = field("Schedule").toBool();
    //auto scheduleAsBusy = field("scheduleBusyOrFree").toBool();
    auto scheduleQuestion = field("ScheduleQuestion").toString();
    auto scheduleDays = field("scheduleDayNames").toStringList();
    auto scheduleFrom = field("scheduleFrom").toInt();
    auto scheduleTo = field("scheduleTo").toInt();

    if(timezone) {
        section[3]->preQuestionSpacer[0]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[3]->questionLabel[0]->show();
        section[3]->questionComboBox[0]->show();
        section[3]->questionBottomLabel[0]->show();
    }
    else {
        section[3]->preQuestionSpacer[0]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[3]->questionLabel[0]->hide();
        section[3]->questionComboBox[0]->hide();
        section[3]->questionBottomLabel[0]->hide();
    }

    if(schedule) {
        section[3]->preQuestionSpacer[1]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[3]->questionLabel[1]->setText(scheduleQuestion);
        section[3]->questionLabel[1]->show();
        QList<QWidget *> widgets = schedGrid->findChildren<QWidget *>();
        for(auto &widget : widgets) {
            int row, col, rowSpan, colSpan;
            schedGridLayout->getItemPosition(schedGridLayout->indexOf(widget), &row, &col, &rowSpan, &colSpan);
            widget->setVisible(((row == 0) || ((row-1) >= scheduleFrom && (row-1) <= scheduleTo)) &&
                               ((col == 0) || (!scheduleDays[col-1].isEmpty())));
            if((row == 0) && (col > 0)) {
                auto *dayLabel = qobject_cast<QLabel *>(widget);
                if(dayLabel != nullptr) {
                    dayLabel->setText(scheduleDays[col-1]);
                }
            }
        }
    }
    else {
        section[3]->preQuestionSpacer[1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[3]->questionLabel[1]->hide();
    }

    if(timezone || schedule) {
        preSectionSpacer[3]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[3]->show();
    }
    else {
        preSectionSpacer[3]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[3]->hide();
    }

    //Course Info
    auto courseSections = field("Section").toBool();
    auto courseSectionsNames = field("SectionNames").toStringList();
    auto prefTeammate = field("PrefTeammate").toBool();
    auto prefNonTeammate = field("PrefNonTeammate").toBool();
    auto studentNames = field("StudentNames").toStringList();

    if(courseSections) {
        section[4]->preQuestionSpacer[0]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[4]->questionLabel[0]->show();
        section[4]->questionComboBox[0]->show();
        section[4]->questionBottomLabel[0]->show();
        section[4]->questionComboBox[0]->clear();
        section[4]->questionComboBox[0]->addItem(SECTIONQUESTION);
        section[4]->questionComboBox[0]->insertSeparator(1);
        section[4]->questionComboBox[0]->addItems(courseSectionsNames);
        section[4]->questionBottomLabel[0]->setText(tr("Dropdown options: ") + courseSectionsNames.join(", "));
    }
    else {
        section[4]->preQuestionSpacer[0]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[4]->questionLabel[0]->hide();
        section[4]->questionComboBox[0]->hide();
        section[4]->questionBottomLabel[0]->hide();
    }

    if(prefTeammate) {
        section[4]->preQuestionSpacer[1]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[4]->questionLabel[1]->show();
        if(studentNames.isEmpty()) {
            section[4]->questionLineEdit[1]->show();
            section[4]->questionComboBox[1]->hide();
        }
        else {
            section[4]->questionLineEdit[1]->hide();
            section[4]->questionComboBox[1]->show();
        }
    }
    else {
        section[4]->preQuestionSpacer[1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[4]->questionLabel[1]->hide();
        section[4]->questionLineEdit[1]->hide();
        section[4]->questionComboBox[1]->hide();
    }

    if(prefNonTeammate) {
        section[4]->preQuestionSpacer[2]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[4]->questionLabel[2]->show();
        if(studentNames.isEmpty()) {
            section[4]->questionLineEdit[2]->show();
            section[4]->questionComboBox[2]->hide();
        }
        else {
            section[4]->questionLineEdit[2]->hide();
            section[4]->questionComboBox[2]->show();
        }
    }
    else {
        section[4]->preQuestionSpacer[2]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[4]->questionLabel[2]->hide();
        section[4]->questionLineEdit[2]->hide();
        section[4]->questionComboBox[2]->hide();
    }

    if(courseSections || prefTeammate || prefNonTeammate) {
        preSectionSpacer[4]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[4]->show();
    }
    else {
        preSectionSpacer[4]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[4]->hide();
    }
}

void PreviewAndExportPage::cleanupPage()
{
    // going back to previous page, so allow user to immediately return to this preview
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton << QWizard::CustomButton1;
    wizard()->setButtonLayout(buttonLayout);
    setButtonText(QWizard::CustomButton1, "Return to Preview");
    wizard()->button(QWizard::CustomButton1)->setStyleSheet(NEXTBUTTONSTYLE);
    connect(wizard(), &QWizard::customButtonClicked, this, [this]{wizard()->setCurrentId(SurveyMakerWizard::Page::previewexport);});
}

bool PreviewAndExportPage::saveSurvey()
{
    return true;
}

bool PreviewAndExportPage::exportSurvey()
{
    return true;
}
