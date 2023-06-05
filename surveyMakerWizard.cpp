#include "surveyMakerWizard.h"
#include "gruepr_globals.h"

//finish previewpage ui
//figure out why multichoice questions dissappear when hitting "next" but not "back" to go to page
//make multichoice sample questions dialog
//crash when closing then reopening survemakaer

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

    button(QWizard::CancelButton)->setStyleSheet(NEXTBUTTONSTYLE);
    setButtonText(QWizard::CancelButton, "\u00AB  Cancel");
    button(QWizard::BackButton)->setStyleSheet(STDBUTTONSTYLE);
    setButtonText(QWizard::BackButton, "\u2B60  Previous Step");
    button(QWizard::NextButton)->setStyleSheet(INVISBUTTONSTYLE);
    setButtonText(QWizard::NextButton, "Next Step  \u2B62");
    button(QWizard::FinishButton)->setStyleSheet(NEXTBUTTONSTYLE);

    setPage(Page::intro, new IntroPage);
    setPage(Page::demographics, new DemographicsPage);
    setPage(Page::multichoice, new MultipleChoicePage);
    setPage(Page::schedule, new SchedulePage);
    setPage(Page::courseinfo, new CourseInfoPage);
    setPage(Page::previewexport, new PreviewAndExportPage);

    setOption(QWizard::NoBackButtonOnStartPage);
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton << QWizard::FinishButton;
    setButtonLayout(buttonLayout);
}


SurveyMakerPage::SurveyMakerPage(SurveyMakerWizard::Page page, int numQuestions, QWidget *parent)
    : QWizardPage(parent),
    numQuestions(numQuestions)
{
    QString title = "<span style=\"color: #" GRUEPRDARKBLUEHEX "\">";
    for(int i = 0, j = SurveyMakerWizard::pageNames.count(); i < j; i++) {
        if(i > 0) {
            title += " &ensp;|&ensp; ";
        }
        title += SurveyMakerWizard::pageNames[i];
        if(i == page) {
            title += "</span><span style=\"color: #" GRUEPRMEDBLUEHEX "\">";
        }
    }
    QString label = "  " + SurveyMakerWizard::pageNames[page] + " " + tr("Questions");

    layout = new QGridLayout;
    setLayout(layout);
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

    questionWidget = new QWidget(this);
    questionArea = new QScrollArea(this);
    questionArea->setWidget(questionWidget);
    questionArea->setStyleSheet("QScrollArea{border: none;}");
    questionArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    questionArea->setWidgetResizable(true);
    questionLayout = new QVBoxLayout(questionWidget);
    questionLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    questionLayout->setSpacing(0);
    layout->addWidget(questionArea, 2, 0, -1, 1);

    previewWidget = new QWidget(this);
    previewWidget->setStyleSheet("background-color: #ebebeb;");
    previewArea = new QScrollArea(this);
    previewArea->setWidget(previewWidget);
    previewArea->setStyleSheet("QScrollArea{background-color: #ebebeb; border: none;}");
    previewArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    previewArea->setWidgetResizable(true);
    previewLayout = new QVBoxLayout(previewWidget);
    previewLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    previewLayout->setSpacing(0);
    layout->addWidget(previewArea, 2, 1, -1, 1);

    layout->setColumnStretch(0,1);
    layout->setColumnStretch(1,1);

    if(numQuestions > 0) {
        questions = new SurveyMakerQuestionWithSwitch[numQuestions];
        questionPreviews = new QWidget[numQuestions];
        questionPreviewLayouts = new QVBoxLayout[numQuestions];
        questionPreviewTopLabels = new QLabel[numQuestions];
        questionPreviewBottomLabels = new QLabel[numQuestions];
        for(int i = 0; i < numQuestions; i++) {
            questionLayout->addSpacing(10);
            questionLayout->addWidget(&questions[i]);

            questionPreviews[i].setLayout(&questionPreviewLayouts[i]);
            previewLayout->addWidget(&questionPreviews[i]);
            questionPreviews[i].setAttribute(Qt::WA_TransparentForMouseEvents);
            questionPreviews[i].setFocusPolicy(Qt::NoFocus);
            questionPreviewTopLabels[i].setStyleSheet(SURVEYMAKERLABELSTYLE);
            questionPreviewBottomLabels[i].setStyleSheet(SURVEYMAKERLABELSTYLE);
            connect(&questions[i], &SurveyMakerQuestionWithSwitch::valueChanged, &questionPreviews[i], &QWidget::setVisible);
        }
        questionLayout->addStretch(1);
    }
    previewLayout->addStretch(1);

    layout->addItem(new QSpacerItem(0,0), 2, 0);
    layout->setRowStretch(2, 1);
}

SurveyMakerPage::~SurveyMakerPage()
{
    if(numQuestions > 0) {
        delete[] questions;
        delete[] questionPreviewTopLabels;
        delete[] questionPreviewBottomLabels;
        delete[] questionPreviewLayouts;
        delete[] questionPreviews;
    }
    delete layout;
}


