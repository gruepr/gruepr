#include "surveyMakerWizard.h"
#include "gruepr_globals.h"
#include "csvfile.h"
#include "widgets/labelWithInstantTooltip.h"
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPainter>
#include <QRadioButton>
#include <QSettings>
#include <QToolTip>

SurveyMakerWizard::SurveyMakerWizard(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(tr("gruepr - Make a survey"));
    setWizardStyle(QWizard::ModernStyle);
    setMinimumWidth(800);
    setMinimumHeight(600);
    setWindowFlag(Qt::CustomizeWindowHint, true);
    setWindowFlag(Qt::WindowCloseButtonHint, false);

    const QSettings savedSettings;
    restoreGeometry(savedSettings.value("surveyMakerWindowGeometry").toByteArray());
    saveFileLocation.setFile(savedSettings.value("saveFileLocation", "").toString());

    setStyleSheet(QString() + COMBOBOXSTYLE + SPINBOXSTYLE + CHECKBOXSTYLE + RADIOBUTTONSTYLE + SCROLLBARSTYLE);

    auto palette = this->palette();
    palette.setColor(QPalette::Window, Qt::white);
    palette.setColor(QPalette::Mid, palette.color(QPalette::Base));
    setPalette(palette);

    setPage(Page::introtitle, new IntroPage);
    setPage(Page::demographics, new DemographicsPage);
    setPage(Page::multichoice, new MultipleChoicePage);
    setPage(Page::schedule, new SchedulePage);
    setPage(Page::courseinfo, new CourseInfoPage);
    setPage(Page::previewexport, new PreviewAndExportPage);

    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::CustomButton1 << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton;
    if(previewPageVisited) {
        buttonLayout << QWizard::CustomButton2;
    }
    setButtonLayout(buttonLayout);

    button(QWizard::CancelButton)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: " DEEPWATERHEX "; "));
    setButtonText(QWizard::CancelButton, QString(LEFTDOUBLEARROW) + "  " + tr("Home"));
    button(QWizard::CustomButton1)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: " DEEPWATERHEX "; "));
    setButtonText(QWizard::CustomButton1, tr("Load Previous Survey"));
    connect(this, &QWizard::customButtonClicked, this, &SurveyMakerWizard::loadSurvey);
    button(QWizard::BackButton)->setStyleSheet(STDBUTTONSTYLE);
    setButtonText(QWizard::BackButton, QString(LEFTARROW) + "  " + tr("Previous Step"));
    setOption(QWizard::NoBackButtonOnStartPage);
    button(QWizard::NextButton)->setStyleSheet(INVISBUTTONSTYLE);
    setButtonText(QWizard::NextButton, tr("Next Step") + "  " + RIGHTARROW);
    button(QWizard::CustomButton2)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: " DEEPWATERHEX "; "));
    setButtonText(QWizard::CustomButton2, tr("Return to Preview") + "  " + RIGHTARROWTOEND);
}

SurveyMakerWizard::~SurveyMakerWizard() {
    QSettings savedSettings;
    savedSettings.setValue("surveyMakerWindowGeometry", saveGeometry());
    savedSettings.setValue("saveFileLocation", saveFileLocation.canonicalFilePath());
}

void SurveyMakerWizard::invalidExpression(QWidget *textWidget, QString &currText, QWidget *parent)
{
    auto *lineEdit = qobject_cast<QLineEdit *>(textWidget);
    if(lineEdit != nullptr) {
        lineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>').remove('/'));
    }

    auto *textEdit = qobject_cast<QTextEdit *>(textWidget);
    if(textEdit != nullptr) {
        if(textEdit != nullptr) {
            textEdit->setText(currText.remove(',').remove('&').remove('<').remove('>').remove('/').remove('\n').remove('\r'));
        }
    }

    QApplication::beep();
    grueprGlobal::errorMessage(parent, tr("Format error"), tr("Sorry, the following punctuation is not allowed:\n"
                                                        "    ,  &  <  > / {enter} \n"
                                                        "Other punctuation is allowed."));
}

