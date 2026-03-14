#include "findbar.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

FindBar::FindBar(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("findBar");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 4, 8, 4);
    mainLayout->setSpacing(2);

    // --- Row 1: Find ---
    auto *findRow = new QHBoxLayout();
    findRow->setSpacing(6);

    auto *findLabel = new QLabel("Find", this);
    findRow->addWidget(findLabel);

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Find in script");
    findRow->addWidget(m_input, 1);

    m_prevButton = new QPushButton("Prev", this);
    m_nextButton = new QPushButton("Next", this);
    findRow->addWidget(m_prevButton);
    findRow->addWidget(m_nextButton);

    m_caseCheck = new QCheckBox("Aa", this);
    m_caseCheck->setToolTip("Case sensitive");
    m_wordCheck = new QCheckBox("Word", this);
    m_wordCheck->setToolTip("Whole word");
    findRow->addWidget(m_caseCheck);
    findRow->addWidget(m_wordCheck);

    m_closeButton = new QPushButton("\u2715", this);
    m_closeButton->setFixedWidth(24);
    findRow->addWidget(m_closeButton);

    mainLayout->addLayout(findRow);

    // --- Row 2: Replace ---
    auto *replaceRow = new QHBoxLayout();
    replaceRow->setSpacing(6);

    auto *replaceLabel = new QLabel("Replace", this);
    replaceRow->addWidget(replaceLabel);

    m_replaceInput = new QLineEdit(this);
    m_replaceInput->setPlaceholderText("Replacement text");
    replaceRow->addWidget(m_replaceInput, 1);

    m_replaceButton = new QPushButton("Replace", this);
    m_replaceAllButton = new QPushButton("Replace All", this);
    replaceRow->addWidget(m_replaceButton);
    replaceRow->addWidget(m_replaceAllButton);

    m_replaceStatusLabel = new QLabel(this);
    m_replaceStatusLabel->setObjectName("panelMeta");
    replaceRow->addWidget(m_replaceStatusLabel);

    mainLayout->addLayout(replaceRow);

    // Connections
    connect(m_input, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_replaceStatusLabel->clear();
        emit queryChanged(text);
    });
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

    connect(m_replaceButton, &QPushButton::clicked, this, [this] {
        emit replaceRequested(m_replaceInput->text());
    });
    connect(m_replaceAllButton, &QPushButton::clicked, this, [this] {
        emit replaceAllRequested(m_input->text(), m_replaceInput->text());
    });
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

void FindBar::setReplaceStatus(int count)
{
    if (count <= 0) {
        m_replaceStatusLabel->setText("No replacements");
    } else {
        m_replaceStatusLabel->setText(QString("%1 replaced").arg(count));
    }
}

QString FindBar::findQuery() const
{
    return m_input->text();
}
