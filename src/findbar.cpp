#include "findbar.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

FindBar::FindBar(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("findBar");

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(6);

    auto *label = new QLabel("Find", this);
    layout->addWidget(label);

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Find in script");
    layout->addWidget(m_input, 1);

    m_prevButton = new QPushButton("Prev", this);
    m_nextButton = new QPushButton("Next", this);
    layout->addWidget(m_prevButton);
    layout->addWidget(m_nextButton);

    m_caseCheck = new QCheckBox("Aa", this);
    m_caseCheck->setToolTip("Case sensitive");
    m_wordCheck = new QCheckBox("Word", this);
    m_wordCheck->setToolTip("Whole word");
    layout->addWidget(m_caseCheck);
    layout->addWidget(m_wordCheck);

    m_closeButton = new QPushButton("✕", this);
    m_closeButton->setFixedWidth(24);
    layout->addWidget(m_closeButton);

    connect(m_input, &QLineEdit::textChanged, this, &FindBar::queryChanged);
    connect(m_prevButton, &QPushButton::clicked, this, &FindBar::findPreviousRequested);
    connect(m_nextButton, &QPushButton::clicked, this, &FindBar::findNextRequested);
    connect(m_caseCheck, &QCheckBox::toggled, this, [this] {
        emit optionsChanged(m_caseCheck->isChecked(), m_wordCheck->isChecked());
    });
    connect(m_wordCheck, &QCheckBox::toggled, this, [this] {
        emit optionsChanged(m_caseCheck->isChecked(), m_wordCheck->isChecked());
    });
    connect(m_closeButton, &QPushButton::clicked, this, &FindBar::closeRequested);

    connect(m_input, &QLineEdit::returnPressed, this, &FindBar::findNextRequested);
}

void FindBar::focusAndSelectAll()
{
    m_input->setFocus();
    m_input->selectAll();
}

void FindBar::setMatchStatus(int currentIndex, int totalMatches)
{
    if (totalMatches <= 0) {
        m_input->setToolTip("No matches");
        return;
    }
    m_input->setToolTip(QString("Match %1 of %2").arg(currentIndex + 1).arg(totalMatches));
}