IntroPage::IntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    QString title = "<span style=\"color: #" GRUEPRDARKBLUEHEX "\">";
    for(int i = 0, j = SurveyMakerWizard::pageNames.count(); i < j; i++) {
        if(i > 0) {
            title += " &ensp;|&ensp; ";
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

    auto *bannerLeft = new QLabel(this);
    bannerLeft->setStyleSheet("border-image: url(:/icons_new/surveyMakerWizardTopLabelBackground.png);");
    bannerLeft->setMinimumSize(0, 120);
    bannerLeft->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *bannerRight = new QLabel(this);
    bannerRight->setStyleSheet("border-image: url(:/icons_new/surveyMakerWizardTopLabelBackground.png);");
    bannerRight->setMinimumSize(0, 120);
    bannerRight->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    banner = new QLabel(this);
    banner->setStyleSheet("border-image: url(:/icons_new/surveyMakerWizardBanner.png);");
    banner->setAlignment(Qt::AlignCenter);
    banner->setScaledContents(true);
    QPixmap bannerpixmap(":/icons_new/surveyMakerWizardBanner.png");
    banner->setMinimumSize(bannerpixmap.width()*120/bannerpixmap.height(), 120);
    banner->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto *bannerWidg = new QWidget(this);
    auto *bannerLayout = new QHBoxLayout(bannerWidg);
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
    int row = 0;
    layout->addWidget(pageTitle, row++, 0, 1, -1);
    layout->addWidget(bannerWidg, row++, 0, 1, -1);
    layout->setRowMinimumHeight(row++, 20);
    layout->addWidget(topLabel, row++, 1, Qt::AlignLeft | Qt::AlignBottom);
    layout->setRowMinimumHeight(row++, 10);
    layout->addWidget(surveyTitle, row++, 1);
    layout->setRowMinimumHeight(row++, 10);
    layout->addWidget(bottomLabel, row++, 1, Qt::AlignLeft | Qt::AlignTop);
    layout->setRowMinimumHeight(row++, 20);
    layout->addWidget(getStartedButton, row++, 1);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 3);
    layout->setColumnStretch(2, 1);
    layout->setSpacing(0);
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
    questions[firstname].setLabel(tr("First name"));
    questionPreviewTopLabels[firstname].setText(tr("First name"));
    questionPreviewLayouts[firstname].addWidget(&questionPreviewTopLabels[firstname]);
    fn = new QLineEdit(FIRSTNAMEQUESTION);
    fn->setCursorPosition(0);
    fn->setStyleSheet(SURVEYMAKERLINEEDITSTYLE);
    questionPreviewLayouts[firstname].addWidget(fn);
    questionPreviewBottomLabels[firstname].setText("");
    //questionPreviewLayouts[firstname].addWidget(&questionPreviewBottomLabels[firstname]);
    questionPreviews[firstname].hide();
    registerField("FirstName", &questions[firstname], "value", "valueChanged");

    questions[lastname].setLabel(tr("Last name"));
    questionPreviewTopLabels[lastname].setText(tr("Last name"));
    questionPreviewLayouts[lastname].addWidget(&questionPreviewTopLabels[lastname]);
    ln = new QLineEdit(LASTNAMEQUESTION);
    ln->setCursorPosition(0);
    ln->setStyleSheet(SURVEYMAKERLINEEDITSTYLE);
    questionPreviewLayouts[lastname].addWidget(ln);
    questionPreviewBottomLabels[lastname].setText("");
    //questionPreviewLayouts[lastname].addWidget(&questionPreviewBottomLabels[lastname]);
    questionPreviews[lastname].hide();
    registerField("LastName", &questions[lastname], "value", "valueChanged");

    questions[email].setLabel(tr("Email"));
    questionPreviewTopLabels[email].setText(tr("Email"));
    questionPreviewLayouts[email].addWidget(&questionPreviewTopLabels[email]);
    em = new QLineEdit(EMAILQUESTION);
    em->setCursorPosition(0);
    em->setStyleSheet(SURVEYMAKERLINEEDITSTYLE);
    questionPreviewLayouts[email].addWidget(em);
    questionPreviewBottomLabels[email].setText("");
    //questionPreviewLayouts[email].addWidget(&questionPreviewBottomLabels[email]);
    questionPreviews[email].hide();
    registerField("Email", &questions[email], "value", "valueChanged");

    questions[gender].setLabel(tr("Gender"));
    connect(&questions[gender], &SurveyMakerQuestionWithSwitch::valueChanged, this, &DemographicsPage::update);
    auto *genderResponses = new QWidget;
    auto *genderResponsesLayout = new QHBoxLayout(genderResponses);
    genderResponsesLabel = new QLabel(tr("Ask as: "));
    genderResponsesLabel->setStyleSheet(SURVEYMAKERLABELSTYLE);
    genderResponsesLabel->setEnabled(false);
    genderResponsesLayout->addWidget(genderResponsesLabel);
    genderResponsesComboBox = new QComboBox;
    genderResponsesComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    genderResponsesComboBox->installEventFilter(new MouseWheelBlocker(genderResponsesComboBox));
    genderResponsesComboBox->addItems({tr("Biological Sex"), tr("Adult Identity"), tr("Child Identity"), tr("Pronouns")});
    genderResponsesComboBox->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
    genderResponsesComboBox->setEnabled(false);
    genderResponsesComboBox->setCurrentIndex(3);
    genderResponsesLayout->addWidget(genderResponsesComboBox);
    genderResponsesLayout->addStretch(1);
    questions[gender].addWidget(genderResponses, 1, 0, true);
    connect(genderResponsesComboBox, &QComboBox::currentIndexChanged, this, &DemographicsPage::update);
    questionPreviewTopLabels[gender].setText(tr("Gender"));
    questionPreviewLayouts[gender].addWidget(&questionPreviewTopLabels[gender]);
    ge = new QComboBox;
    ge->addItem(PRONOUNQUESTION);
    ge->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
    questionPreviewLayouts[gender].addWidget(ge);
    questionPreviewBottomLabels[gender].setText(tr("Options: ") + QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    questionPreviewLayouts[gender].addWidget(&questionPreviewBottomLabels[gender]);
    questionPreviews[gender].hide();
    registerField("Gender", &questions[gender], "value", "valueChanged");
    registerField("genderOptions", genderResponsesComboBox);

    questions[urm].setLabel(tr("Race / ethnicity"));
    questionPreviewTopLabels[urm].setText(tr("Race / ethnicity"));
    questionPreviewLayouts[urm].addWidget(&questionPreviewTopLabels[urm]);
    re = new QLineEdit(URMQUESTION);
    re->setCursorPosition(0);
    re->setStyleSheet(SURVEYMAKERLINEEDITSTYLE);
    questionPreviewLayouts[urm].addWidget(re);
    questionPreviewBottomLabels[urm].setText("");
    //questionPreviewLayouts[urm].addWidget(&questionPreviewBottomLabels[urm]);
    questionPreviews[urm].hide();
    registerField("RaceEthnicity", &questions[urm], "value", "valueChanged");

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
    wiz->button(QWizard::CancelButton)->setStyleSheet(NEXTBUTTONSTYLE);
}

void DemographicsPage::update()
{
    genderResponsesLabel->setEnabled((&questions[gender])->getValue());
    genderResponsesComboBox->setEnabled((&questions[gender])->getValue());
    ge->clear();
    GenderType genderType = static_cast<GenderType>(genderResponsesComboBox->currentIndex());
    if(genderType == GenderType::biol)
    {
        ge->addItem(GENDERQUESTION);
        (&questionPreviewBottomLabels[gender])->setText(tr("Options: ") + QString(BIOLGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else if(genderType == GenderType::adult)
    {
        ge->addItem(GENDERQUESTION);
        (&questionPreviewBottomLabels[gender])->setText(tr("Options: ") + QString(ADULTGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else if(genderType == GenderType::child)
    {
        ge->addItem(GENDERQUESTION);
        (&questionPreviewBottomLabels[gender])->setText(tr("Options: ") + QString(CHILDGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else //if(genderType == GenderType::pronoun)
    {
        ge->addItem(PRONOUNQUESTION);
        (&questionPreviewBottomLabels[gender])->setText(tr("Options: ") + QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
}


MultipleChoicePage::MultipleChoicePage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::multichoice, 0, parent)
{
    registerField("multiChoiceQuestions", this, "questionList", "questionListChanged");

    sampleQuestionsFrame = new QFrame(this);
    sampleQuestionsFrame->setStyleSheet("background-color: #" GRUEPRYELLOWHEX "; color: #" GRUEPRDARKBLUEHEX ";");
    sampleQuestionsIcon = new QLabel;
    sampleQuestionsIcon->setPixmap(QPixmap(":/icons_new/lightbulb.png").scaled(20,20,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    sampleQuestionsLabel = new QLabel(tr("Unsure of what to ask? Take a look at some example questions!"));
    sampleQuestionsLabel->setStyleSheet(SURVEYMAKERLABELSTYLE);
    sampleQuestionsLabel->setWordWrap(true);
    sampleQuestionsLayout = new QHBoxLayout(sampleQuestionsFrame);
    sampleQuestionsButton = new QPushButton(tr("View Examples"));
    sampleQuestionsButton->setStyleSheet(EXAMPLEBUTTONSTYLE);
    sampleQuestionsDialog = new QDialog(this);
    connect(sampleQuestionsButton, &QPushButton::clicked, sampleQuestionsDialog, &QDialog::show);
    sampleQuestionsLayout->addWidget(sampleQuestionsIcon, 0, Qt::AlignLeft | Qt::AlignVCenter);
    sampleQuestionsLayout->addWidget(sampleQuestionsLabel, 1, Qt::AlignVCenter);
    sampleQuestionsLayout->addWidget(sampleQuestionsButton, 0, Qt::AlignRight | Qt::AlignVCenter);
    questionLayout->addWidget(sampleQuestionsFrame);

    questionLayout->addSpacing(10);

    addQuestionFrame = new QFrame(this);
    addQuestionFrame->setStyleSheet("background-color: #" GRUEPRVERYLIGHTBLUEHEX "; color: #" GRUEPRDARKBLUEHEX ";");
    addQuestionLayout = new QHBoxLayout(addQuestionFrame);
    addQuestionButton = new QPushButton;
    addQuestionButton->setStyleSheet(ADDBUTTONSTYLE);
    addQuestionButton->setText(tr("Create another question"));
    addQuestionButton->setIcon(QIcon(":/icons_new/addButton.png"));
    connect(addQuestionButton, &QPushButton::clicked, this, &MultipleChoicePage::addQuestion);
    addQuestionLayout->addWidget(addQuestionButton, 0, Qt::AlignVCenter);
    questionLayout->addWidget(addQuestionFrame);
    questionLayout->addStretch(1);

    questionList.reserve(15);

    addQuestion();
    addQuestion();
}


void MultipleChoicePage::setQuestionList(const QList<int> &newQuestionList)
{
    QList<int> questionList = newQuestionList;
    emit questionListChanged(questionList);
}

QList<int> MultipleChoicePage::getQuestionList() const
{
    return questionList;
}

void MultipleChoicePage::addQuestion()
{
    static int numQuestionsEntered = 0;  // used to set the row in the grid layout and the field numbers (since these can be added but never removed)

    //add the question
    multichoiceQuestions.append(new SurveyMakerMultichoiceQuestion(questionList.size() + 1));
    multichoiceQuestions[numQuestionsEntered]->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    multichoiceQuestions[numQuestionsEntered]->installEventFilter(new MouseWheelBlocker(multichoiceQuestions[numQuestionsEntered]));

    questionLayout->insertSpacing((2 * numQuestionsEntered) + 1, 10);
    questionLayout->insertWidget((2 * numQuestionsEntered) + 2, multichoiceQuestions[numQuestionsEntered]);

    registerField("Question" + QString::number(numQuestionsEntered), multichoiceQuestions[numQuestionsEntered], "question", "questionChanged");
    registerField("Responses" + QString::number(numQuestionsEntered), multichoiceQuestions[numQuestionsEntered], "responses", "responsesChanged");
    registerField("MultiResponsesAllowed" + QString::number(numQuestionsEntered), multichoiceQuestions[numQuestionsEntered], "multi", "multiChanged");

    //add the preview
    questionPreviews = new QWidget;
    questionPreviews->setAttribute(Qt::WA_TransparentForMouseEvents);
    questionPreviews->setFocusPolicy(Qt::NoFocus);
    questionPreviewLayouts = new QVBoxLayout;
    questionPreviews->setLayout(questionPreviewLayouts);
    QString fillInQuestion = "[" + tr("Question") + " " + QString::number(questionList.size() + 1) + "]";
    questionPreviewTopLabels = new QLabel(fillInQuestion);
    questionPreviewTopLabels->setStyleSheet(SURVEYMAKERLABELSTYLE);
    questionPreviewLayouts->addWidget(questionPreviewTopLabels);
    qu.append(new QComboBox);
    qu[numQuestionsEntered]->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
    qu[numQuestionsEntered]->setEditable(true);
    qu[numQuestionsEntered]->setCurrentText(fillInQuestion);
    questionPreviewLayouts->addWidget(qu[numQuestionsEntered]);
    questionPreviewBottomLabels = new QLabel(tr("Options") + ": ---");
    questionPreviewBottomLabels->setStyleSheet(SURVEYMAKERLABELSTYLE);
    questionPreviewLayouts->addWidget(questionPreviewBottomLabels);

    connect(multichoiceQuestions[numQuestionsEntered], &SurveyMakerMultichoiceQuestion::questionChanged, questionPreviewTopLabels, &QLabel::setText);
    connect(multichoiceQuestions[numQuestionsEntered], &SurveyMakerMultichoiceQuestion::questionChanged, qu[numQuestionsEntered], &QComboBox::setCurrentText);
    connect(multichoiceQuestions[numQuestionsEntered], &SurveyMakerMultichoiceQuestion::responsesAsStringChanged, questionPreviewBottomLabels, &QLabel::setText);
    connect(multichoiceQuestions[numQuestionsEntered], &SurveyMakerMultichoiceQuestion::deleteRequested,
            this, [this, numQuestionsEntered = numQuestionsEntered]{deleteAQuestion(numQuestionsEntered);});

    previewLayout->insertWidget(numQuestionsEntered, questionPreviews);

    questionList << numQuestionsEntered;
    numQuestionsEntered++;

    if(questionList.size() == 14) {
        addQuestionButton->setEnabled(false);
        addQuestionButton->setToolTip(tr("Maximum number of questions reached."));
    }
}

void MultipleChoicePage::deleteAQuestion(int questionNum)
{
    questionLayout->itemAt((2 * questionNum) + 1)->spacerItem()->changeSize(0,0,QSizePolicy::Maximum,QSizePolicy::Maximum);
    questionLayout->itemAt((2 * questionNum) + 2)->widget()->hide();
    previewLayout->itemAt(questionNum)->widget()->hide();
    setField("Question" + QString::number(questionNum), QString());
    setField("Responses" + QString::number(questionNum), QStringList());
    setField("MultiResponsesAllowed" + QString::number(questionNum), false);

    questionList.removeAll(questionNum);

    int i = 1;
    for(auto question : questionList) {
        multichoiceQuestions[question]->setNumber(i++);
        //now, to reset the question preview texts if the question is empty
        if(multichoiceQuestions[question]->getQuestion().isEmpty()) {
            multichoiceQuestions[question]->setQuestion(".");
            multichoiceQuestions[question]->setQuestion("");
        }
    }

    if(questionList.size() < 14) {
        addQuestionButton->setEnabled(true);
        addQuestionButton->setToolTip("");
    }
}


SchedulePage::SchedulePage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::schedule, 2, parent)
{
    questions[timezone].setLabel(tr("Timezone"));
    connect(&questions[timezone], &SurveyMakerQuestionWithSwitch::valueChanged, this, &SchedulePage::update);
    questionPreviewTopLabels[timezone].setText(tr("Timezone"));
    questionPreviewLayouts[timezone].addWidget(&questionPreviewTopLabels[timezone]);
    tz = new QComboBox;
    tz->addItem(TIMEZONEQUESTION);
    tz->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
    questionPreviewLayouts[timezone].addWidget(tz);
    questionPreviewBottomLabels[timezone].setText(tr("Dropdown options: List of global timezones"));
    questionPreviewLayouts[timezone].addWidget(&questionPreviewBottomLabels[timezone]);
    questionPreviews[timezone].hide();
    registerField("Timezone", &questions[timezone], "value", "valueChanged");

    questions[schedule].setLabel(tr("Schedule"));
    connect(&questions[schedule], &SurveyMakerQuestionWithSwitch::valueChanged, this, &SchedulePage::update);
    questionPreviewTopLabels[schedule].setText(QString(SCHEDULEQUESTION1) + SCHEDULEQUESTION2FREE + SCHEDULEQUESTION3);
    questionPreviewLayouts[schedule].addWidget(&questionPreviewTopLabels[schedule]);
    sc = new QWidget;
    scLayout = new QGridLayout(sc);
    for(int hr = 0; hr < 24; hr++) {
        auto *rowLabel = new QLabel(sunday.time().addSecs(hr * 3600).toString("h A"));
        rowLabel->setStyleSheet(SURVEYMAKERLABELSTYLE);
        scLayout->addWidget(rowLabel, hr+1, 0);
    }
    for(int day = 0; day < 7; day++) {
        auto *colLabel = new QLabel(sunday.addDays(day).toString("ddd"));
        colLabel->setStyleSheet(SURVEYMAKERLABELSTYLE);
        scLayout->addWidget(colLabel, 0, day+1);
    }
    for(int hr = 1; hr <= 24; hr++) {
        for(int day = 1; day <= 7; day++) {
            auto check = new QCheckBox;
            check->setChecked(true);
            check->setStyleSheet(SURVEYMAKERCHECKBOXSTYLE);
            scLayout->addWidget(check, hr, day);
        }
    }
    scLayout->setSpacing(3);
    scLayout->setColumnStretch(7, 1);
    scLayout->setRowStretch(24, 1);
    questionPreviewLayouts[schedule].addWidget(sc);
    questionPreviewBottomLabels[schedule].setText(tr(""));
    //questionPreviewLayouts[schedule].addWidget(&questionPreviewBottomLabels[schedule]);
    questionPreviews[schedule].hide();
    registerField("Schedule", &questions[schedule], "value", "valueChanged");

    //subItems inside schedule question
    int row = 1;

    auto *busyOrFree = new QWidget;
    auto *busyOrFreeLayout = new QHBoxLayout(busyOrFree);
    busyOrFreeLabel = new QLabel(tr("Ask as: "));
    busyOrFreeLabel->setStyleSheet(SURVEYMAKERLABELSTYLE);
    busyOrFreeLabel->setEnabled(false);
    busyOrFreeLayout->addWidget(busyOrFreeLabel);
    busyOrFreeComboBox = new QComboBox;
    busyOrFreeComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    busyOrFreeComboBox->installEventFilter(new MouseWheelBlocker(busyOrFreeComboBox));
    busyOrFreeComboBox->addItems({tr("Free"), tr("Busy")});
    busyOrFreeComboBox->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
    busyOrFreeComboBox->setEnabled(false);
    busyOrFreeComboBox->setCurrentIndex(0);
    busyOrFreeLayout->addWidget(busyOrFreeComboBox);
    busyOrFreeLayout->addStretch(1);
    questions[schedule].addWidget(busyOrFree, row++, 0, true);
    connect(busyOrFreeComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleBusyOrFree", busyOrFreeComboBox);

    baseTimezoneLabel = new QLabel(tr("Select timezone"));
    baseTimezoneLabel->setStyleSheet(SURVEYMAKERLABELSTYLE);
    baseTimezoneLabel->hide();
    questions[schedule].addWidget(baseTimezoneLabel, row++, 0, false);
    timeZoneNames = QString(TIMEZONENAMES).split(";");
    for(auto &timeZoneName : timeZoneNames)
    {
        timeZoneName.remove('"');
    }
    baseTimezoneComboBox = new ComboBoxWithElidedContents("Pacific: US and Canada, Tijuana [GMT-08:00]", this);
    baseTimezoneComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    baseTimezoneComboBox->installEventFilter(new MouseWheelBlocker(baseTimezoneComboBox));
    baseTimezoneComboBox->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
    baseTimezoneComboBox->setToolTip(tr("<html>Description of the timezone students should use to interpret the times in the grid.&nbsp;"
                                        "<b>Be aware how the meaning of the times in the grid changes depending on this setting.</b></html>"));
    baseTimezoneComboBox->insertItem(TimezoneType::noneOrHome, tr("[no timezone given]"));
    baseTimezoneComboBox->insertSeparator(TimezoneType::noneOrHome+1);
    baseTimezoneComboBox->insertItem(TimezoneType::custom, tr("Custom timezone:"));
    baseTimezoneComboBox->insertSeparator(TimezoneType::custom+1);
    for(int zone = 0; zone < timeZoneNames.size(); zone++)
    {
        const QString &zonename = timeZoneNames.at(zone);
        baseTimezoneComboBox->insertItem(TimezoneType::set + zone, zonename);
        baseTimezoneComboBox->setItemData(TimezoneType::set + zone, zonename, Qt::ToolTipRole);
    }
    questions[schedule].addWidget(baseTimezoneComboBox, row++, 0, true);
    baseTimezoneComboBox->hide();
    connect(baseTimezoneComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleTimezone", baseTimezoneComboBox);

    timespanLabel = new QLabel(tr("Timespan:"));
    timespanLabel->setStyleSheet(SURVEYMAKERLABELSTYLE);
    timespanLabel->setEnabled(false);
    questions[schedule].addWidget(timespanLabel, row++, 0, false);
    daysComboBox = new QComboBox;
    daysComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    daysComboBox->installEventFilter(new MouseWheelBlocker(daysComboBox));
    daysComboBox->addItems({tr("All days"), tr("Weekdays"), tr("Weekends"), tr("Custom days/daynames")});
    daysComboBox->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
    daysComboBox->setEnabled(false);
    daysComboBox->setCurrentIndex(0);
    questions[schedule].addWidget(daysComboBox, row++, 0, true, Qt::AlignLeft);
    connect(daysComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleDays", daysComboBox);

    auto *fromTo = new QWidget;
    auto *fromToLayout = new QHBoxLayout(fromTo);
    fromLabel = new QLabel(tr("From"));
    fromLabel->setStyleSheet(SURVEYMAKERLABELSTYLE);
    fromLabel->setEnabled(false);
    fromToLayout->addWidget(fromLabel, 0, Qt::AlignCenter);
    fromComboBox = new QComboBox;
    fromComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    fromComboBox->installEventFilter(new MouseWheelBlocker(fromComboBox));
    fromComboBox->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
    fromComboBox->setEnabled(false);
    fromToLayout->addWidget(fromComboBox, 0, Qt::AlignCenter);
    toLabel = new QLabel(tr("to"));
    toLabel->setStyleSheet(SURVEYMAKERLABELSTYLE);
    toLabel->setEnabled(false);
    fromToLayout->addWidget(toLabel, 0, Qt::AlignCenter);
    toComboBox = new QComboBox;
    toComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    toComboBox->installEventFilter(new MouseWheelBlocker(toComboBox));
    toComboBox->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
    toComboBox->setEnabled(false);
    for(int hr = 0; hr < 24; hr++) {
        QString time = sunday.time().addSecs(hr * 3600).toString("h A");
        fromComboBox->addItem(time);
        toComboBox->addItem(time);
    }
    fromComboBox->setCurrentIndex(STANDARDSCHEDSTARTTIME);
    toComboBox->setCurrentIndex(STANDARDSCHEDENDTIME);
    fromToLayout->addWidget(toComboBox, 0, Qt::AlignCenter);
    fromToLayout->addStretch(1);
    questions[schedule].addWidget(fromTo, row++, 0, true);
    connect(fromComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleFrom", fromComboBox);
    connect(toComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleTo", toComboBox);

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
                           ((col == 0) || (daysComboBox->currentIndex() == 0) || ((daysComboBox->currentIndex() == 1) && (col > 1) && (col < 7))
                                                                              || ((daysComboBox->currentIndex() == 2) && ((col == 1) || (col == 7)))));
    }
    if(fromComboBox->currentIndex() > toComboBox->currentIndex()) {
        fromComboBox->setStyleSheet(ERRORCOMBOBOXSTYLE);
        toComboBox->setStyleSheet(ERRORCOMBOBOXSTYLE);
    }
    else {
        fromComboBox->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
        toComboBox->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
    }

    bool scheduleOn = questions[schedule].getValue();
    bool timezoneOn = questions[timezone].getValue();

    baseTimezoneLabel->setVisible(timezoneOn);
    baseTimezoneLabel->setEnabled(scheduleOn);
    baseTimezoneComboBox->setVisible(timezoneOn);
    baseTimezoneComboBox->setEnabled(scheduleOn);
    baseTimezone = baseTimezoneComboBox->currentText();
    if(baseTimezone == tr("[no timezone given]")) {
        baseTimezone.clear();
    }

    QString previewlabelText = SCHEDULEQUESTION1;
    previewlabelText += ((busyOrFreeComboBox->currentIndex() == 0)? SCHEDULEQUESTION2FREE : SCHEDULEQUESTION2BUSY);
    previewlabelText += SCHEDULEQUESTION3;
    if(timezoneOn && scheduleOn) {
        previewlabelText += SCHEDULEQUESTION4;
        if(baseTimezone.isEmpty())
        {
            previewlabelText += SCHEDULEQUESTIONHOME;
        }
        else
        {
            previewlabelText += baseTimezone;
        }
        previewlabelText += SCHEDULEQUESTION5;
    }
    questionPreviewTopLabels[schedule].setText(previewlabelText);

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
    questions[section].setLabel(tr("Section"));
    connect(&questions[section], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    sectionLineEdits.reserve(10);
    deleteSectionButtons.reserve(10);
    sectionNames.reserve(10);
    sectionLineEdits.append(new QLineEdit);
    sectionLineEdits.append(new QLineEdit);
    deleteSectionButtons.append(new QPushButton);
    deleteSectionButtons.append(new QPushButton);
    addSectionButton = new QPushButton;
    sectionLineEdits[0]->setStyleSheet(SURVEYMAKERLINEEDITSTYLE);
    sectionLineEdits[1]->setStyleSheet(SURVEYMAKERLINEEDITSTYLE);
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
    questions[section].addWidget(sectionLineEdits[0], 1, 0, false);
    questions[section].addWidget(sectionLineEdits[1], 2, 0, false);
    questions[section].addWidget(deleteSectionButtons[0], 1, 1, false);
    questions[section].addWidget(deleteSectionButtons[1], 2, 1, false);
    questions[section].addWidget(addSectionButton, 3, 0, false, Qt::AlignLeft);
    connect(sectionLineEdits[0], &QLineEdit::textChanged, this, &CourseInfoPage::update);
    connect(sectionLineEdits[1], &QLineEdit::textChanged, this, &CourseInfoPage::update);
    connect(deleteSectionButtons[0], &QPushButton::clicked, this, [this]{deleteASection(0);});
    connect(deleteSectionButtons[1], &QPushButton::clicked, this, [this]{deleteASection(1);});
    connect(addSectionButton, &QPushButton::clicked, this, &CourseInfoPage::addASection);

    questionPreviewTopLabels[section].setText(tr("Section"));
    questionPreviewLayouts[section].addWidget(&questionPreviewTopLabels[section]);
    sc = new QComboBox;
    sc->addItem(SECTIONQUESTION);
    sc->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
    questionPreviewLayouts[section].addWidget(sc);
    questionPreviewLayouts[section].addWidget(&questionPreviewBottomLabels[section]);
    questionPreviews[section].hide();
    registerField("Section", &questions[section], "value", "valueChanged");
    registerField("SectionNames", this, "sectionNames", "sectionNamesChanged");

    questions[wantToWorkWith].setLabel(tr("Classmates I want to work with"));
    connect(&questions[wantToWorkWith], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    questionPreviewTopLabels[wantToWorkWith].setText(tr("Classmates"));
    questionPreviewLayouts[wantToWorkWith].addWidget(&questionPreviewTopLabels[wantToWorkWith]);
    ww = new QLineEdit(PREF1TEAMMATEQUESTION);
    ww->setCursorPosition(0);
    ww->setStyleSheet(SURVEYMAKERLINEEDITSTYLE);
    wwc = new QComboBox;
    wwc->addItem(tr("Select a classmate you would like to work with"));
    wwc->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
    questionPreviewLayouts[wantToWorkWith].addWidget(ww);
    questionPreviewLayouts[wantToWorkWith].addWidget(wwc);
    questionPreviewBottomLabels[wantToWorkWith].setText(tr(""));
    questionPreviewLayouts[wantToWorkWith].addWidget(&questionPreviewBottomLabels[wantToWorkWith]);
    wwc->hide();
    questionPreviews[wantToWorkWith].hide();
    questionPreviewBottomLabels[wantToWorkWith].hide();
    registerField("PrefTeammate", &questions[wantToWorkWith], "value", "valueChanged");

    questionLayout->removeItem(questionLayout->itemAt(4));

    questions[wantToAvoid].setLabel(tr("Classmates I want to avoid"));
    connect(&questions[wantToAvoid], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    questionPreviewTopLabels[wantToAvoid].setText(tr("Classmates"));
    questionPreviewLayouts[wantToAvoid].addWidget(&questionPreviewTopLabels[wantToAvoid]);
    wa = new QLineEdit(PREF1NONTEAMMATEQUESTION);
    wa->setCursorPosition(0);
    wa->setStyleSheet(SURVEYMAKERLINEEDITSTYLE);
    wac = new QComboBox;
    wac->addItem(tr("Select a classmate you would like to avoid working with"));
    wac->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
    questionPreviewLayouts[wantToAvoid].addWidget(wa);
    questionPreviewLayouts[wantToAvoid].addWidget(wac);
    questionPreviewBottomLabels[wantToAvoid].setText(tr(""));
    questionPreviewLayouts[wantToAvoid].addWidget(&questionPreviewBottomLabels[wantToAvoid]);
    wac->hide();
    questionPreviewBottomLabels[wantToWorkWith].hide();
    questionPreviews[wantToAvoid].hide();
    registerField("PrefNonTeammate", &questions[wantToAvoid], "value", "valueChanged");

    questionLayout->removeItem(questionLayout->itemAt(5));

    questions[selectFromList].setLabel(tr("Select from list of classmates"));
    connect(&questions[selectFromList], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    uploadExplainer = new QLabel(tr("You can upload a list of classmates so that students select names rather than typing as a free response question"));
    uploadExplainer->setWordWrap(true);
    uploadExplainer->setStyleSheet(SURVEYMAKERLABELSTYLE);
    uploadButton = new QPushButton;
    uploadButton->setStyleSheet(ADDBUTTONSTYLE);
    uploadButton->setText(tr("Upload class roster"));
    uploadButton->setIcon(QIcon(":/icons_new/addButton.png"));
    uploadButton->setEnabled(false);
    connect(uploadButton, &QPushButton::clicked, this, &CourseInfoPage::uploadRoster);
    questions[selectFromList].addWidget(uploadExplainer, 1, 0, true);
    questions[selectFromList].addWidget(uploadButton, 2, 0, false, Qt::AlignLeft);
    questionPreviews[selectFromList].hide();
    registerField("StudentNames", this, "studentNames", "studentNamesChanged");

    update();
}

void CourseInfoPage::initializePage()
{
    wizard()->setButtonText(QWizard::NextButton, "View Preview");
}

void CourseInfoPage::cleanupPage()
{
    wizard()->setButtonText(QWizard::NextButton, "Next Step  \u2B62");
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
        sectionLineEdit->setEnabled(questions[section].getValue());
        deleteSectionButtons[i++]->setEnabled((questions[section].getValue()) && (lastFilledSection > 1));
    }
    addSectionButton->setEnabled(questions[section].getValue());
    questionPreviewBottomLabels[section].setText(tr("Options: ") + sectionNames.join("  |  "));
    emit sectionNamesChanged(sectionNames);

    uploadButton->setEnabled(questions[selectFromList].isEnabled() && questions[selectFromList].getValue());

    questionPreviewTopLabels[wantToAvoid].setHidden(questions[wantToWorkWith].getValue() && questions[wantToAvoid].getValue());
    if(!questions[selectFromList].getValue() || studentNames.isEmpty()) {
        ww->show();
        wa->show();
        wwc->hide();
        wac->hide();
        questionPreviewBottomLabels[wantToWorkWith].hide();
        questionPreviewBottomLabels[wantToAvoid].hide();
    }
    else {
        ww->hide();
        wa->hide();
        wwc->show();
        wac->show();
        questionPreviewBottomLabels[wantToWorkWith].show();
        questionPreviewBottomLabels[wantToAvoid].show();
        questionPreviewBottomLabels[wantToWorkWith].setHidden(questions[wantToWorkWith].getValue() && questions[wantToAvoid].getValue());
        questionPreviewBottomLabels[wantToWorkWith].setText(tr("Options: ") + studentNames.join("  |  "));
        questionPreviewBottomLabels[wantToAvoid].setText(tr("Options: ") + studentNames.join("  |  "));
    }
}

void CourseInfoPage::addASection()
{
    static int numSectionsEntered = 2;  // used to set the row in the grid layout where the new lineedit goes (since rows can be added but never removed)
    numSectionsEntered++;
    int nextSectionNum = sectionLineEdits.size();   // used to set the location in the QLists of lineedits and pushbuttons
    sectionLineEdits.append(new QLineEdit);
    deleteSectionButtons.append(new QPushButton);
    sectionLineEdits[nextSectionNum]->setStyleSheet(SURVEYMAKERLINEEDITSTYLE);
    deleteSectionButtons[nextSectionNum]->setStyleSheet(DELBUTTONSTYLE);
    sectionLineEdits[nextSectionNum]->setPlaceholderText(tr("Section name"));
    deleteSectionButtons[nextSectionNum]->setText(tr("Delete"));
    deleteSectionButtons[nextSectionNum]->setIcon(QIcon(":/icons_new/trashButton.png"));
    sectionLineEdits[nextSectionNum]->setEnabled(questions[section].getValue());
    questions[section].moveWidget(addSectionButton, numSectionsEntered + 1, 0, false, Qt::AlignLeft);
    questions[section].addWidget(sectionLineEdits[nextSectionNum], numSectionsEntered, 0, false);
    questions[section].addWidget(deleteSectionButtons[nextSectionNum], numSectionsEntered, 1, false);
    connect(sectionLineEdits[nextSectionNum], &QLineEdit::textChanged, this, &CourseInfoPage::update);
    connect(deleteSectionButtons[nextSectionNum], &QPushButton::clicked, this, [this, nextSectionNum]{deleteASection(nextSectionNum);});

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
    : QWizardPage(parent)
{
    setTitle(tr("Survey Preview:"));

    bottomLabel = new QLabel;
    bottomLabel->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(bottomLabel);
    setLayout(layout);
}


void PreviewAndExportPage::initializePage()
{
    QString multis;
    auto multiQuestions = field("multiChoiceQuestions").toList();
    int i = 0;
    for(const auto &multiQuestion : multiQuestions) {
        if(!field("Question" + QString::number(multiQuestion.toInt())).toString().isEmpty()) {
            multis += " Q" + QString::number(++i) + ": " + field("Question" + QString::number(multiQuestion.toInt())).toString() + "\n\tResponses: " + field("Responses" + QString::number(multiQuestion.toInt())).toStringList().join(" / ") + (field("MultiResponsesAllowed" + QString::number(multiQuestion.toInt())).toBool()?" {Multiple Allowed}":"") + "\n";
        }
    }
    bottomLabel->setText("Title: " + field("SurveyTitle").toString() + "\n"
                         "First Name: " + (field("FirstName").toBool()?"Yes":"No") + "\n"
                         "Last Name: " + (field("LastName").toBool()?"Yes":"No") + "\n"
                         "Email: " + (field("Email").toBool()?"Yes":"No") + "\n"
                         "Gender: " + (field("Gender").toBool()?"Yes":"No") + ", " + QString::number(field("genderOptions").toInt()) + "\n"
                         "URM: " + (field("RaceEthnicity").toBool()?"Yes":"No") + "\n"
                         "Multiple choice questions:\n" + multis +
                         "Timezone: " + (field("Timezone").toBool()?"Yes":"No") + "\n"
                         "Schedule: " + (field("Schedule").toBool()?"Yes":"No") + ", " + (field("scheduleBusyOrFree").toBool()?"Busy":"Free") + ", " + (field("scheduleTimezone").toBool()?"w/TZ":"noTZ") + ", " + QString::number(field("scheduleDays").toInt()) + ", " + QString::number(field("scheduleFrom").toInt()) + ", " + QString::number(field("scheduleTo").toInt()) + "\n"
                         "Section: " + (field("Section").toBool()?"Yes":"No") + ": " + (field("SectionNames").toStringList().join(", ")) + "\n"
                         "PrefTeammate: " + (field("PrefTeammate").toBool()?"Yes":"No") + "\n"
                         "PrefNonTeammate: " + (field("PrefNonTeammate").toBool()?"Yes":"No") + "\n"
                         "    Select from: " + (field("StudentNames").toStringList().join(", "))
                         );
}
