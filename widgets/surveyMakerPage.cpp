#include "surveyMakerPage.h"

SurveyMakerPage::SurveyMakerPage(int numQuestions, QWidget *parent)
    : QWizardPage(parent),
    numQuestions(numQuestions)
{
    layout = new QGridLayout;
    setLayout(layout);
    layout->setSpacing(0);

    pageTitle = new QLabel(this);
    pageTitle->setStyleSheet(TITLESTYLE);
    pageTitle->setAlignment(Qt::AlignCenter);
    pageTitle->setMinimumHeight(40);
    pageTitle->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    layout->addWidget(pageTitle, row++, 0, 1, -1);

    topLabel = new QLabel(this);
    topLabel->setStyleSheet(TOPLABELSTYLE);
    topLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    topLabel->setMinimumHeight(40);
    topLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    layout->addWidget(topLabel, row++, 0, 1, -1);

    previewWidget = new QWidget(this);
    previewWidget->setStyleSheet("background-color: #ebebeb;");
    preview = new QScrollArea(this);
    preview->setWidget(previewWidget);
    preview->setStyleSheet("QScrollArea{background-color: #ebebeb; border: none}");
    preview->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    preview->setWidgetResizable(true);
    previewLayout = new QVBoxLayout(previewWidget);
    previewLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    previewLayout->setSpacing(0);
    layout->addWidget(preview, row, 1, -1, 1);

    layout->setColumnStretch(0,1);
    layout->setColumnStretch(1,1);

    questions = new SurveyMakerQuestionWithSwitch[numQuestions];
    questionPreviews = new QWidget[numQuestions];
    questionPreviewLayouts = new QVBoxLayout[numQuestions];
    questionPreviewTopLabels = new QLabel[numQuestions];
    questionPreviewBottomLabels = new QLabel[numQuestions];
    for(int i = 0; i < numQuestions; i++) {
        layout->setRowMinimumHeight(row++, 10);
        layout->addWidget(&questions[i], row++, 0, 1, 1);

        questionPreviews[i].setLayout(&questionPreviewLayouts[i]);
        previewLayout->addWidget(&questionPreviews[i]);
        questionPreviewTopLabels[i].setStyleSheet(SURVEYMAKERLABELSTYLE);
        questionPreviewBottomLabels[i].setStyleSheet(SURVEYMAKERLABELSTYLE);
        connect(&questions[i], &SurveyMakerQuestionWithSwitch::valueChanged, &questionPreviews[i], &QWidget::setVisible);
    }
    layout->addItem(new QSpacerItem(0,0), row, 0);
    layout->setRowStretch(row, 1);

    previewLayout->addStretch(1);
}

SurveyMakerPage::~SurveyMakerPage()
{
    delete[] questions;
    delete[] questionPreviewTopLabels;
    delete[] questionPreviewBottomLabels;
    delete[] questionPreviewLayouts;
    delete[] questionPreviews;
    delete layout;
}