void SurveyMakerWizard::loadSurvey(int customButton)
{
    //make sure we got here with custombutton1
    if(customButton != QWizard::CustomButton1) {
        return;
    }

    //read all options from a text file
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), saveFileLocation.canonicalPath(), tr("gruepr survey File (*.gru);;All Files (*)"));
    if( !(fileName.isEmpty()) ) {
        QFile loadFile(fileName);
        if(loadFile.open(QIODevice::ReadOnly)) {
            saveFileLocation.setFile(QFileInfo(fileName).canonicalPath());
            const QJsonDocument loadDoc(QJsonDocument::fromJson(loadFile.readAll()));
            const QJsonObject loadObject = loadDoc.object();

            if(loadObject.contains("Title") && loadObject["Title"].isString()) {
                setField("SurveyTitle", loadObject["Title"].toString());
            }
            if(loadObject.contains("FirstName") && loadObject["FirstName"].isBool()) {
                setField("FirstName", loadObject["FirstName"].toBool());
            }
            if(loadObject.contains("LastName") && loadObject["LastName"].isBool()) {
                setField("LastName", loadObject["LastName"].toBool());
            }
            if(loadObject.contains("Email") && loadObject["Email"].isBool()) {
                setField("Email", loadObject["Email"].toBool());
            }
            if(loadObject.contains("Gender") && loadObject["Gender"].isBool()) {
                setField("Gender", loadObject["Gender"].toBool());
            }
            if(loadObject.contains("GenderType") && loadObject["GenderType"].isDouble()) {
                setField("genderOptions", loadObject["GenderType"].toInt());
            }
            if(loadObject.contains("URM") && loadObject["URM"].isBool()) {
                setField("RaceEthnicity", loadObject["URM"].toBool());
            }

            if(loadObject.contains("numAttributes") && loadObject["numAttributes"].isDouble()) {
                const int numMultiChoiceQuestions = loadObject["numAttributes"].toInt();
                setField("multiChoiceNumQuestions", numMultiChoiceQuestions);   // need to set the number before setting the texts, responses, or multis
                QList<QString> multiChoiceQuestionTexts;
                QList<QList<QString>> multiChoiceQuestionResponses;
                QList<bool> multiChoiceQuestionMultis;
                auto responseOptions = QString(RESPONSE_OPTIONS).split(';');
                for(int question = 0; question < numMultiChoiceQuestions; question++) {
                    multiChoiceQuestionTexts << "";
                    if(loadObject.contains("Attribute" + QString::number(question+1)+"Question") &&
                        loadObject["Attribute" + QString::number(question+1)+"Question"].isString()) {
                        multiChoiceQuestionTexts.last() = loadObject["Attribute" + QString::number(question+1)+"Question"].toString();
                    }

                    multiChoiceQuestionResponses << QStringList({""});
                    if(loadObject.contains("Attribute" + QString::number(question+1)+"Response") &&
                        loadObject["Attribute" + QString::number(question+1)+"Response"].isDouble()) {
                        auto responseIndex = loadObject["Attribute" + QString::number(question+1)+"Response"].toInt() - 1;  // "-1" for historical reasons :(
                        if((responseIndex >= 0) && (responseIndex < responseOptions.size()-1)) {
                            multiChoiceQuestionResponses.last() = responseOptions[responseIndex].split('/');
                        }
                        else if(loadObject.contains("Attribute" + QString::number(question+1)+"Options") &&
                             loadObject["Attribute" + QString::number(question+1)+"Options"].isString()) {
                            multiChoiceQuestionResponses.last() = loadObject["Attribute" + QString::number(question+1)+"Options"].toString().split('/');
                        }
                        for(auto &response : multiChoiceQuestionResponses.last()) {
                            response = response.trimmed();
                        }
                    }
                    multiChoiceQuestionMultis << false;
                    if(loadObject.contains("Attribute" + QString::number(question+1)+"AllowMultiResponse") &&
                        loadObject["Attribute" + QString::number(question+1)+"AllowMultiResponse"].isBool()) {
                        multiChoiceQuestionMultis.last() = loadObject["Attribute" + QString::number(question+1)+"AllowMultiResponse"].toBool();
                    }
                }
                setField("multiChoiceQuestionTexts", multiChoiceQuestionTexts);
                setField("multiChoiceQuestionResponses", QVariant::fromValue<QList<QList<QString>>>(multiChoiceQuestionResponses));
                setField("multiChoiceQuestionMultis", QVariant::fromValue<QList<bool>>(multiChoiceQuestionMultis));
            }

            if(loadObject.contains("Timezone") && loadObject["Timezone"].isBool()) {
                setField("Timezone", loadObject["Timezone"].toBool());
            }
            if(loadObject.contains("Schedule") && loadObject["Schedule"].isBool()) {
                setField("Schedule", loadObject["Schedule"].toBool());
            }
            if(loadObject.contains("ScheduleAsBusy") && loadObject["ScheduleAsBusy"].isBool()) {
                setField("scheduleBusyOrFree", loadObject["ScheduleAsBusy"].toBool()? SchedulePage::busy : SchedulePage::free);
            }
            QStringList scheduleDayNames;
            for(int day = 0; day < MAX_DAYS; day++) {
                const QString dayString1 = "scheduleDay" + QString::number(day+1);
                const QString dayString2 = dayString1 + "Name";
                if(loadObject.contains(dayString2) && loadObject[dayString2].isString()) {
                    scheduleDayNames << loadObject[dayString2].toString();
                    //older style was to include day name AND a bool that could turn off the day
                    if(loadObject.contains(dayString1) && loadObject[dayString1].isBool()) {
                        if(!loadObject[dayString1].toBool()) {
                            scheduleDayNames.last() = "";
                        }
                    }
                }
                else {
                    scheduleDayNames << "";
                    if(loadObject.contains(dayString1) && loadObject[dayString1].isBool()) {
                        if(loadObject[dayString1].toBool()) {
                            scheduleDayNames.last() = defaultDayNames.at(day);
                        }
                    }
                }
            }
            setField("scheduleDayNames", scheduleDayNames);
            if(loadObject.contains("scheduleStartHour") && loadObject["scheduleStartHour"].isDouble()) {
                setField("scheduleFrom", loadObject["scheduleStartHour"].toInt());
            }
            if(loadObject.contains("scheduleEndHour") && loadObject["scheduleEndHour"].isDouble()) {
                setField("scheduleTo", loadObject["scheduleEndHour"].toInt());
            }
            if(loadObject.contains("scheduleResolution") && loadObject["scheduleResolution"].isDouble()) {
                setField("scheduleResolution", loadObject["scheduleResolution"].toInt());
            }
            if(loadObject.contains("scheduleTimeFormat") && loadObject["scheduleTimeFormat"].isDouble()) {
                setField("scheduleTimeFormat", loadObject["scheduleTimeFormat"].toInt());
            }
            if(loadObject.contains("baseTimezone") && loadObject["baseTimezone"].isString()) {
                setField("baseTimezone", loadObject["baseTimezone"].toString());
            }
            if(loadObject.contains("scheduleQuestion") && loadObject["scheduleQuestion"].isString()) {
                setField("ScheduleQuestion", loadObject["scheduleQuestion"].toString());
            }
            else {
                setField("ScheduleQuestion", SchedulePage::generateScheduleQuestion(field("scheduleBusyOrFree").toInt() == SchedulePage::busy,
                                                                                    field("Timezone").toBool(),
                                                                                    field("baseTimezone").toString()));
            }

            if(loadObject.contains("Section") && loadObject["Section"].isBool()) {
                setField("Section", loadObject["Section"].toBool());
            }
            if(loadObject.contains("SectionNames") && loadObject["SectionNames"].isString()) {
                setField("SectionNames", loadObject["SectionNames"].toString().split(','));
            }

            if(loadObject.contains("PreferredTeammates") && loadObject["PreferredTeammates"].isBool()) {
                setField("PrefTeammate", loadObject["PreferredTeammates"].toBool());
            }
            if(loadObject.contains("PreferredNonTeammates") && loadObject["PreferredNonTeammates"].isBool()) {
                setField("PrefNonTeammate", loadObject["PreferredNonTeammates"].toBool());
            }
            if(loadObject.contains("numPrefTeammates") && loadObject["numPrefTeammates"].isDouble()) {
                setField("numPrefTeammates", loadObject["numPrefTeammates"].toInt());
            }
            if(loadObject.contains("StudentNames") && loadObject["StudentNames"].isString()) {
                setField("StudentNames", loadObject["StudentNames"].toString().split(','));
            }

            loadFile.close();

            while(currentId() != SurveyMakerWizard::Page::previewexport) {
                next();
            }
        }
        else {
            grueprGlobal::errorMessage(this, tr("File Error"), tr("This file cannot be read."));
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////


SurveyMakerPage::SurveyMakerPage(SurveyMakerWizard::Page page, QWidget *parent)
    : QWizardPage(parent),
    numQuestions(SurveyMakerWizard::numOfQuestionsInPage[page])
{
    QString title = "<span style=\"color: " DEEPWATERHEX "\">";
    for(int i = 0; i < SurveyMakerWizard::pageNames.count(); i++) {
        if(i > 0) {
            title += " &emsp;/&emsp; ";
        }
        title += SurveyMakerWizard::pageNames[i];
        if(i == page) {
            title += "</span><span style=\"color: " OPENWATERHEX "\">";
        }
    }
    title += "</span>";
    QString label = "  ";
    if(page != SurveyMakerWizard::Page::previewexport) {
        label += SurveyMakerWizard::pageNames[page] + " " + tr("Questions");
    }
    else {
        label += "Survey Preview";
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

        if(page != SurveyMakerWizard::Page::multichoice) {
            for(int i = 0; i < numQuestions; i++) {
                questions << new SurveyMakerQuestionWithSwitch;
                questionPreviews << new QWidget;
                questionPreviewLayouts << new QVBoxLayout;
                questionPreviewTopLabels << new QLabel;
                questionPreviewBottomLabels << new QLabel;

                questionLayout->addSpacing(10);
                questionLayout->addWidget(questions.last());

                questionPreviews.last()->setLayout(questionPreviewLayouts.last());
                questionPreviewLayouts.last()->setSpacing(0);
                previewLayout->addWidget(questionPreviews.last());
                questionPreviews.last()->setAttribute(Qt::WA_TransparentForMouseEvents);
                questionPreviews.last()->setFocusPolicy(Qt::NoFocus);
                questionPreviewTopLabels.last()->setStyleSheet(LABELSTYLE);
                questionPreviewTopLabels.last()->setWordWrap(true);
                questionPreviewBottomLabels.last()->setStyleSheet(LABELSTYLE);
                questionPreviewBottomLabels.last()->setWordWrap(true);
                connect(questions.last(), &SurveyMakerQuestionWithSwitch::valueChanged, questionPreviews.last(), &QWidget::setVisible);
            }
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


//////////////////////////////////////////////////////////////////////////////////////////////////


IntroPage::IntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    QString title = "<span style=\"color: " DEEPWATERHEX "\">";
    for(int i = 0, j = SurveyMakerWizard::pageNames.count(); i < j; i++) {
        if(i > 0) {
            title += " &emsp;/&emsp; ";
        }
        title += SurveyMakerWizard::pageNames[i];
        if(i == 0) {
            title += "</span><span style=\"color: " OPENWATERHEX "\">";
        }
    }
    pageTitle = new QLabel(title, this);
    pageTitle->setStyleSheet(TITLESTYLE);
    pageTitle->setAlignment(Qt::AlignCenter);
    pageTitle->setScaledContents(true);
    pageTitle->setMinimumHeight(40);
    pageTitle->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    bannerLeft = new QLabel;
    bannerLeft->setStyleSheet("QLabel {background-color: " DEEPWATERHEX ";}");
    const QPixmap leftPixmap(":/icons_new/BannerLeft.png");
    bannerLeft->setPixmap(leftPixmap.scaledToHeight(120, Qt::SmoothTransformation));
    bannerLeft->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    bannerLeft->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    bannerRight = new QLabel;
    bannerRight->setStyleSheet("QLabel {background-color: " DEEPWATERHEX ";}");
    const QPixmap rightPixmap(":/icons_new/BannerRight.png");
    bannerRight->setPixmap(rightPixmap.scaledToHeight(120, Qt::SmoothTransformation));
    bannerRight->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    bannerRight->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    banner = new QLabel;
    const QString labelText = "<html><body>"
                              "<span style=\"font-family: 'Paytone One'; font-size: 24pt; color: white;\">"
                              "Make a survey with gruepr</span><br>"
                              "<span style=\"font-family: 'DM Sans'; font-size: 16pt; color: white;\">"
                              "Creating optimal grueps is easy! Get started with our five step survey-making flow below.</span>"
                              "</body></html>";
    banner->setText(labelText);
    banner->setAlignment(Qt::AlignCenter);
    banner->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    banner->setWordWrap(true);
    banner->setStyleSheet("QLabel {background-color: " DEEPWATERHEX "; color: white;}");
    auto *bannerLayout = new QHBoxLayout;
    bannerLayout->setSpacing(0);
    bannerLayout->setContentsMargins(0, 0, 0, 0);
    bannerLayout->addWidget(bannerLeft);
    bannerLayout->addWidget(banner);
    bannerLayout->addWidget(bannerRight);

    topLabel = new QLabel(this);
    topLabel->setText("<span style=\"color: " DEEPWATERHEX "; font-size: 12pt; font-family: 'DM Sans';\">" +
                      tr("Survey Name") + "</span>");
    topLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    surveyTitle = new QLineEdit(this);
    surveyTitle->setPlaceholderText(tr("Enter Text"));
    surveyTitle->setStyleSheet("color: " DEEPWATERHEX "; font-size: 14pt; font-family: 'DM Sans'; "
                               "border-style: outset; border-width: 2px; border-color: " DEEPWATERHEX "; ");
    surveyTitle->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    registerField("SurveyTitle", surveyTitle);

    bottomLabel = new QLabel(this);
    bottomLabel->setText("<span style=\"color: " DEEPWATERHEX "; font-size: 10pt; font-family: 'DM Sans'\">" +
                         tr("This will be the name of the survey you send to your students!") + "</span>");
    bottomLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    getStartedButton = new QPushButton("Get Started  " + QString(RIGHTARROW), this);
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
    connect(getStartedButton, &QPushButton::clicked, wizard(), &QWizard::next);
}


//////////////////////////////////////////////////////////////////////////////////////////////////


DemographicsPage::DemographicsPage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::demographics, parent)
{
    questions[firstname]->setLabel(tr("First name"));
    questionPreviewTopLabels[firstname]->setText(FIRSTNAMEQUESTION);
    questionPreviewLayouts[firstname]->addWidget(questionPreviewTopLabels[firstname]);
    fn = new QLineEdit;
    fn->setPlaceholderText(tr("First name"));
    fn->setCursorPosition(0);
    fn->setStyleSheet(LINEEDITSTYLE);
    questionPreviewLayouts[firstname]->addWidget(fn);
    questionPreviewBottomLabels[firstname]->hide();
    questionPreviewLayouts[firstname]->addWidget(questionPreviewBottomLabels[firstname]);
    questionPreviews[firstname]->hide();
    registerField("FirstName", questions[firstname], "value", "valueChanged");

    questions[lastname]->setLabel(tr("Last name"));
    questionPreviewTopLabels[lastname]->setText(LASTNAMEQUESTION);
    questionPreviewLayouts[lastname]->addWidget(questionPreviewTopLabels[lastname]);
    ln = new QLineEdit;
    ln->setPlaceholderText(tr("Last name"));
    ln->setCursorPosition(0);
    ln->setStyleSheet(LINEEDITSTYLE);
    questionPreviewLayouts[lastname]->addWidget(ln);
    questionPreviewBottomLabels[lastname]->hide();
    questionPreviewLayouts[lastname]->addWidget(questionPreviewBottomLabels[lastname]);
    questionPreviews[lastname]->hide();
    registerField("LastName", questions[lastname], "value", "valueChanged");

    questions[email]->setLabel(tr("Email"));
    questionPreviewTopLabels[email]->setText(EMAILQUESTION);
    questionPreviewLayouts[email]->addWidget(questionPreviewTopLabels[email]);
    em = new QLineEdit;
    em->setPlaceholderText(tr("Email"));
    em->setCursorPosition(0);
    em->setStyleSheet(LINEEDITSTYLE);
    questionPreviewLayouts[email]->addWidget(em);
    questionPreviewBottomLabels[email]->hide();
    questionPreviewLayouts[email]->addWidget(questionPreviewBottomLabels[email]);
    questionPreviews[email]->hide();
    registerField("Email", questions[email], "value", "valueChanged");

    questions[gender]->setLabel(tr("Gender"));
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
    genderResponsesComboBox->setMinimumContentsLength(QString(tr("Biological Sex")).size());
    genderResponsesComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    genderResponsesComboBox->setEnabled(false);
    genderResponsesComboBox->setCurrentIndex(3);
    genderResponsesLayout->addWidget(genderResponsesComboBox);
    genderResponsesLayout->addStretch(1);
    questions[gender]->addWidget(genderResponses, 1, 0, true);
    connect(genderResponsesComboBox, &QComboBox::currentIndexChanged, this, &DemographicsPage::update);
    questionPreviewTopLabels[gender]->setText(PRONOUNQUESTION);
    questionPreviewLayouts[gender]->addWidget(questionPreviewTopLabels[gender]);
    connect(questions[gender], &SurveyMakerQuestionWithSwitch::valueChanged, this, &DemographicsPage::update);
    connect(questions[gender], &SurveyMakerQuestionWithSwitch::valueChanged, genderResponsesLabel, &QLabel::setEnabled);
    connect(questions[gender], &SurveyMakerQuestionWithSwitch::valueChanged, genderResponsesComboBox, &QComboBox::setEnabled);
    auto *options = new QGroupBox("");
    options->setStyleSheet("border-style:none;");
    auto *vbox = new QVBoxLayout;
    vbox->setSpacing(0);
    vbox->setContentsMargins(20, 0, 0, 0);
    options->setLayout(vbox);
    auto *topLabel = new QLabel(SELECTONE);
    topLabel->setStyleSheet(LABELSTYLE);
    topLabel->setWordWrap(true);
    vbox->addWidget(topLabel);
    const QStringList genderOptions = QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
    for(const auto &genderOption : genderOptions) {
        ge << new QRadioButton(genderOption);
        vbox->addWidget(ge.last());
    }
    ge.first()->setChecked(true);
    questionPreviewLayouts[gender]->addWidget(options);
    questionPreviewBottomLabels[gender]->hide();
    questionPreviewLayouts[gender]->addWidget(questionPreviewBottomLabels[gender]);
    questionPreviews[gender]->hide();
    registerField("Gender", questions[gender], "value", "valueChanged");
    registerField("genderOptions", genderResponsesComboBox);

    questions[urm]->setLabel(tr("Race / ethnicity"));
    questionPreviewTopLabels[urm]->setText(URMQUESTION);
    questionPreviewLayouts[urm]->addWidget(questionPreviewTopLabels[urm]);
    re = new QLineEdit;
    re->setPlaceholderText(tr("Race / ethnicity / cultural heritage"));
    re->setCursorPosition(0);
    re->setStyleSheet(LINEEDITSTYLE);
    questionPreviewLayouts[urm]->addWidget(re);
    questionPreviewBottomLabels[urm]->hide();
    questionPreviewLayouts[urm]->addWidget(questionPreviewBottomLabels[urm]);
    questionPreviews[urm]->hide();
    registerField("RaceEthnicity", questions[urm], "value", "valueChanged");

    update();

    questions[firstname]->setValue(true);
    questions[lastname]->setValue(true);
}

void DemographicsPage::initializePage()
{
    auto *wiz = qobject_cast<SurveyMakerWizard *>(wizard());
    auto palette = wiz->palette();
    palette.setColor(QPalette::Window, QColor::fromString(DEEPWATERHEX));
    wiz->setPalette(palette);
    wiz->button(QWizard::CancelButton)->setStyleSheet(STDBUTTONSTYLE);
    wiz->button(QWizard::NextButton)->setStyleSheet(NEXTBUTTONSTYLE);
    wiz->button(QWizard::CustomButton2)->setStyleSheet(NEXTBUTTONSTYLE);
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton;
    if(wiz->previewPageVisited) {
        buttonLayout << QWizard::CustomButton2;
    }
    wiz->setButtonLayout(buttonLayout);
}

void DemographicsPage::cleanupPage()
{
    auto *wiz = qobject_cast<SurveyMakerWizard *>(wizard());
    auto palette = wiz->palette();
    palette.setColor(QPalette::Window, Qt::white);
    wiz->setPalette(palette);
    wiz->button(QWizard::CancelButton)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: " DEEPWATERHEX "; "));
    wiz->button(QWizard::NextButton)->setStyleSheet(INVISBUTTONSTYLE);
    wiz->button(QWizard::CustomButton2)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: " DEEPWATERHEX "; "));
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::CustomButton1 << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton;
    if(wiz->previewPageVisited) {
        buttonLayout << QWizard::CustomButton2;
    }
    wiz->setButtonLayout(buttonLayout);
}

void DemographicsPage::update()
{
    auto genderType = static_cast<GenderType>(genderResponsesComboBox->currentIndex());
    QStringList genderOptions;
    QString questionText;
    if(genderType == GenderType::biol) {
        questionText = GENDERQUESTION;
        genderOptions = QString(BIOLGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
    }
    else if(genderType == GenderType::adult) {
        questionText = GENDERQUESTION;
        genderOptions = QString(ADULTGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
    }
    else if(genderType == GenderType::child) {
        questionText = GENDERQUESTION;
        genderOptions = QString(CHILDGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
    }
    else {  //if(genderType == GenderType::pronoun)
        questionText = PRONOUNQUESTION;
        genderOptions = QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
    }
    questionPreviewTopLabels[gender]->setText(questionText);
    int i = 0;
    for(const auto &genderOption : genderOptions) {
        ge[i++]->setText(genderOption);
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////


MultipleChoicePage::MultipleChoicePage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::multichoice, parent)
{
    auto stretch = questionLayout->takeAt(0);   // will put this back at the end of the layout after adding everything
    sampleQuestionsFrame = new QFrame(this);
    sampleQuestionsFrame->setStyleSheet("background-color: " + (QColor::fromString(QString(STARFISHHEX)).lighter(133).name()) + "; color: " DEEPWATERHEX ";");
    sampleQuestionsIcon = new QLabel;
    sampleQuestionsIcon->setPixmap(QPixmap(":/icons_new/lightbulb.png").scaled(20,20,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    sampleQuestionsLabel = new QLabel(tr("Unsure of what to ask? Take a look at some example questions!"));
    sampleQuestionsLabel->setStyleSheet(LABELSTYLE);
    sampleQuestionsLabel->setWordWrap(true);
    sampleQuestionsLayout = new QHBoxLayout(sampleQuestionsFrame);
    sampleQuestionsButton = new QPushButton(tr("View Examples"));
    sampleQuestionsButton->setStyleSheet(EXAMPLEBUTTONSTYLE);
    sampleQuestionsDialog = new SampleQuestionsDialog(this);
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
        const QString fillInQuestion = "[" + tr("Question") + " " + QString::number(i + 1) + "]";
        questionPreviewTopLabels << new QLabel(fillInQuestion);
        questionPreviewTopLabels.last()->setStyleSheet(QString(LABELSTYLE).replace("font-size: 10pt;", "font-size: 12pt;"));
        questionPreviewTopLabels.last()->setWordWrap(true);
        questionPreviewLayouts.last()->addWidget(questionPreviewTopLabels.last());
        questionPreviewLayouts.last()->addWidget(multichoiceQuestions.last()->previewWidget);
        previewSeparators << new QFrame;
        previewSeparators.last()->setStyleSheet("border-color: " DEEPWATERHEX);
        previewSeparators.last()->setLineWidth(1);
        previewSeparators.last()->setMidLineWidth(1);
        previewSeparators.last()->setFrameShape(QFrame::HLine);
        previewSeparators.last()->setFrameShadow(QFrame::Plain);
        questionPreviewLayouts.last()->addWidget(previewSeparators.last());
        previewLayout->insertWidget(i, questionPreviews.last());
        questionPreviews.last()->hide();

        //connect question to delete action and to updating the wizard fields and the preview
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::deleteRequested, this, [this, i]{deleteAQuestion(i);});
        questionTexts << "";
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::questionChanged, this, [this, i, fillInQuestion](const QString &newText)
                                                                                                     {questionTexts[i] = newText;
                                                                                                      questionPreviewTopLabels[i]->setText(newText.isEmpty()? fillInQuestion : newText);});
        questionResponses << QStringList({""});
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::responsesChanged, this, [this, i](const QStringList &newResponses){questionResponses[i] = newResponses;});
        questionMultis << false;
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::multiChanged, this, [this, i](const bool newMulti){questionMultis[i] = newMulti;});
    }

    addQuestionButtonFrame = new QFrame(this);
    addQuestionButtonFrame->setStyleSheet("background-color: " BUBBLYHEX "; color: " DEEPWATERHEX ";");
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

void MultipleChoicePage::initializePage()
{
}

void MultipleChoicePage::cleanupPage()
{
}

void MultipleChoicePage::setNumQuestions(const int newNumQuestions)
{
    while(numQuestions > 0) {
        deleteAQuestion(0);
    }
    while(numQuestions < newNumQuestions) {
        addQuestion();
    }
    emit numQuestionsChanged(numQuestions);
}

int MultipleChoicePage::getNumQuestions() const
{
    return numQuestions;
}

void MultipleChoicePage::setQuestionTexts(const QList<QString> &newQuestionTexts)
{
    int i = 0;
    for(const auto &newQuestionText : newQuestionTexts) {
        if(i >= numQuestions) {
            addQuestion();
        }
        multichoiceQuestions[i]->setQuestion(newQuestionText);
        i++;
    }
    while(i < MAX_ATTRIBUTES-1) {
        multichoiceQuestions[i++]->setQuestion("");
    }
    emit questionTextsChanged(questionTexts);
}

QList<QString> MultipleChoicePage::getQuestionTexts() const
{
    return questionTexts;
}

void MultipleChoicePage::setQuestionResponses(const QList<QList<QString>> &newQuestionResponses)
{
    int i = 0;
    for(const auto &newQuestionResponse : newQuestionResponses) {
        if(i >= numQuestions) {
            addQuestion();
        }
        multichoiceQuestions[i]->setResponses(newQuestionResponse);
        i++;
    }
    while(i < MAX_ATTRIBUTES-1) {
        multichoiceQuestions[i++]->setResponses(QStringList({""}));
    }
    emit questionResponsesChanged(questionResponses);
}

QList<QList<QString>> MultipleChoicePage::getQuestionResponses() const
{
    return questionResponses;
}

void MultipleChoicePage::setQuestionMultis(const QList<bool> &newQuestionMultis)
{
    int i = 0;
    for(const auto &newQuestionMulti : newQuestionMultis) {
        if(i >= numQuestions) {
            addQuestion();
        }
        multichoiceQuestions[i]->setMulti(newQuestionMulti);
        i++;
    }
    while(i < MAX_ATTRIBUTES-1) {
        multichoiceQuestions[i++]->setMulti(false);
    }
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


//////////////////////////////////////////////////////////////////////////////////////////////////


SchedulePage::SchedulePage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::schedule, parent)
{
    questions[timezone]->setLabel(tr("Timezone"));
    connect(questions[timezone], &SurveyMakerQuestionWithSwitch::valueChanged, this, &SchedulePage::update);
    questionPreviewTopLabels[timezone]->setText(TIMEZONEQUESTION);
    questionPreviewLayouts[timezone]->addWidget(questionPreviewTopLabels[timezone]);
    tz = new QComboBox;
    tz->setStyleSheet(COMBOBOXSTYLE);
    //tz->addItem(tr("Select timezone"));
    tz->setPlaceholderText(tr("Select timezone"));
    questionPreviewLayouts[timezone]->addWidget(tz);
    questionPreviewBottomLabels[timezone]->setText(tr("Options: List of global timezones"));
    questionPreviewLayouts[timezone]->addWidget(questionPreviewBottomLabels[timezone]);
    questionPreviews[timezone]->hide();
    registerField("Timezone", questions[timezone], "value", "valueChanged");

    questions[schedule]->setLabel(tr("Schedule"));
    connect(questions[schedule], &SurveyMakerQuestionWithSwitch::valueChanged, this, &SchedulePage::update);
    questionPreviewTopLabels[schedule]->setText(generateScheduleQuestion(false, false, ""));
    questionPreviewLayouts[schedule]->addWidget(questionPreviewTopLabels[schedule]);
    sc = new QWidget;
    scLayout = new QGridLayout(sc);
    for(int time = 0; time < MAX_BLOCKS_PER_DAY; time++) {
        auto *rowLabel = new QLabel(SurveyMakerWizard::sundayMidnight.time().addSecs(time*MIN_SCHEDULE_RESOLUTION*60)
                                                        .toString(timeFormats.at(DEFAULTTIMEFORMAT).first));
        rowLabel->setStyleSheet(LABELSTYLE);
        scLayout->addWidget(rowLabel, time+1, 0);
    }
    for(int day = 0; day < 7; day++) {
        auto *colLabel = new QLabel(SurveyMakerWizard::sundayMidnight.addDays(day).toString("ddd"));
        colLabel->setStyleSheet(LABELSTYLE);
        scLayout->addWidget(colLabel, 0, day+1);
    }
    for(int time = 1; time <= MAX_BLOCKS_PER_DAY; time++) {
        for(int day = 1; day <= 7; day++) {
            auto check = new QCheckBox;
            check->setChecked(true);
            scLayout->addWidget(check, time, day);
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
    busyOrFreeComboBox->insertItem(busy, tr("Busy"));
    busyOrFreeComboBox->insertItem(free, tr("Free"));
    busyOrFreeComboBox->setMinimumContentsLength(std::max(QString(tr("Busy")).size(), QString(tr("Free")).size()));
    busyOrFreeComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
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
    baseTimezoneComboBox = new ComboBoxWithElidedContents("Pacific: US and Canada, Tijuana [GMT-08:00]", this);
    baseTimezoneComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    baseTimezoneComboBox->installEventFilter(new MouseWheelBlocker(baseTimezoneComboBox));
    baseTimezoneComboBox->setToolTip(tr("<html>Description of the timezone students should use to interpret the times in the grid.&nbsp;"
                                        "<b>Be aware how the meaning of the times in the grid changes depending on this setting.</b></html>"));
    baseTimezoneComboBox->addItem(tr("[student's home timezone]"));
    baseTimezoneComboBox->addItem(tr("Custom timezone"));
    baseTimezoneComboBox->insertSeparator(2);
    int itemNum = 3;
    for(const auto &zonename : SurveyMakerWizard::timezoneNames) {
        baseTimezoneComboBox->addItem(zonename);
        baseTimezoneComboBox->setItemData(itemNum++, zonename, Qt::ToolTipRole);
    }
    questions[schedule]->addWidget(baseTimezoneComboBox, row++, 0, true);
    baseTimezoneComboBox->hide();
    connect(baseTimezoneComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    customBaseTimezone = new QLineEdit;
    customBaseTimezone->setStyleSheet(LINEEDITSTYLE);
    customBaseTimezone->setPlaceholderText(tr("Custom timezone"));
    questions[schedule]->addWidget(customBaseTimezone, row++, 0, true);
    customBaseTimezone->hide();
    connect(customBaseTimezone, &QLineEdit::textChanged, this, &SchedulePage::update);
    registerField("baseTimezone", this, "baseTimezone", "baseTimezoneChanged");
    registerField("ScheduleQuestion", this, "scheduleQuestion", "scheduleQuestionChanged");

    auto *timespan = new QWidget;
    auto *timespanLayout = new QVBoxLayout(timespan);
    timespanLayout->setSpacing(2);
    timespanLabel = new QLabel(tr("Timespan:"));
    timespanLabel->setStyleSheet(LABELSTYLE);
    timespanLabel->setEnabled(false);
    timespanLayout->addWidget(timespanLabel);
    daysComboBox = new QComboBox;
    daysComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    daysComboBox->installEventFilter(new MouseWheelBlocker(daysComboBox));
    daysComboBox->addItems({tr("All days"), tr("Weekdays"), tr("Weekends"), tr("Custom days/daynames")});
    daysComboBox->setMinimumContentsLength(QString(tr("Custom days/daynames")).size());
    daysComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    daysComboBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    daysComboBox->setEnabled(false);
    daysComboBox->setCurrentIndex(0);
    timespanLayout->addWidget(daysComboBox);
    connect(daysComboBox, &QComboBox::activated, this, &SchedulePage::daysComboBox_activated);

    //connect subwindow ui to slots
    dayNames.reserve(MAX_DAYS);
    dayCheckBoxes.reserve(MAX_DAYS);
    dayLineEdits.reserve(MAX_DAYS);
    for(int day = 0; day < MAX_DAYS; day++) {
        dayNames << SurveyMakerWizard::defaultDayNames.at(day);
        dayCheckBoxes << new QCheckBox;
        dayLineEdits <<  new QLineEdit;
        dayCheckBoxes.last()->setChecked(true);
        dayLineEdits.last()->setText(dayNames.at(day));
        dayLineEdits.last()->setPlaceholderText(tr("Day ") + QString::number(day + 1) + tr(" name"));
        connect(dayLineEdits.last(), &QLineEdit::textChanged, this, [this, day](const QString &text)
                                        {day_LineEdit_textChanged(text, dayLineEdits[day], dayNames[day]);});
        connect(dayLineEdits.last(), &QLineEdit::editingFinished, this, [this, day]
                                        {if(dayNames.at(day).isEmpty()){dayCheckBoxes[day]->setChecked(false);};});
        connect(dayCheckBoxes.last(), &QCheckBox::toggled, this, [this, day](bool checked)
                                        {day_CheckBox_toggled(checked, dayLineEdits[day], SurveyMakerWizard::defaultDayNames.at(day));});
    }
    daysWindow = new dayNamesDialog(dayCheckBoxes, dayLineEdits, this);
    registerField("scheduleDayNames", this, "dayNames", "dayNamesChanged");

    auto *fromTo = new QWidget;
    auto *fromToLayout = new QHBoxLayout(fromTo);
    fromLabel = new QLabel(tr("From"));
    fromLabel->setStyleSheet(LABELSTYLE);
    fromLabel->setEnabled(false);
    fromToLayout->addWidget(fromLabel);
    fromComboBox = new QComboBox;
    fromComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    fromComboBox->installEventFilter(new MouseWheelBlocker(fromComboBox));
    fromComboBox->setMinimumContentsLength(5);
    fromComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    fromComboBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    fromComboBox->setEnabled(false);
    fromToLayout->addWidget(fromComboBox, 1);
    toLabel = new QLabel(tr("to"));
    toLabel->setStyleSheet(LABELSTYLE);
    toLabel->setEnabled(false);
    fromToLayout->addWidget(toLabel);
    toComboBox = new QComboBox;
    toComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    toComboBox->installEventFilter(new MouseWheelBlocker(toComboBox));
    toComboBox->setMinimumContentsLength(5);
    toComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    toComboBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    toComboBox->setEnabled(false);
    int startindex = 0, endindex = 0;
    for(int time = 0; time < (24 * 60 / MIN_SCHEDULE_RESOLUTION); time++) {
        const float hoursSinceMidnight = time * MIN_SCHEDULE_RESOLUTION / 60.0f;
        const QString timeName = SurveyMakerWizard::sundayMidnight.time().addSecs(hoursSinceMidnight * 3600).toString("h:mm A");
        fromComboBox->addItem(timeName, hoursSinceMidnight);
        toComboBox->addItem(timeName, hoursSinceMidnight);
        if(hoursSinceMidnight == DEFAULTSCHEDSTARTTIME) {
            startindex = time;
        }
        if(hoursSinceMidnight == DEFAULTSCHEDENDTIME) {
            endindex = time;
        }
    }
    fromComboBox->setCurrentIndex(startindex);
    toComboBox->setCurrentIndex(endindex);
    fromToLayout->addWidget(toComboBox, 1);
    timespanLayout->addWidget(fromTo);
    connect(fromComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleFrom", this, "scheduleFrom", "scheduleFromChanged");
    connect(toComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleTo", this, "scheduleTo", "scheduleToChanged");

    auto *resolution = new QWidget;
    auto *resolutionLayout = new QHBoxLayout(resolution);
    resolutionLabel = new QLabel(tr("every"));
    resolutionLabel->setStyleSheet(LABELSTYLE);
    resolutionLabel->setEnabled(false);
    resolutionLayout->addWidget(resolutionLabel);
    resolutionComboBox = new QComboBox;
    resolutionComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    resolutionComboBox->installEventFilter(new MouseWheelBlocker(toComboBox));
    resolutionComboBox->setMinimumContentsLength(11);
    resolutionComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    resolutionComboBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    resolutionComboBox->setEnabled(false);
    for(const auto &[name, value] : resolutionValues) {
        resolutionComboBox->addItem(name, value);
    }
    resolutionComboBox->setCurrentIndex(1);
    resolutionLayout->addWidget(resolutionComboBox, 1);
    timespanLayout->addWidget(resolution);
    connect(resolutionComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleResolution", resolutionComboBox);

    auto *format = new QWidget;
    auto *formatLayout = new QHBoxLayout(format);
    formatLabel = new QLabel(tr("Time format:"));
    formatLabel->setStyleSheet(LABELSTYLE);
    formatLabel->setEnabled(false);
    formatLayout->addWidget(formatLabel);
    formatComboBox = new QComboBox;
    formatComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    formatComboBox->installEventFilter(new MouseWheelBlocker(toComboBox));
    formatComboBox->setMinimumContentsLength(5);
    formatComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    formatComboBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    formatComboBox->setEnabled(false);
    for(const auto &[format, example] : timeFormats) {
        formatComboBox->addItem(example, format);
    }
    formatComboBox->setCurrentIndex(7);
    formatLayout->addWidget(formatComboBox, 1);
    timespanLayout->addWidget(format);
    connect(formatComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleTimeFormat", formatComboBox);

    questions[schedule]->addWidget(timespan, row, 0, true);

    update();
}

void SchedulePage::cleanupPage()
{
}

void SchedulePage::setDayNames(const QStringList &newDayNames)
{
    dayNames = newDayNames;
    while(dayNames.size() < MAX_DAYS) {
        dayNames << "";
    }
    for(int i = 0; i < MAX_DAYS; i++) {
        dayLineEdits[i]->setText(dayNames[i]);
        dayCheckBoxes[i]->setChecked(!dayNames[i].isEmpty());
    }
    emit dayNamesChanged(dayNames);
}

QStringList SchedulePage::getDayNames() const
{
    return dayNames;
}

void SchedulePage::setBaseTimezone(const QString &newBaseTimezone)
{
    const int index = baseTimezoneComboBox->findText(newBaseTimezone, Qt::MatchFixedString);
    if(index != -1) {
        baseTimezoneComboBox->setCurrentIndex(index);
        customBaseTimezone->hide();
    }
    else {
        baseTimezoneComboBox->setCurrentIndex(1);
        customBaseTimezone->show();
        customBaseTimezone->setEnabled(baseTimezoneComboBox->isEnabled());
        customBaseTimezone->setText(newBaseTimezone);
    }
    const QString newScheduleQuestion = generateScheduleQuestion(busyOrFreeComboBox->currentIndex() == busy, questions[timezone]->getValue(), newBaseTimezone);
    setScheduleQuestion(newScheduleQuestion);
    baseTimezone = newBaseTimezone;
    emit baseTimezoneChanged(newBaseTimezone);
}

QString SchedulePage::getBaseTimezone() const
{
    return baseTimezone;
}

void SchedulePage::setScheduleQuestion(const QString &newScheduleQuestion)
{
    scheduleQuestion = newScheduleQuestion;
    questionPreviewTopLabels[schedule]->setText(scheduleQuestion);
    emit scheduleQuestionChanged(scheduleQuestion);
}

QString SchedulePage::getScheduleQuestion() const
{
    return scheduleQuestion;
}

void SchedulePage::setScheduleFrom(const float newScheduleFrom)
{
    // set from combobox to index with data that matches new value
    int index = fromComboBox->findData(newScheduleFrom);
    if(index == -1) {
        // no exact match; find closest
        do {
            index++;
        }
        while(fromComboBox->itemData(index).toFloat() < newScheduleFrom);
    }
    fromComboBox->setCurrentIndex(index);
    emit scheduleFromChanged(newScheduleFrom);
}

float SchedulePage::getScheduleFrom() const
{
    return fromComboBox->currentData().toFloat();
}

void SchedulePage::setScheduleTo(const float newScheduleTo)
{
    // set to combobox to index with data that matches new value
    int index = toComboBox->findData(newScheduleTo);
    if(index == -1) {
        // no exact match; find closest
        do {
            index++;
        }
        while(toComboBox->itemData(index).toFloat() < newScheduleTo);
    }
    toComboBox->setCurrentIndex(index);
    emit scheduleToChanged(newScheduleTo);
}

float SchedulePage::getScheduleTo() const
{
    return toComboBox->currentData().toFloat();
}

void SchedulePage::daysComboBox_activated(int index)
{
    daysComboBox->blockSignals(true);
    if(index == 0) {
        //All Days
        for(auto &dayCheckBox : dayCheckBoxes) {
            dayCheckBox->setChecked(true);
        }
    }
    else if(index == 1) {
        //Weekdays
        dayCheckBoxes[Sun]->setChecked(false);
        for(int day = Mon; day <= Fri; day++) {
            dayCheckBoxes[day]->setChecked(true);
        }
        dayCheckBoxes[Sat]->setChecked(false);
    }
    else if(index == 2) {
        //Weekends
        dayCheckBoxes[Sun]->setChecked(true);
        for(int day = Mon; day <= Fri; day++) {
            dayCheckBoxes[day]->setChecked(false);
        }
        dayCheckBoxes[Sat]->setChecked(true);
    }
    else {
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
    const bool weekends = dayCheckBoxes[Sun]->isChecked() && dayCheckBoxes[Sat]->isChecked();
    const bool noWeekends = !(dayCheckBoxes[Sun]->isChecked() || dayCheckBoxes[Sat]->isChecked());
    const bool weekdays = dayCheckBoxes[Mon]->isChecked() && dayCheckBoxes[Tue]->isChecked() &&
                          dayCheckBoxes[Wed]->isChecked() && dayCheckBoxes[Thu]->isChecked() && dayCheckBoxes[Fri]->isChecked();
    const bool noWeekdays = !(dayCheckBoxes[Mon]->isChecked() || dayCheckBoxes[Tue]->isChecked() ||
                              dayCheckBoxes[Wed]->isChecked() || dayCheckBoxes[Thu]->isChecked() || dayCheckBoxes[Fri]->isChecked());
    if(weekends && weekdays) {
        daysComboBox->setCurrentIndex(0);
    }
    else if(weekdays && noWeekends) {
        daysComboBox->setCurrentIndex(1);
    }
    else if(weekends && noWeekdays) {
        daysComboBox->setCurrentIndex(2);
    }
    else {
        daysComboBox->setCurrentIndex(3);
    }
}

void SchedulePage::day_LineEdit_textChanged(const QString &text, QLineEdit *dayLineEdit, QString &dayname)
{
    //validate entry
    QString currText = text;
    int currPos = 0;
    if(SurveyMakerWizard::noInvalidPunctuation.validate(currText, currPos) != QValidator::Acceptable) {
        SurveyMakerWizard::invalidExpression(dayLineEdit, currText, this);
    }
    dayname = currText.trimmed();
    update();
}

void SchedulePage::update()
{
    //update the schedule grid
    QList<QWidget *> widgets = sc->findChildren<QWidget *>();
    const int numRowsToShow = std::max(0.0f, 1 + ((toComboBox->currentData().toFloat() - fromComboBox->currentData().toFloat()) *
                                                   60 / (MIN_SCHEDULE_RESOLUTION * resolutionComboBox->currentData().toInt())));
    for(auto &widget : widgets) {
        int row, col, rowSpan, colSpan;
        scLayout->getItemPosition(scLayout->indexOf(widget), &row, &col, &rowSpan, &colSpan);
        widget->setVisible(((row-1) < numRowsToShow) && ((col == 0) || dayCheckBoxes[col-1]->isChecked()));
        if((row == 0) && (col > 0)) {
            auto *dayLabel = qobject_cast<QLabel *>(widget);
            if(dayLabel != nullptr) {
                dayLabel->setText((dayNames[col-1]).left(3));
            }
        }
        if((col == 0) && (row > 0)) {
            auto *timeLabel = qobject_cast<QLabel *>(widget);
            if(timeLabel != nullptr) {
                timeLabel->setText(SurveyMakerWizard::sundayMidnight.time()
                                       .addSecs(60 * ((60 * fromComboBox->currentData().toFloat()) + ((row-1) * MIN_SCHEDULE_RESOLUTION * resolutionComboBox->currentData().toInt())))
                                       .toString(formatComboBox->currentData().toString()));
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

    const bool scheduleOn = questions[schedule]->getValue();
    const bool timezoneOn = questions[timezone]->getValue();

    baseTimezoneLabel->setVisible(timezoneOn);
    baseTimezoneLabel->setEnabled(scheduleOn);
    baseTimezoneComboBox->setVisible(timezoneOn);
    baseTimezoneComboBox->setEnabled(scheduleOn);
    customBaseTimezone->setVisible(baseTimezoneComboBox->isVisible() && (baseTimezoneComboBox->currentIndex() == 1));
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
    scheduleQuestion = generateScheduleQuestion(busyOrFreeComboBox->currentIndex() == busy, timezoneOn, baseTimezone);
    questionPreviewTopLabels[schedule]->setText(scheduleQuestion);

    busyOrFreeLabel->setEnabled(scheduleOn);
    busyOrFreeComboBox->setEnabled(scheduleOn);
    timespanLabel->setEnabled(scheduleOn);
    daysComboBox->setEnabled(scheduleOn);
    fromLabel->setEnabled(scheduleOn);
    fromComboBox->setEnabled(scheduleOn);
    toLabel->setEnabled(scheduleOn);
    toComboBox->setEnabled(scheduleOn);
    resolutionLabel->setEnabled(scheduleOn);
    resolutionComboBox->setEnabled(scheduleOn);
    formatLabel->setEnabled(scheduleOn);
    formatComboBox->setEnabled(scheduleOn);
}

QString SchedulePage::generateScheduleQuestion(bool scheduleAsBusy, bool timezoneOn, const QString &baseTimezone)
{
    QString questionText = SCHEDULEQUESTION1;
    questionText += (scheduleAsBusy? SCHEDULEQUESTION2BUSY : SCHEDULEQUESTION2FREE);
    questionText += SCHEDULEQUESTION3;
    if(timezoneOn) {
        questionText += SCHEDULEQUESTION4 + baseTimezone;
    }
    return questionText;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


CourseInfoPage::CourseInfoPage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::courseinfo, parent)
{
    questions[section]->setLabel(tr("Section"));
    connect(questions[section], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    addSectionButton = new QPushButton;
    addSectionButton->setStyleSheet(ADDBUTTONSTYLE);
    addSectionButton->setText(tr("Add section"));
    addSectionButton->setIcon(QIcon(":/icons_new/addButton.png"));
    addSectionButton->setEnabled(false);
    questions[section]->addWidget(addSectionButton, 1, 0, false, Qt::AlignLeft);
    connect(addSectionButton, &QPushButton::clicked, this, &CourseInfoPage::addASection);
    sectionLineEdits.reserve(MAX_SECTIONSEXPECTED);
    deleteSectionButtons.reserve(MAX_SECTIONSEXPECTED);
    sectionNames.reserve(MAX_SECTIONSEXPECTED);

    questionPreviewTopLabels[section]->setText(SECTIONQUESTION);
    questionPreviewLayouts[section]->addWidget(questionPreviewTopLabels[section]);
    auto *sectionsPreviewBox = new QGroupBox("");
    sectionsPreviewBox->setStyleSheet("border-style:none;");
    sectionsPreviewLayout = new QVBoxLayout;
    sectionsPreviewLayout->setSpacing(0);
    sectionsPreviewLayout->setContentsMargins(20, 0, 0, 0);
    sectionsPreviewBox->setLayout(sectionsPreviewLayout);
    questionPreviewLayouts[section]->addWidget(sectionsPreviewBox);
    questionPreviewBottomLabels[section]->hide();
    questionPreviews[section]->hide();
    registerField("Section", questions[section], "value", "valueChanged");
    registerField("SectionNames", this, "sectionNames", "sectionNamesChanged");

    questions[wantToWorkWith]->setLabel(tr("Classmates I want to work with"));
    connect(questions[wantToWorkWith], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    questionPreviewTopLabels[wantToWorkWith]->setText(tr("Classmates"));
    questionPreviewLayouts[wantToWorkWith]->addWidget(questionPreviewTopLabels[wantToWorkWith]);
    ww = new QPlainTextEdit;
    ww->setStyleSheet(PLAINTEXTEDITSTYLE);
    ww->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ww->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    wwc.reserve(MAX_PREFTEAMMATES);
    wwc << new QComboBox;
    wwc.last()->setStyleSheet(COMBOBOXSTYLE);
    questionPreviewLayouts[wantToWorkWith]->addWidget(ww);
    questionPreviewLayouts[wantToWorkWith]->addWidget(wwc.last());
    questionPreviewBottomLabels[wantToWorkWith]->clear();
    questionPreviewBottomLabels[wantToWorkWith]->setWordWrap(true);
    questionPreviewLayouts[wantToWorkWith]->addWidget(questionPreviewBottomLabels[wantToWorkWith]);
    wwc.last()->hide();
    questionPreviews[wantToWorkWith]->hide();
    questionPreviewBottomLabels[wantToWorkWith]->hide();
    registerField("PrefTeammate", questions[wantToWorkWith], "value", "valueChanged");

    questionLayout->removeItem(questionLayout->itemAt(4));

    questions[wantToAvoid]->setLabel(tr("Classmates I want to avoid"));
    connect(questions[wantToAvoid], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    questionPreviewTopLabels[wantToAvoid]->setText(tr("Classmates"));
    questionPreviewLayouts[wantToAvoid]->addWidget(questionPreviewTopLabels[wantToAvoid]);
    wa = new QPlainTextEdit;
    wa->setStyleSheet(PLAINTEXTEDITSTYLE);
    wa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    wa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    wac.reserve(MAX_PREFTEAMMATES);
    wac << new QComboBox;
    wac.last()->setStyleSheet(COMBOBOXSTYLE);
    questionPreviewLayouts[wantToAvoid]->addWidget(wa);
    questionPreviewLayouts[wantToAvoid]->addWidget(wac.last());
    questionPreviewBottomLabels[wantToAvoid]->clear();
    questionPreviewBottomLabels[wantToAvoid]->setWordWrap(true);
    questionPreviewLayouts[wantToAvoid]->addWidget(questionPreviewBottomLabels[wantToAvoid]);
    wac.last()->hide();
    questionPreviewBottomLabels[wantToWorkWith]->hide();
    questionPreviews[wantToAvoid]->hide();
    registerField("PrefNonTeammate", questions[wantToAvoid], "value", "valueChanged");

    numPrefTeammatesExplainer = new QLabel(tr("Number of classmates a student can indicate:"));
    numPrefTeammatesExplainer->setWordWrap(true);
    numPrefTeammatesExplainer->setStyleSheet(LABELSTYLE);
    numPrefTeammatesSpinBox = new QSpinBox;
    numPrefTeammatesSpinBox->setValue(1);
    numPrefTeammatesSpinBox->setMinimum(1);
    numPrefTeammatesSpinBox->setMaximum(MAX_PREFTEAMMATES);
    connect(numPrefTeammatesSpinBox, &QSpinBox::valueChanged, this, &CourseInfoPage::update);
    questions[wantToAvoid]->addWidget(numPrefTeammatesExplainer, 1, 0, true);
    questions[wantToAvoid]->addWidget(numPrefTeammatesSpinBox, 2, 0, false, Qt::AlignLeft);
    registerField("numPrefTeammates", numPrefTeammatesSpinBox);

    selectFromRosterLabel = new LabelThatForwardsMouseClicks;
    selectFromRosterLabel->setText("<span style=\"color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size:12pt\">" + tr("Select from a class roster") + "</span>");
    selectFromRosterSwitch = new SwitchButton(false);
    connect(selectFromRosterSwitch, &SwitchButton::valueChanged, this, &CourseInfoPage::update);
    connect(selectFromRosterLabel, &LabelThatForwardsMouseClicks::mousePressed, selectFromRosterSwitch, &SwitchButton::mousePressEvent);
    questions[wantToAvoid]->addWidget(selectFromRosterLabel, 3, 0, false);
    questions[wantToAvoid]->addWidget(selectFromRosterSwitch, 3, 1, false, Qt::AlignRight);
    uploadExplainer = new QLabel(tr("You can upload a class roster so that students select names rather than typing as a free response question"));
    uploadExplainer->setWordWrap(true);
    uploadExplainer->setStyleSheet(LABELSTYLE);
    uploadButton = new QPushButton;
    uploadButton->setStyleSheet(ADDBUTTONSTYLE);
    uploadButton->setText(tr("Upload class roster"));
    uploadButton->setIcon(QIcon(":/icons_new/addButton.png"));
    uploadButton->setEnabled(false);
    connect(uploadButton, &QPushButton::clicked, this, &CourseInfoPage::uploadRoster);
    questions[wantToAvoid]->addWidget(uploadExplainer, 4, 0, true);
    questions[wantToAvoid]->addWidget(uploadButton, 5, 0, false, Qt::AlignLeft);

    registerField("StudentNames", this, "studentNames", "studentNamesChanged");

    addASection(true);
    addASection();
}

void CourseInfoPage::initializePage()
{
}

void CourseInfoPage::cleanupPage()
{
}

void CourseInfoPage::resizeEvent(QResizeEvent *event)
{
    resizeQuestionPlainTextEdit(ww);
    resizeQuestionPlainTextEdit(wa);
    QWidget::resizeEvent(event);
}

void CourseInfoPage::setSectionNames(const QStringList &newSectionNames)
{
    for(int i = 0; i < sectionLineEdits.size(); i++) {
        deleteASection(i, true);
    }
    for(const auto &newSectionName : newSectionNames) {
        addASection(true);
        sectionLineEdits.last()->setText(newSectionName);
    }
    emit sectionNamesChanged(sectionNames);

    update();
}

QStringList CourseInfoPage::getSectionNames() const
{
    return sectionNames;
}

void CourseInfoPage::setStudentNames(const QStringList &newStudentNames)
{
    studentNames = newStudentNames;
    selectFromRosterSwitch->setValue(!studentNames.isEmpty());
    emit studentNamesChanged(studentNames);
}

QStringList CourseInfoPage::getStudentNames() const
{
    return studentNames;
}

void CourseInfoPage::update()
{
    sectionNames.clear();

    QLayoutItem *child;
    while ((child = sectionsPreviewLayout->takeAt(0)) != nullptr) {
        delete child->widget(); // delete the widget
        delete child;   // delete the layout item
    }

    auto *topLabel = new QLabel(tr("Select one:"));
    topLabel->setStyleSheet(LABELSTYLE);
    topLabel->setWordWrap(true);
    sectionsPreviewLayout->addWidget(topLabel);
    sc.clear();
    for(const int visibleSectionLineEdit : visibleSectionLineEdits) {
        sc << new QRadioButton;
        sectionsPreviewLayout->addWidget(sc.last());
        const QString sectName = sectionLineEdits[visibleSectionLineEdit]->text();
        if(!sectName.isEmpty()) {
            sc.last()->setText(sectName);
            sectionNames << sectName;
        }
    }
    sc.first()->setChecked(true);

    for(auto &sectionLineEdit : sectionLineEdits) {
        sectionLineEdit->setEnabled(questions[section]->getValue());
    }
    for(auto &deleteSectionButton : deleteSectionButtons) {
        deleteSectionButton->setEnabled(questions[section]->getValue() && (visibleSectionLineEdits.size() > 2));
    }
    addSectionButton->setEnabled(questions[section]->getValue());

    const bool oneOfPrefQuestions = (questions[wantToWorkWith]->getValue() || questions[wantToAvoid]->getValue());
    if (oneOfPrefQuestions) {
        selectFromRosterLabel->setText(selectFromRosterLabel->text().replace("#bebebe", DEEPWATERHEX));
    }
    else {
        selectFromRosterLabel->setText(selectFromRosterLabel->text().replace(DEEPWATERHEX, "#bebebe"));
    }
    selectFromRosterSwitch->setEnabled(oneOfPrefQuestions);
    numPrefTeammatesExplainer->setEnabled(oneOfPrefQuestions);
    numPrefTeammatesSpinBox->setEnabled(oneOfPrefQuestions);
    uploadExplainer->setEnabled(oneOfPrefQuestions);
    uploadButton->setEnabled(selectFromRosterSwitch->isEnabled() && selectFromRosterSwitch->value());

    numPrefTeammates = numPrefTeammatesSpinBox->value();
    questionPreviewTopLabels[wantToAvoid]->setHidden(questions[wantToWorkWith]->getValue() && questions[wantToAvoid]->getValue());  //don't need two labels if both questions on
    if(!selectFromRosterSwitch->value()) {
        setStudentNames({});
    }
    if(studentNames.isEmpty()) {
        ww->show();
        ww->setPlainText(generateTeammateQuestion(true, true, numPrefTeammates));
        resizeQuestionPlainTextEdit(ww);
        wa->show();
        wa->setPlainText(generateTeammateQuestion(false, true, numPrefTeammates));
        resizeQuestionPlainTextEdit(wa);
        for(auto &combobox : wwc) {
            combobox->hide();
        }
        for(auto &combobox : wac) {
            combobox->hide();
        }
        questionPreviewBottomLabels[wantToWorkWith]->hide();
        questionPreviewBottomLabels[wantToAvoid]->hide();
    }
    else {
        ww->hide();
        wa->hide();
        while(wwc.size() < numPrefTeammates) {
            wwc << new QComboBox;
            wwc.last()->setStyleSheet(COMBOBOXSTYLE);
            questionPreviewLayouts[wantToWorkWith]->insertWidget(questionPreviewLayouts[wantToWorkWith]->count()-1, wwc.last());
        }
        while(wwc.size() > numPrefTeammates) {
            delete wwc.last();
            wwc.removeLast();
        }
        for(auto &combobox : wwc) {
            combobox->show();
            combobox->clear();
            combobox->addItem(generateTeammateQuestion(true, false, numPrefTeammates));
        }

        while(wac.size() < numPrefTeammates) {
            wac << new QComboBox;
            wac.last()->setStyleSheet(COMBOBOXSTYLE);
            questionPreviewLayouts[wantToAvoid]->insertWidget(questionPreviewLayouts[wantToAvoid]->count()-1, wac.last());
        }
        while(wac.size() > numPrefTeammates) {
            delete wac.last();
            wac.removeLast();
        }
        for(auto &combobox : wac) {
            combobox->show();
            combobox->clear();
            combobox->addItem(generateTeammateQuestion(false, false, numPrefTeammates));
        }

        QString studentNamesLabelText = tr("Options: ");
        // Show the first 10
        if(studentNames.size() > 10) {
            studentNamesLabelText += studentNames.first(10).join("  |  ") + " |  {" + QString::number(studentNames.size()-10) + tr(" more ...}");
        }
        else {
            studentNamesLabelText += studentNames.join("  |  ");
        }
        questionPreviewBottomLabels[wantToWorkWith]->setText(studentNamesLabelText);
        questionPreviewBottomLabels[wantToWorkWith]->show();
        questionPreviewBottomLabels[wantToAvoid]->setText(studentNamesLabelText);
        questionPreviewBottomLabels[wantToAvoid]->show();
        questionPreviewBottomLabels[wantToWorkWith]->setHidden(questions[wantToWorkWith]->getValue() && questions[wantToAvoid]->getValue());
    }
}

QString CourseInfoPage::generateTeammateQuestion(bool wantToWorkWith, bool typingNames, int numClassmates)
{
    QString question;
    question += typingNames ? PREFTEAMMATEQUESTION1TYPE : PREFTEAMMATEQUESTION1SELECT;

    question += numClassmates == 1 ? PREFTEAMMATEQUESTION2AMATE : PREFTEAMMATEQUESTION2MULTIA + QString::number(numClassmates) + PREFTEAMMATEQUESTION2MULTIB;

    question += wantToWorkWith ? PREFTEAMMATEQUESTION3YES : PREFTEAMMATEQUESTION3NO;

    if(typingNames) {
        question += numClassmates == 1 ? PREFTEAMMATEQUESTION4TYPEONE : PREFTEAMMATEQUESTION4TYPEMULTI;
    }

    return question;
}

void CourseInfoPage::resizeQuestionPlainTextEdit(QPlainTextEdit *const questionPlainTextEdit)
{
    // make the edit box as tall as needed to show all text
    const auto *const doc = questionPlainTextEdit->document();
    const QFontMetrics font(doc->defaultFont());
    const auto margins = questionPlainTextEdit->contentsMargins();
    const int height = (font.lineSpacing() * std::max(1.0, doc->size().height())) +
                       (doc->documentMargin() + questionPlainTextEdit->frameWidth()) * 2 + margins.top() + margins.bottom();
    questionPlainTextEdit->setFixedHeight(height);
}

void CourseInfoPage::addASection(bool pauseVisualUpdate)
{
    sectionLineEdits << new QLineEdit;
    deleteSectionButtons << new QPushButton;
    sectionLineEdits.last()->setStyleSheet(LINEEDITSTYLE);
    deleteSectionButtons.last()->setStyleSheet(DELBUTTONSTYLE);
    sectionLineEdits.last()->setPlaceholderText(tr("Section name"));
    deleteSectionButtons.last()->setText(tr(" Delete"));
    deleteSectionButtons.last()->setIcon(QIcon(":/icons_new/trashButton.png"));
    questions[section]->moveWidget(addSectionButton, numSectionsEntered + 2, 0, false, Qt::AlignLeft);
    questions[section]->addWidget(sectionLineEdits.last(), numSectionsEntered + 1, 0, false);
    sectionLineEdits.last()->show();
    questions[section]->addWidget(deleteSectionButtons.last(), numSectionsEntered + 1, 1, false);
    deleteSectionButtons.last()->show();
    connect(sectionLineEdits.last(), &QLineEdit::textChanged, this, &CourseInfoPage::update);
    connect(deleteSectionButtons.last(), &QPushButton::clicked, this, [this, numSectionsEntered = numSectionsEntered]{deleteASection(numSectionsEntered);});
    visibleSectionLineEdits << numSectionsEntered;
    numSectionsEntered++;
    sectionLineEdits.last()->setFocus();

    if(!pauseVisualUpdate) {
        update();
    }
}

void CourseInfoPage::deleteASection(int sectionNum, bool pauseVisualUpdate)
{
    sectionLineEdits[sectionNum]->hide();
    sectionLineEdits[sectionNum]->clear();
    deleteSectionButtons[sectionNum]->hide();

    visibleSectionLineEdits.removeAll(sectionNum);

    if(!pauseVisualUpdate) {
        update();
    }
}

bool CourseInfoPage::uploadRoster()
{
    // Open the roster file
    CsvFile rosterFile;
    QFileInfo *saveFilePath = &(qobject_cast<SurveyMakerWizard *>(wizard()))->saveFileLocation;
    if(!rosterFile.open(this, CsvFile::Operation::read, tr("Open Student Roster File"), saveFilePath->canonicalFilePath(), tr("Roster File"))) {
        return false;
    }

    // Read the header row
    if(!rosterFile.readHeader()) {
        // header row could not be read as valid data
        grueprGlobal::errorMessage(this, tr("File error."), tr("This file is empty or there is an error in its format."));
        return false;
    }

    // Ask user what the columns mean
    // Preloading the selector boxes with "unused" except first time "first name", "last name", and "name" are found
    const QList<possFieldMeaning> rosterFieldOptions  = {{"First Name", "((first)|(given)|(preferred)).*(name)", 1},
                                                          {"Last Name", "((last)|(sur)|(family)).*(name)", 1},
                                                          {"Full Name (First Last)", "(name)", 1},
                                                          {"Full Name (Last, First)", "(name)", 1}};;
    if(rosterFile.chooseFieldMeaningsDialog(rosterFieldOptions, this)->exec() == QDialog::Rejected) {
        return false;
    }

    // set field values now according to uer's selection of field meanings (defulting to -1 if not chosen)
    const int firstNameField = int(rosterFile.fieldMeanings.indexOf("First Name"));
    const int lastNameField = int(rosterFile.fieldMeanings.indexOf("Last Name"));
    const int firstLastNameField = int(rosterFile.fieldMeanings.indexOf("Full Name (First Last)"));
    const int lastFirstNameField = int(rosterFile.fieldMeanings.indexOf("Full Name (Last, First)"));

    studentNames.clear();

    // Process each row until there's an empty one. Load names one-by-one
    // Also, skip the header row, if there is one
    if(rosterFile.hasHeaderRow) {
        rosterFile.readDataRow();
    }
    else {
        rosterFile.readDataRow(true);
    }
    do {
        QString name;
        if(firstLastNameField != -1) {
            name = rosterFile.fieldValues.at(firstLastNameField).simplified();
        }
        else if(lastFirstNameField != -1) {
            const QStringList lastandfirstname = rosterFile.fieldValues.at(lastFirstNameField).split(',');
            name = lastandfirstname.at(1).trimmed() + " " + lastandfirstname.at(0).simplified();
        }
        else if(firstNameField != -1 && lastNameField != -1) {
            name = rosterFile.fieldValues.at(firstNameField).simplified() + " " + rosterFile.fieldValues.at(lastNameField).simplified();
        }
        else if(firstNameField != -1) {
            name = rosterFile.fieldValues.at(firstNameField).simplified();
        }
        else if(lastNameField != -1) {
            name = rosterFile.fieldValues.at(lastNameField).simplified();
        }
        else {
            grueprGlobal::errorMessage(this, tr("File error."), tr("This roster does not contain student names."));
            return false;
        }
        if(!name.isEmpty() && name != " ") {
            studentNames << name;
        }
    } while(rosterFile.readDataRow());

    rosterFile.close();

    setStudentNames(studentNames);
    update();
    return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


PreviewAndExportPage::PreviewAndExportPage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::previewexport, parent)
{
    const int lastPageIndex = qobject_cast<SurveyMakerWizard *>(wizard())->numPages - 1;
    for(int sectionNum = 0; sectionNum < lastPageIndex; sectionNum++) {
        preSectionSpacer << new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        questionLayout->addItem(preSectionSpacer.last());
        section << new SurveyMakerPreviewSection(sectionNum, SurveyMakerWizard::pageNames[sectionNum], SurveyMakerWizard::numOfQuestionsInPage[sectionNum], this);
        questionLayout->addWidget(section.last());
        connect(section.last(), &SurveyMakerPreviewSection::editRequested, this, [this, lastPageIndex](int pageNum){while(pageNum < lastPageIndex){wizard()->back(); pageNum++;}});
    }

    auto *saveExportFrame = new QFrame;
    saveExportFrame->setStyleSheet(QString("background-color: " DEEPWATERHEX "; color: white; font-family: 'DM Sans'; font-size: 12pt;"));
    auto *saveExportlayout = new QGridLayout(saveExportFrame);
    auto *saveExporttitle = new QLabel("<span style=\"color: white; font-family:'DM Sans'; font-size:14pt;\">" + tr("Export Survey As:") + "</span>");
    auto *destination = new QGroupBox("");
    destination->setStyleSheet("border-style:none;");
    auto helpIcon = new LabelWithInstantTooltip("", this);
    helpIcon->setStyleSheet(BIGTOOLTIPSTYLE);
    QPixmap whiteLightbulb = QPixmap(":/icons_new/lightbulb.png").scaled(25, 25, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPainter painter(&whiteLightbulb);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(whiteLightbulb.rect(), QColor("white"));
    painter.end();
    helpIcon->setPixmap(whiteLightbulb);
    auto helpLabel = new LabelWithInstantTooltip(tr(" Help me choose!"), this);
    helpLabel->setStyleSheet(QString(LABELSTYLE).replace(DEEPWATERHEX, "white").replace("10pt;", "12pt;") + BIGTOOLTIPSTYLE);
    helpLabel->setWordWrap(true);
    auto helpLayout = new QHBoxLayout;
    helpLayout->addWidget(helpIcon, 0, Qt::AlignLeft | Qt::AlignVCenter);
    helpLayout->addWidget(helpLabel, 1, Qt::AlignVCenter);
    const QString helpText = tr("<html><span style=\"color: black;\">gruepr offers the following ways to use the survey you've created:"
                                "<ul>"
                                "<li><u>Google Form in Your Google Drive</u> Send your students the link, and gruepr can download the results.</li>"
                                "<li><u>Survey in Your Canvas Course</u> Publish it in the Canvas page your class already uses, and gruepr can download the results.</li>"
                                "<li><u>Text Files on Your Computer</u>  Use your own survey instrument. "
                                " One file lists your survey questions, and another is preformatted for you to (1) open in Excel, Numbers, or Sheets,"
                                " (2) fill in your students' responses, and then (3) open in gruepr.</li>"
                                "<li><u>gruepr Survey File on Your Computer</u> Save your work for reuse, modification, or sharing with colleagues.</li>"
                                "</ul>"
                                "</span></html>");
    helpIcon->setToolTipText(helpText);
    helpLabel->setToolTipText(helpText);
    destinationGoogle = new QRadioButton(tr("Google Form"));
    destinationCanvas = new QRadioButton(tr("Canvas Survey"));
    destinationTextFiles = new QRadioButton(tr("Text Files"));
    destinationGrueprFile = new QRadioButton(tr("gruepr Survey File"));
    destinationGoogle->setChecked(true);
    auto *vbox = new QVBoxLayout;
    vbox->addWidget(destinationGoogle);
    vbox->addWidget(destinationCanvas);
    vbox->addWidget(destinationTextFiles);
    vbox->addWidget(destinationGrueprFile);
    destination->setLayout(vbox);
    auto *exportButton = new QPushButton;
    exportButton->setStyleSheet(NEXTBUTTONSTYLE);
    exportButton->setText(tr("Export Survey"));
    connect(exportButton, &QPushButton::clicked, this, &PreviewAndExportPage::exportSurvey);
    int row = 0;
    saveExportlayout->addWidget(saveExporttitle, row, 0, 1, 1, Qt::AlignLeft);
    saveExportlayout->addLayout(helpLayout, row++, 1, 1, 1, Qt::AlignRight);
    saveExportlayout->setColumnStretch(0, 1);
    saveExportlayout->addWidget(destination, row++, 0, 1, -1);
    saveExportlayout->addWidget(exportButton, row++, 0, 1, -1);
    preSectionSpacer << new QSpacerItem(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
    questionLayout->addItem(preSectionSpacer.last());
    questionLayout->addWidget(saveExportFrame);

    questionLayout->addStretch(1);

    section[SurveyMakerWizard::demographics]->questionLabel[0]->setText(FIRSTNAMEQUESTION);
    section[SurveyMakerWizard::demographics]->questionLineEdit[0]->setPlaceholderText(tr("First Name"));
    section[SurveyMakerWizard::demographics]->questionLabel[1]->setText(LASTNAMEQUESTION);
    section[SurveyMakerWizard::demographics]->questionLineEdit[1]->setPlaceholderText(tr("Last Name"));
    section[SurveyMakerWizard::demographics]->questionLabel[2]->setText(EMAILQUESTION);
    section[SurveyMakerWizard::demographics]->questionLineEdit[2]->setPlaceholderText(tr("Email"));
    section[SurveyMakerWizard::demographics]->questionLabel[3]->setText(tr("Gender"));
    section[SurveyMakerWizard::demographics]->questionLabel[4]->setText(URMQUESTION);
    section[SurveyMakerWizard::demographics]->questionLineEdit[4]->setPlaceholderText(tr("Race / ethnicity / cultural heritage"));

    section[SurveyMakerWizard::schedule]->questionLabel[0]->setText(TIMEZONEQUESTION);
    section[SurveyMakerWizard::schedule]->questionComboBox[0]->addItems(SurveyMakerWizard::timezoneNames);
    section[SurveyMakerWizard::schedule]->questionComboBox[0]->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    section[SurveyMakerWizard::schedule]->questionComboBox[0]->setPlaceholderText(QString(SELECTONE).remove(':'));
    section[SurveyMakerWizard::schedule]->questionLabel[1]->setText(tr("Schedule"));
    schedGrid = new QWidget;
    schedGridLayout = new QGridLayout(schedGrid);
    for(int time = 0; time < MAX_BLOCKS_PER_DAY; time++) {
        auto *rowLabel = new QLabel(SurveyMakerWizard::sundayMidnight.time().addSecs(time * MIN_SCHEDULE_RESOLUTION * 60).toString("h:mm A"));
        rowLabel->setStyleSheet(LABELSTYLE);
        schedGridLayout->addWidget(rowLabel, time+1, 0);
    }
    for(int day = 0; day < MAX_DAYS; day++) {
        auto *colLabel = new QLabel(SurveyMakerWizard::sundayMidnight.addDays(day).toString("ddd"));
        colLabel->setStyleSheet(LABELSTYLE);
        schedGridLayout->addWidget(colLabel, 0, day+1);
        schedGridLayout->setColumnStretch(day, 1);
    }
    for(int time = 1; time <= MAX_BLOCKS_PER_DAY; time++) {
        for(int day = 1; day <= MAX_DAYS; day++) {
            auto check = new QCheckBox;
            schedGridLayout->addWidget(check, time, day);
        }
    }
    schedGridLayout->setSpacing(5);
    schedGridLayout->setRowStretch(24, 1);
    section[SurveyMakerWizard::schedule]->addWidget(schedGrid);

    section[SurveyMakerWizard::courseinfo]->questionLabel[0]->setText(SECTIONQUESTION);
    section[SurveyMakerWizard::courseinfo]->questionLineEdit[1]->setPlaceholderText(tr("Classmates I want to work with"));
    section[SurveyMakerWizard::courseinfo]->questionLineEdit[2]->setPlaceholderText(tr("Classmates I want to avoid working with"));
}

PreviewAndExportPage::~PreviewAndExportPage()
{
    delete canvas;
    delete google;
}

void PreviewAndExportPage::initializePage()
{
    survey = new Survey;
    surveyHasBeenExported = false;
    auto *wiz = qobject_cast<SurveyMakerWizard *>(wizard());
    wiz->previewPageVisited = true;
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton;
    wiz->setButtonLayout(buttonLayout);
    wiz->button(QWizard::NextButton)->setStyleSheet(INVISBUTTONSTYLE);
    wizard()->button(QWizard::CancelButton)->disconnect();
    connect(wizard()->button(QWizard::CancelButton), &QPushButton::clicked, this, [this] {if(surveyHasBeenExported) {wizard()->reject();}
                                                                                          else {
                                                                                            const bool okClear = grueprGlobal::warningMessage(this, "gruepr",
                                                                                            tr("You have not yet exported this survey.\n"
                                                                                               "Are you sure you want to close?"),
                                                                                            tr("Yes"), tr("No"));
                                                                                            if(okClear) {
                                                                                                wizard()->reject();
                                                                                            }
                                                                                          }});
    //Survey title
    const QString title = field("SurveyTitle").toString().trimmed();
    preSectionSpacer[0]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
    section[SurveyMakerWizard::introtitle]->setTitle(title);
    survey->title = title;

    //Demographics
    const bool firstname = field("FirstName").toBool();
    const bool lastname = field("LastName").toBool();
    const bool email = field("Email").toBool();
    const bool gender = field("Gender").toBool();
    const bool urm = field("RaceEthnicity").toBool();

    if(firstname) {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[0]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[0]->show();
        section[SurveyMakerWizard::demographics]->questionLineEdit[0]->show();
        survey->questions << Question(FIRSTNAMEQUESTION, Question::QuestionType::shorttext);
    }
    else {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[0]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[0]->hide();
        section[SurveyMakerWizard::demographics]->questionLineEdit[0]->hide();
    }

    if(lastname) {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[1]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[1]->show();
        section[SurveyMakerWizard::demographics]->questionLineEdit[1]->show();
        survey->questions << Question(LASTNAMEQUESTION, Question::QuestionType::shorttext);
    }
    else {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[1]->hide();
        section[SurveyMakerWizard::demographics]->questionLineEdit[1]->hide();
    }

    if(email) {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[2]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[2]->show();
        section[SurveyMakerWizard::demographics]->questionLineEdit[2]->show();
        survey->questions << Question(EMAILQUESTION, Question::QuestionType::shorttext);
    }
    else {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[2]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[2]->hide();
        section[SurveyMakerWizard::demographics]->questionLineEdit[2]->hide();
    }

    if(gender) {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[3]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[3]->show();
        QString questionText;
        QStringList genderOptions;
        switch(static_cast<GenderType>(field("genderOptions").toInt())) {
        case GenderType::biol:
            questionText = GENDERQUESTION;
            genderOptions = QString(BIOLGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
            break;
        case GenderType::adult:
            questionText = GENDERQUESTION;
            genderOptions = QString(ADULTGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
            break;
        case GenderType::child:
            questionText = GENDERQUESTION;
            genderOptions = QString(CHILDGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
            break;
        case GenderType::pronoun:
            questionText = PRONOUNQUESTION;
            genderOptions = QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
            break;
        }
        section[SurveyMakerWizard::demographics]->questionLabel[3]->setText(questionText);
        QLayoutItem *child;
        while((child = section[SurveyMakerWizard::demographics]->questionGroupLayout[3]->takeAt(0)) != nullptr) {
            delete child->widget(); // delete the widget
            delete child;   // delete the layout item
        }
        for(const auto &genderOption : genderOptions) {
            auto *option = new QRadioButton(genderOption);
            section[SurveyMakerWizard::demographics]->questionGroupLayout[3]->addWidget(option);
        }
        section[SurveyMakerWizard::demographics]->questionGroupBox[3]->show();
        survey->questions << Question(questionText, Question::QuestionType::radiobutton, genderOptions);
    }
    else {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[3]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[3]->hide();
        section[SurveyMakerWizard::demographics]->questionGroupBox[3]->hide();
    }

    if(urm) {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[4]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[4]->show();
        section[SurveyMakerWizard::demographics]->questionLineEdit[4]->show();
        survey->questions << Question(URMQUESTION, Question::QuestionType::shorttext);
    }
    else {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[4]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[4]->hide();
        section[SurveyMakerWizard::demographics]->questionLineEdit[4]->hide();
    }

    if(firstname || lastname || email || gender || urm) {
        preSectionSpacer[1]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->show();
    }
    else {
        preSectionSpacer[1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->hide();
    }

    //Multiple Choice
    const int multiChoiceNumQuestions = field("multiChoiceNumQuestions").toInt();
    const QStringList multiQuestionTexts = field("multiChoiceQuestionTexts").toStringList();
    const auto multiQuestionResponses = field("multiChoiceQuestionResponses").toList();
    const auto multiQuestionMultis = field("multiChoiceQuestionMultis").toList();

    int actualNumMultiQuestions = 0;
    for(int questionNum = 0; questionNum < multiChoiceNumQuestions; questionNum++) {
        if(!multiQuestionTexts[questionNum].isEmpty()) {
            actualNumMultiQuestions++;
            section[SurveyMakerWizard::multichoice]->preQuestionSpacer[questionNum]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
            section[SurveyMakerWizard::multichoice]->questionLabel[questionNum]->show();
            auto responses = multiQuestionResponses[questionNum].toStringList();
            if((multiQuestionMultis[questionNum]).toBool()) {
                section[SurveyMakerWizard::multichoice]->questionLabel[questionNum]->setText(multiQuestionTexts[questionNum] + "\n" + SELECTMULT);
                QLayoutItem *child;
                while((child = section[SurveyMakerWizard::multichoice]->questionGroupLayout[questionNum]->takeAt(0)) != nullptr) {
                    delete child->widget(); // delete the widget
                    delete child;   // delete the layout item
                }
                for(const auto &response : responses) {
                    auto *option = new QCheckBox(response);
                    section[SurveyMakerWizard::multichoice]->questionGroupLayout[questionNum]->addWidget(option);
                }
                section[SurveyMakerWizard::multichoice]->questionGroupBox[questionNum]->show();
                section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->hide();
                survey->questions << Question(multiQuestionTexts[questionNum], Question::QuestionType::checkbox, responses);
            }
            else if(responses.size() < 10) {
                section[SurveyMakerWizard::multichoice]->questionLabel[questionNum]->setText(multiQuestionTexts[questionNum] + "\n" + SELECTONE);
                QLayoutItem *child;
                while((child = section[SurveyMakerWizard::multichoice]->questionGroupLayout[questionNum]->takeAt(0)) != nullptr) {
                    delete child->widget(); // delete the widget
                    delete child;   // delete the layout item
                }
                for(const auto &response : responses) {
                    auto *option = new QRadioButton(response);
                    section[SurveyMakerWizard::multichoice]->questionGroupLayout[questionNum]->addWidget(option);
                }
                section[SurveyMakerWizard::multichoice]->questionGroupBox[questionNum]->show();
                section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->hide();
                survey->questions << Question(multiQuestionTexts[questionNum], Question::QuestionType::radiobutton, responses);
            }
            else {
                section[SurveyMakerWizard::multichoice]->questionLabel[questionNum]->setText(multiQuestionTexts[questionNum]);
                section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->clear();
                section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
                section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->addItems(responses);
                section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->setPlaceholderText(QString(SELECTONE).remove(':'));
                section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->setCurrentIndex(-1);
                section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->show();
                section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->setMinimumWidth(
                                                            section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->width() * 1.1);
                section[SurveyMakerWizard::multichoice]->questionGroupBox[questionNum]->hide();
                survey->questions << Question(multiQuestionTexts[questionNum], Question::QuestionType::dropdown, responses);
            }
        }
        else {
            section[SurveyMakerWizard::multichoice]->preQuestionSpacer[questionNum]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
            section[SurveyMakerWizard::multichoice]->questionLabel[questionNum]->hide();
            section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->hide();
            section[SurveyMakerWizard::multichoice]->questionGroupBox[questionNum]->hide();
        }
    }
    for(int i = actualNumMultiQuestions; i < MAX_ATTRIBUTES; i++) {
        section[SurveyMakerWizard::multichoice]->preQuestionSpacer[i]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::multichoice]->questionLabel[i]->hide();
        section[SurveyMakerWizard::multichoice]->questionComboBox[i]->hide();
        section[SurveyMakerWizard::multichoice]->questionGroupBox[i]->hide();
    }

    if(actualNumMultiQuestions > 0) {
        preSectionSpacer[2]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::multichoice]->show();
    }
    else {
        preSectionSpacer[2]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::multichoice]->hide();
    }

    //Schedule
    const bool timezone = field("Timezone").toBool();
    const bool schedule = field("Schedule").toBool();
    //const bool scheduleAsBusy = field("scheduleBusyOrFree").toBool();
    const QString scheduleQuestion = field("ScheduleQuestion").toString();
    const QStringList scheduleDays = field("scheduleDayNames").toStringList();
    const float scheduleFrom = field("scheduleFrom").toFloat();
    const float scheduleTo = field("scheduleTo").toFloat();
    const int scheduleResolution = SchedulePage::resolutionValues.at(field("scheduleResolution").toInt()).second;
    const QString scheduleTimeFormat = SchedulePage::timeFormats.at(field("scheduleTimeFormat").toInt()).first;

    if(timezone) {
        section[SurveyMakerWizard::schedule]->preQuestionSpacer[0]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::schedule]->questionLabel[0]->show();
        section[SurveyMakerWizard::schedule]->questionComboBox[0]->show();
        section[SurveyMakerWizard::schedule]->questionComboBox[0]->setCurrentIndex(-1);
        section[SurveyMakerWizard::schedule]->questionComboBox[0]->setMinimumWidth(section[SurveyMakerWizard::schedule]->questionComboBox[0]->width() * 1.1);
        survey->questions << Question(TIMEZONEQUESTION, Question::QuestionType::dropdown, SurveyMakerWizard::timezoneNames);
    }
    else {
        section[SurveyMakerWizard::schedule]->preQuestionSpacer[0]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::schedule]->questionLabel[0]->hide();
        section[SurveyMakerWizard::schedule]->questionComboBox[0]->hide();
    }

    if(schedule) {
        section[SurveyMakerWizard::schedule]->preQuestionSpacer[1]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::schedule]->questionLabel[1]->setText(scheduleQuestion);
        section[SurveyMakerWizard::schedule]->questionLabel[1]->show();
        survey->questions << Question(scheduleQuestion, Question::QuestionType::schedule);
        for(int day = 0; day < MAX_DAYS; day++) {
            if(!scheduleDays.at(day).isEmpty()) {
                survey->schedDayNames << scheduleDays.at(day);
            }
        }
        schedGrid->show();
        QList<QWidget *> widgets = schedGrid->findChildren<QWidget *>();

        const int numRowsToShow = std::max(0.0f, 1 + ((scheduleTo - scheduleFrom) * 60 / (MIN_SCHEDULE_RESOLUTION * scheduleResolution)));
        for(auto &widget : widgets) {
            int row, col, rowSpan, colSpan;
            schedGridLayout->getItemPosition(schedGridLayout->indexOf(widget), &row, &col, &rowSpan, &colSpan);
            widget->setVisible(((row-1) < numRowsToShow) && ((col == 0) || !scheduleDays[col-1].isEmpty()));
            if((row == 0) && (col > 0)) {
                auto *dayLabel = qobject_cast<QLabel *>(widget);
                if(dayLabel != nullptr) {
                    dayLabel->setText(scheduleDays[col-1]);
                }
            }
            if((col == 0) && (row > 0)) {
                auto *timeLabel = qobject_cast<QLabel *>(widget);
                if(timeLabel != nullptr) {
                    timeLabel->setText(SurveyMakerWizard::sundayMidnight.time()
                                           .addSecs(60 * ((60 * scheduleFrom) + ((row-1) * MIN_SCHEDULE_RESOLUTION * scheduleResolution)))
                                           .toString(scheduleTimeFormat));
                }
                if((row-1) < numRowsToShow) {
                    survey->schedTimeNames << timeLabel->text();
                }
            }
        }
    }
    else {
        section[SurveyMakerWizard::schedule]->preQuestionSpacer[1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::schedule]->questionLabel[1]->hide();
        schedGrid->hide();
    }

    if(timezone || schedule) {
        preSectionSpacer[3]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::schedule]->show();
    }
    else {
        preSectionSpacer[3]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::schedule]->hide();
    }

    //Course Info
    const bool courseSections = field("Section").toBool();
    const QStringList courseSectionsNames = field("SectionNames").toStringList();
    const bool prefTeammate = field("PrefTeammate").toBool();
    const bool prefNonTeammate = field("PrefNonTeammate").toBool();
    const int numPrefTeammates = field("numPrefTeammates").toInt();
    QStringList studentNames = field("StudentNames").toStringList();
    const bool typingStudentNames = studentNames.isEmpty();
    studentNames.prepend("");

    if(courseSections) {
        section[SurveyMakerWizard::courseinfo]->preQuestionSpacer[0]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->questionLabel[0]->show();
        QLayoutItem *child;
        while((child = section[SurveyMakerWizard::courseinfo]->questionGroupLayout[0]->takeAt(0)) != nullptr) {
            delete child->widget(); // delete the widget
            delete child;   // delete the layout item
        }
        for(const auto &courseSectionsName : courseSectionsNames) {
            auto *option = new QRadioButton(courseSectionsName);
            section[SurveyMakerWizard::courseinfo]->questionGroupLayout[0]->addWidget(option);
        }
        section[SurveyMakerWizard::courseinfo]->questionGroupBox[0]->show();
        survey->questions << Question(SECTIONQUESTION, Question::QuestionType::radiobutton, courseSectionsNames);
    }
    else {
        section[SurveyMakerWizard::courseinfo]->preQuestionSpacer[0]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->questionLabel[0]->hide();
        section[SurveyMakerWizard::courseinfo]->questionGroupBox[0]->hide();
    }

    if(prefTeammate) {
        section[SurveyMakerWizard::courseinfo]->preQuestionSpacer[1]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        const QString questionText = CourseInfoPage::generateTeammateQuestion(true, typingStudentNames, numPrefTeammates);
        section[SurveyMakerWizard::courseinfo]->questionLabel[1]->setText(questionText);
        section[SurveyMakerWizard::courseinfo]->questionLabel[1]->show();
        if(typingStudentNames) {
            section[SurveyMakerWizard::courseinfo]->questionLineEdit[1]->show();
            section[SurveyMakerWizard::courseinfo]->questionGroupBox[1]->hide();
            survey->questions << Question(questionText, (numPrefTeammates == 1? Question::QuestionType::shorttext : Question::QuestionType::longtext));
        }
        else {
            section[SurveyMakerWizard::courseinfo]->questionLineEdit[1]->hide();
            section[SurveyMakerWizard::courseinfo]->questionGroupBox[1]->show();
            QLayoutItem *child;
            while((child = section[SurveyMakerWizard::courseinfo]->questionGroupLayout[1]->takeAt(0)) != nullptr) {
                delete child->widget(); // delete the widget
                delete child;   // delete the layout item
            }
            for(int i = 0; i < numPrefTeammates; i++) {
                auto *selector = new QComboBox;
                selector->setStyleSheet(COMBOBOXSTYLE);
                selector->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
                selector->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
                selector->installEventFilter(new MouseWheelBlocker(selector));
                selector->addItems(studentNames);
                selector->setPlaceholderText(QString(SELECTONE).replace(":", tr(" classmate")));
                selector->setCurrentIndex(-1);
                section[SurveyMakerWizard::courseinfo]->questionGroupLayout[1]->addWidget(selector, 0, Qt::AlignLeft);
                selector->show();
                selector->setMinimumWidth(selector->width()*1.1);
                survey->questions << Question(questionText, Question::QuestionType::dropdown, studentNames);                
            }
        }
    }
    else {
        section[SurveyMakerWizard::courseinfo]->preQuestionSpacer[1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->questionLabel[1]->hide();
        section[SurveyMakerWizard::courseinfo]->questionLineEdit[1]->hide();
        section[SurveyMakerWizard::courseinfo]->questionGroupBox[1]->hide();
    }

    if(prefNonTeammate) {
        section[SurveyMakerWizard::courseinfo]->preQuestionSpacer[2]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        const QString questionText = CourseInfoPage::generateTeammateQuestion(false, typingStudentNames, numPrefTeammates);
        section[SurveyMakerWizard::courseinfo]->questionLabel[2]->show();
        section[SurveyMakerWizard::courseinfo]->questionLabel[2]->setText(questionText);
        if(typingStudentNames) {
            section[SurveyMakerWizard::courseinfo]->questionLineEdit[2]->show();
            section[SurveyMakerWizard::courseinfo]->questionGroupBox[2]->hide();
            survey->questions << Question(questionText, (numPrefTeammates == 1? Question::QuestionType::shorttext : Question::QuestionType::longtext));
        }
        else {
            section[SurveyMakerWizard::courseinfo]->questionLineEdit[2]->hide();
            section[SurveyMakerWizard::courseinfo]->questionGroupBox[2]->show();
            QLayoutItem *child;
            while((child = section[SurveyMakerWizard::courseinfo]->questionGroupLayout[2]->takeAt(0)) != nullptr) {
                delete child->widget(); // delete the widget
                delete child;   // delete the layout item
            }
            for(int i = 0; i < numPrefTeammates; i++) {
                auto *selector = new QComboBox;
                selector->setStyleSheet(COMBOBOXSTYLE);
                selector->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
                selector->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
                selector->installEventFilter(new MouseWheelBlocker(selector));
                selector->addItems(studentNames);
                selector->setPlaceholderText(QString(SELECTONE).replace(":", tr(" classmate")));
                selector->setCurrentIndex(-1);
                section[SurveyMakerWizard::courseinfo]->questionGroupLayout[2]->addWidget(selector, 0, Qt::AlignLeft);
                selector->show();
                selector->setMinimumWidth(selector->width()*1.1);
                survey->questions << Question(questionText, Question::QuestionType::dropdown, studentNames);
            }
        }
    }
    else {
        section[SurveyMakerWizard::courseinfo]->preQuestionSpacer[2]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->questionLabel[2]->hide();
        section[SurveyMakerWizard::courseinfo]->questionLineEdit[2]->hide();
        section[SurveyMakerWizard::courseinfo]->questionGroupBox[2]->hide();
    }

    if(courseSections || prefTeammate || prefNonTeammate) {
        preSectionSpacer[4]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->show();
    }
    else {
        preSectionSpacer[4]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->hide();
    }
}

void PreviewAndExportPage::cleanupPage()
{
    delete survey;

    // going back to previous page, so allow user to immediately return to this preview
    auto *wiz = qobject_cast<SurveyMakerWizard *>(wizard());
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton << QWizard::CustomButton2;
    wiz->setButtonLayout(buttonLayout);
    wiz->button(QWizard::NextButton)->setStyleSheet(NEXTBUTTONSTYLE);
    connect(wiz, &QWizard::customButtonClicked, this, [this](int customButton)
            {if(customButton == QWizard::CustomButton2) {while(wizard()->currentId() != SurveyMakerWizard::Page::previewexport) {wizard()->next();}}});
}

void PreviewAndExportPage::exportSurvey()
{
    if(destinationGrueprFile->isChecked()) {
        QFileInfo *saveFileLocation = &(qobject_cast<SurveyMakerWizard *>(wizard()))->saveFileLocation;
        //save all options to a text file
        const QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), saveFileLocation->canonicalPath(), tr("gruepr survey File (*.gru);;All Files (*)"));
        if( !(fileName.isEmpty()) ) {
            QFile saveFile(fileName);
            if(saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                saveFileLocation->setFile(QFileInfo(fileName).canonicalFilePath());
                QJsonObject saveObject;
                saveObject["Title"] = field("SurveyTitle").toString();

                saveObject["FirstName"] = field("FirstName").toBool();
                saveObject["LastName"] = field("LastName").toBool();
                saveObject["Email"] = field("Email").toBool();
                saveObject["Gender"] = field("Gender").toBool();
                saveObject["GenderType"] = field("genderOptions").toInt();
                saveObject["URM"] = field("RaceEthnicity").toBool();

                const int numMultiChoiceQuestions = field("multiChoiceNumQuestions").toInt();
                saveObject["numAttributes"] = numMultiChoiceQuestions;
                QList<QString> multiQuestionTexts = field("multiChoiceQuestionTexts").toStringList();
                auto multiQuestionResponses = field("multiChoiceQuestionResponses").toList();
                auto multiQuestionMultis = field("multiChoiceQuestionMultis").toList();
                auto responseOptions = QString(RESPONSE_OPTIONS).split(';');
                for(int question = 0; question < numMultiChoiceQuestions; question++) {
                    saveObject["Attribute" + QString::number(question+1)+"Question"] = multiQuestionTexts[question];
                    const int responseOptionNum = responseOptions.indexOf(multiQuestionResponses[question].toStringList().join(" / "));
                    saveObject["Attribute" + QString::number(question+1)+"Response"] = responseOptionNum + 1;
                    saveObject["Attribute" + QString::number(question+1)+"AllowMultiResponse"] = multiQuestionMultis[question].toBool();
                    if(responseOptionNum == -1) { // custom options being used
                        saveObject["Attribute" + QString::number(question+1)+"Options"] = multiQuestionResponses[question].toStringList().join(" / ");
                    }
                }
                saveObject["Schedule"] = field("Schedule").toBool();
                saveObject["ScheduleAsBusy"] = (field("scheduleBusyOrFree").toInt() == SchedulePage::busy);
                saveObject["Timezone"] = field("Timezone").toBool();
                saveObject["baseTimezone"] = field("baseTimezone").toString();
                saveObject["scheduleQuestion"] = field("ScheduleQuestion").toString();
                QList<QString> dayNames = field("scheduleDayNames").toStringList();
                for(int day = 0; day < MAX_DAYS; day++) {
                    const QString dayString1 = "scheduleDay" + QString::number(day+1);
                    const QString dayString2 = dayString1 + "Name";
                    saveObject[dayString1] = !dayNames[day].isEmpty();
                    saveObject[dayString2] = dayNames[day];
                }
                saveObject["scheduleStartHour"] = field("scheduleFrom").toInt();
                saveObject["scheduleEndHour"] = field("scheduleTo").toInt();
                saveObject["scheduleResolution"] = field("scheduleResolution").toInt();
                saveObject["scheduleTimeFormat"] = field("scheduleTimeFormat").toInt();
                saveObject["Section"] = field("Section").toBool();
                saveObject["SectionNames"] = field("SectionNames").toStringList().join(',');
                saveObject["PreferredTeammates"] = field("PrefTeammate").toBool();
                saveObject["PreferredNonTeammates"] = field("PrefNonTeammate").toBool();
                saveObject["numPrefTeammates"] = field("numPrefTeammates").toInt();
                saveObject["StudentNames"] = field("StudentNames").toStringList().join(',');

                const QJsonDocument saveDoc(saveObject);
                saveFile.write(saveDoc.toJson());
                saveFile.close();
                surveyHasBeenExported = true;
            }
            else {
                grueprGlobal::errorMessage(this, tr("No Files Saved"), tr("This survey was not saved.\nThere was an issue writing the file to disk."));
            }
        }
    }
    else if(destinationTextFiles->isChecked()) {
        QFileInfo *saveFileLocation = &(qobject_cast<SurveyMakerWizard *>(wizard()))->saveFileLocation;
        //give instructions about how this option works
        QMessageBox createSurvey;
        createSurvey.setIcon(QMessageBox::Information);
        createSurvey.setWindowTitle(tr("Survey Creation"));
        createSurvey.setText(tr("The next step will save two files to your computer:\n\n"
                                "  » A text file that lists the questions you should include in your survey.\n\n"
                                "  » A csv file that gruepr can read after you paste into it the survey data you receive."));
        createSurvey.setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);
        if(createSurvey.exec() == QMessageBox::Cancel) {
            return;
        }

        //get the filenames and location
        const QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), saveFileLocation->canonicalFilePath(), tr("text and survey files (*);;All Files (*)"));
        if(fileName.isEmpty()) {
            grueprGlobal::errorMessage(this, tr("No Files Saved"), tr("This survey was not saved.\nThere was an issue writing the files to disk."));
            return;
        }
        //create the files
        QFile saveFile(fileName + ".txt"), saveFile2(fileName + ".csv");
        if(!saveFile.open(QIODevice::WriteOnly | QIODevice::Text) || !saveFile2.open(QIODevice::WriteOnly | QIODevice::Text)) {
            grueprGlobal::errorMessage(this, tr("No Files Saved"), tr("This survey was not saved.\nThere was an issue writing the files to disk."));
            return;
        }
        saveFileLocation->setFile(QFileInfo(fileName).canonicalPath());

        QString textFileContents, csvFileContents = "Timestamp";
        int questionNumber = 0;
        if(!survey->title.isEmpty()) {
            textFileContents += survey->title + "\n\n";
        }

        textFileContents += tr("Your survey (and/or other data sources) should collect the following information to paste into the csv file \"") + fileName + ".csv\":";
        for(const auto &question : qAsConst(survey->questions)) {
            textFileContents += "\n\n  " + QString::number(++questionNumber) + ") " + question.text;
            if(question.type == Question::QuestionType::schedule) {
                textFileContents += "\n                 ";
                for(const auto &timeName : survey->schedTimeNames) {
                    textFileContents += timeName + "    ";
                }
                textFileContents += "\n";
                for(const auto &dayName : qAsConst(survey->schedDayNames)) {
                    if(!(dayName.isEmpty())) {
                        textFileContents += "\n      " + dayName + "\n";
                        csvFileContents +=  "," + (question.text.contains(',')? "\"" + question.text + "\"" : question.text) + "[" + dayName + "]";
                    }
                }
            }
            else {
                csvFileContents += "," + (question.text.contains(',')? "\"" + question.text + "\"" : question.text);
                if((question.type == Question::QuestionType::dropdown) ||
                   (question.type == Question::QuestionType::radiobutton) ||
                   (question.type == Question::QuestionType::checkbox)) {
                    textFileContents += "\n     " + tr("choices") + ": [" + question.options.join(" | ") + "]";
                    if(question.type == Question::QuestionType::checkbox) {
                        textFileContents += "\n     (" + tr("Select one or more") + ")";
                    }
                }
            }
        }

        //write the files
        QTextStream output(&saveFile), output2(&saveFile2);
        output << textFileContents;
        output2 << csvFileContents;
        saveFile.close();
        saveFile2.close();
        surveyHasBeenExported = true;
    }
    else if(destinationGoogle->isChecked()) {
        if(!grueprGlobal::internetIsGood()) {
            return;
        }

        //create googleHandler and/or authenticate as needed
        if(google == nullptr) {
            google = new GoogleHandler;
        }
        if(!google->authenticated) {
            auto *loginDialog = new QMessageBox(this);
            loginDialog->setStyleSheet(LABELSTYLE);
            const QPixmap icon(":/icons_new/google.png");
            loginDialog->setIconPixmap(icon.scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            loginDialog->setText("");

            // if refreshToken is found, try to use it to get accessTokens without re-granting permission
            if(google->refreshTokenExists) {
                loginDialog->setText(tr("Contacting Google..."));
                loginDialog->setStandardButtons(QMessageBox::Cancel);
                connect(google, &GoogleHandler::granted, loginDialog, &QMessageBox::accept);
                connect(google, &GoogleHandler::denied, loginDialog, [&loginDialog]() {loginDialog->setText(tr("Google is requesting that you re-authorize gruepr.\n\n"));
                                                                                       loginDialog->accept();});

                google->authenticate();

                if(loginDialog->exec() == QMessageBox::Cancel) {
                    delete loginDialog;
                    return;
                }

                //refreshToken failed, so need to start over
                if(!google->authenticated) {
                    delete google;
                    google = new GoogleHandler;
                }
            }

            // still not authenticated, so either didn't have a refreshToken to use or the refreshToken didn't work; need to re-log in on the browser
            if(!google->authenticated) {
                loginDialog->setText(loginDialog->text() + tr("The next step will open a browser window so you can sign in with Google.\n\n"
                                                              "  » Your computer may ask whether gruepr can access the network. "
                                                              "This access is needed so that gruepr and Google can communicate.\n\n"
                                                              "  » In the browser, Google will ask whether you authorize gruepr to create and access a file on your Google Drive. "
                                                              "This access is needed so that the survey Form can be created now and the responses can be downloaded to your computer later.\n\n"
                                                              "  » All data associated with this survey, including the questions asked and responses received, will exist in your Google Drive only. "
                                                              "No data from or about this survey will ever be stored or sent anywhere else."));
                loginDialog->setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);
                auto *okButton = loginDialog->button(QMessageBox::Ok);
                auto *cancelButton = loginDialog->button(QMessageBox::Cancel);
                const int height = okButton->height();
                QPixmap loginpic(":/icons_new/google_signin_button.png");
                loginpic = loginpic.scaledToHeight(int(1.5f * float(height)), Qt::SmoothTransformation);
                okButton->setText("");
                okButton->setIconSize(loginpic.rect().size());
                okButton->setIcon(loginpic);
                okButton->adjustSize();
                QPixmap cancelpic(":/icons_new/cancel_signin_button.png");
                cancelpic = cancelpic.scaledToHeight(int(1.5f * float(height)), Qt::SmoothTransformation);
                cancelButton->setText("");
                cancelButton->setIconSize(cancelpic.rect().size());
                cancelButton->setIcon(cancelpic);
                cancelButton->adjustSize();
                if(loginDialog->exec() == QMessageBox::Cancel) {
                    delete loginDialog;
                    return;
                }

                google->authenticate();

                loginDialog->setText(tr("Please use your browser to log in to Google and then return here."));
                loginDialog->setStandardButtons(QMessageBox::Cancel);
                connect(google, &GoogleHandler::granted, loginDialog, &QMessageBox::accept);
                if(loginDialog->exec() == QMessageBox::Cancel) {
                    delete loginDialog;
                    return;
                }
            }
            delete loginDialog;
        }

        //upload the survey as a form
        auto *busyBox = google->actionDialog();
        auto form = google->createSurvey(survey);

        QPixmap icon;
        const QSize iconSize = google->actionDialogIcon->size();
        if(form.name.isEmpty()) {
            google->actionDialogLabel->setText(tr("Error. The survey was not created."));
            icon.load(":/icons_new/error.png");
            google->actionDialogIcon->setPixmap(icon.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            google->actionDialogButtons->setStandardButtons(QDialogButtonBox::Ok);
            busyBox->adjustSize();
            busyBox->exec();
            google->actionComplete(busyBox);
        }
        else {
            QEventLoop loop;
            google->actionDialogLabel->setText(tr("Success!"));
            icon.load(":/icons_new/ok.png");
            google->actionDialogIcon->setPixmap(icon.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            busyBox->adjustSize();
            QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
            loop.exec();
            google->actionComplete(busyBox);

            // append this survey to the saved values
            QSettings settings;
            const int currentArraySize = settings.beginReadArray("GoogleForm");
            settings.endArray();
            settings.beginWriteArray("GoogleForm");
            settings.setArrayIndex(currentArraySize);
            settings.setValue("name", form.name);
            settings.setValue("ID", form.ID);
            settings.setValue("createdTime", form.createdTime);
            settings.setValue("responderURL", form.responderURL);
            settings.endArray();

            auto *successDialog = new QMessageBox(this);
            successDialog->setStyleSheet(QString() + LABELSTYLE + SMALLBUTTONSTYLE);
            successDialog->setTextFormat(Qt::RichText);
            successDialog->setTextInteractionFlags(Qt::TextBrowserInteraction);
            const QString textheight = QString::number(successDialog->fontMetrics().boundingRect('G').height() * 2);
            successDialog->setText(
                                    tr("Success! Survey created.<br><br>"
                                       "If you'd like to preview or edit the survey, find it in your")
                                    + " <a href='https://drive.google.com'>Google Drive</a> "
                                      "<img height=\""
                                    + textheight + "\" width=\"" + textheight
                                    + R"(" src=":/icons_new/external-link.png">. )"
                                    + tr("If you wish, you can modify:"
                                         "<ul>"
                                         "<li>the survey title</li>"
                                         "<li>the instructions shown at the top of the survey</li>"
                                         "<li>the \"description text\" shown with any of the questions</li>"
                                         "</ul>"
                                         "Changing the order of the questions or the wording of a question in any "
                                         "other way is not recommended.<br><br>"
                                         "Students should fill out the survey by going to the following "
                                         "URL:<br><strong>")
                + form.responderURL.toEncoded()
                + tr(
                    "</strong><br>You can copy this URL to your clipboard with the button below."));
            successDialog->setStandardButtons(QMessageBox::Ok);
            auto *copyButton = successDialog->addButton(tr("Copy URL to clipboard"), QMessageBox::ResetRole);
            copyButton->setStyleSheet(copyButton->styleSheet() + QString(BIGTOOLTIPSTYLE).replace( "background-color: white;", "background-color: green;"));
            copyButton->disconnect();     // disconnect the button from all slots so that it doesn't close the dialog when clicked
            connect(copyButton, &QPushButton::clicked, successDialog, [&form, &copyButton](){QClipboard *clipboard = QGuiApplication::clipboard();
                                                                                             clipboard->setText(form.responderURL.toEncoded());
                                                                                             QToolTip::showText(copyButton->mapToGlobal(QPoint(0, 0)),
                                                                                                                tr("URL copied"), copyButton, QRect(),
                                                                                                                UI_DISPLAY_DELAYTIME);});
            successDialog->exec();
            successDialog->deleteLater();
            surveyHasBeenExported = true;
        }
    }
    else if(destinationCanvas->isChecked()) {
        if(!grueprGlobal::internetIsGood()) {
            return;
        }

        //create canvasHandler and/or authenticate as needed
        if(canvas == nullptr) {
            canvas = new CanvasHandler();
        }
        if(!canvas->authenticated) {
            //IN BETA--GETS USER'S API TOKEM MANUALLY
            QSettings savedSettings;
            QString savedCanvasURL = savedSettings.value("canvasURL").toString();
            QString savedCanvasToken = savedSettings.value("canvasToken").toString();

            const QStringList newURLAndToken = canvas->askUserForManualToken(savedCanvasURL, savedCanvasToken);
            if(newURLAndToken.isEmpty()) {
                return;
            }

            savedCanvasURL = (newURLAndToken.at(0).isEmpty() ? savedCanvasURL : newURLAndToken.at(0)).trimmed();
            if(!savedCanvasURL.startsWith("http", Qt::CaseInsensitive)) {
                savedCanvasURL.prepend("https://");
            }
            savedCanvasToken =  (newURLAndToken.at(1).isEmpty() ? savedCanvasToken : newURLAndToken.at(1)).trimmed();
            savedSettings.setValue("canvasURL", savedCanvasURL);
            savedSettings.setValue("canvasToken", savedCanvasToken);

            canvas->setBaseURL(savedCanvasURL);
            canvas->authenticate(savedCanvasToken);
        }

        //ask the user in which course we're creating the survey
        auto *busyBox = canvas->actionDialog();
        QStringList courseNames = canvas->getCourses();
        canvas->actionComplete(busyBox);

        auto *canvasCourses = new QDialog(this);
        canvasCourses->setWindowTitle(tr("Choose Canvas course"));
        canvasCourses->setWindowIcon(QIcon(":/icons_new/canvas.png"));
        canvasCourses->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        auto *vLayout = new QVBoxLayout;
        int i = 1;
        auto *label = new QLabel(tr("In which course should this survey be created?"));
        label->setStyleSheet(LABELSTYLE);
        auto *coursesComboBox = new QComboBox;
        for(const auto &courseName : qAsConst(courseNames)) {
            coursesComboBox->addItem(courseName);
            coursesComboBox->setItemData(i++, QString::number(canvas->getStudentCount(courseName)) + " students", Qt::ToolTipRole);
        }
        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
        buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        vLayout->addWidget(label);
        vLayout->addWidget(coursesComboBox);
        vLayout->addWidget(buttonBox);
        canvasCourses->setLayout(vLayout);
        connect(buttonBox, &QDialogButtonBox::accepted, canvasCourses, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, canvasCourses, &QDialog::reject);
        if((canvasCourses->exec() == QDialog::Rejected)) {
            delete canvasCourses;
            return;
        }

        //upload the survey as a quiz
        busyBox = canvas->actionDialog();
        const bool success = canvas->createSurvey(coursesComboBox->currentText(), survey);

        QPixmap icon;
        const QSize iconSize = canvas->actionDialogIcon->size();
        if(!success) {
            canvas->actionDialogLabel->setText(tr("Error. The survey was not created."));
            icon.load(":/icons_new/error.png");
            canvas->actionDialogIcon->setPixmap(icon.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            canvas->actionDialogButtons->setStandardButtons(QDialogButtonBox::Ok);
            busyBox->adjustSize();
            busyBox->exec();
            canvas->actionComplete(busyBox);
        }
        else {
            QEventLoop loop;
            canvas->actionDialogLabel->setText(tr("Success!"));
            icon.load(":/icons_new/ok.png");
            canvas->actionDialogIcon->setPixmap(icon.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            busyBox->adjustSize();
            QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
            loop.exec();
            canvas->actionComplete(busyBox);

            auto *successDialog = new QMessageBox(this);
            successDialog->setStyleSheet(QString() + LABELSTYLE + SMALLBUTTONSTYLE);
            successDialog->setTextFormat(Qt::RichText);
            successDialog->setText(tr("Success! Survey created.<br><br>"
                                       "If you'd like to preview or edit the survey, find it under the \"Quizzes\" navigation item in your course Canvas site. "
                                       "If you wish, you can modify:"
                                       "<ul>"
                                       "<li>the survey title</li>"
                                       "<li>the Quiz Instructions shown at the start of the survey</li>"
                                       "</ul>"
                                       "Changing the order or the wording of the questions is not recommended.<br><br>"
                                       "<strong>The survey is currently \"Unpublished\".</strong><br>"
                                       "When you are ready for students to fill out the survey, you must log in to your course Canvas site, \"Publish\" it, and ensure it "
                                       "is in a Module or other location accessible to the students."));
            successDialog->setStandardButtons(QMessageBox::Ok);
            successDialog->exec();
            successDialog->deleteLater();
            surveyHasBeenExported = true;
        }
        delete canvasCourses;
    }
}
