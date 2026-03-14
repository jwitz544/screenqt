#include "startscreen.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

StartScreen::StartScreen(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("startScreen");

    // ── Root layout: outer margins + centred column ───────────────────────
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addStretch(1);

    // Centre column
    auto *col = new QVBoxLayout();
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);
    col->setAlignment(Qt::AlignHCenter);

    // ── Brand block ───────────────────────────────────────────────────────
    auto *appName = new QLabel("ScreenQt", this);
    appName->setObjectName("startAppName");
    appName->setAlignment(Qt::AlignLeft);
    col->addWidget(appName);

    col->addSpacing(6);

    auto *tagline = new QLabel("A modern screenplay editor", this);
    tagline->setObjectName("startTagline");
    tagline->setAlignment(Qt::AlignLeft);
    col->addWidget(tagline);

    col->addSpacing(32);

    // ── Hairline divider ──────────────────────────────────────────────────
    auto *divider = new QFrame(this);
    divider->setFixedHeight(1);
    divider->setStyleSheet("background: #3C3C3C;");
    col->addWidget(divider);

    col->addSpacing(24);

    // ── "START" section ───────────────────────────────────────────────────
    auto *sectionLabel = new QLabel("START", this);
    sectionLabel->setObjectName("startSectionHeader");
    col->addWidget(sectionLabel);

    col->addSpacing(10);

    // Helper: action row  [button]  [shortcut hint]
    auto makeAction = [&](const QString &label, const QString &shortcut) -> QPushButton * {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(16);

        auto *btn = new QPushButton(label, this);
        btn->setObjectName("startAction");
        btn->setCursor(Qt::PointingHandCursor);
        row->addWidget(btn);

        auto *hint = new QLabel(shortcut, this);
        hint->setStyleSheet("color: #4D4D4D; font-size: 11px;");
        hint->setAlignment(Qt::AlignVCenter);
        row->addWidget(hint);
        row->addStretch();

        col->addLayout(row);
        return btn;
    };

    auto *newBtn  = makeAction("New Screenplay",       "Ctrl+N");
    col->addSpacing(4);
    auto *loadBtn = makeAction("Open Screenplay\u2026", "Ctrl+O");

    // ── "RECENT" section ──────────────────────────────────────────────────
    col->addSpacing(28);

    auto *recentDivider = new QFrame(this);
    recentDivider->setFixedHeight(1);
    recentDivider->setStyleSheet("background: #3C3C3C;");
    col->addWidget(recentDivider);

    col->addSpacing(24);

    auto *recentLabel = new QLabel("RECENT", this);
    recentLabel->setObjectName("startSectionHeader");
    col->addWidget(recentLabel);

    col->addSpacing(10);

    // Container for dynamic recent file entries
    m_recentSection = new QWidget(this);
    m_recentLayout  = new QVBoxLayout(m_recentSection);
    m_recentLayout->setContentsMargins(0, 0, 0, 0);
    m_recentLayout->setSpacing(4);
    col->addWidget(m_recentSection);

    // ── Centre the column horizontally ────────────────────────────────────
    auto *hCenter = new QHBoxLayout();
    hCenter->addStretch(1);
    hCenter->addLayout(col);
    hCenter->addStretch(1);

    root->addLayout(hCenter);
    root->addStretch(2);

    // ── Connections ───────────────────────────────────────────────────────
    connect(newBtn,  &QPushButton::clicked, this, &StartScreen::onNew);
    connect(loadBtn, &QPushButton::clicked, this, &StartScreen::onLoad);
}

void StartScreen::setRecentFiles(const QStringList &files)
{
    m_recentFiles = files;
    buildRecentSection();
}

void StartScreen::buildRecentSection()
{
    // Clear existing widgets
    QLayoutItem *item;
    while ((item = m_recentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    if (m_recentFiles.isEmpty()) {
        auto *none = new QLabel("No recent files", m_recentSection);
        none->setStyleSheet("color: #4D4D4D; font-size: 12px;");
        m_recentLayout->addWidget(none);
        return;
    }

    for (const QString &filePath : m_recentFiles) {
        QFileInfo info(filePath);

        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(12);

        auto *btn = new QPushButton(info.fileName(), m_recentSection);
        btn->setObjectName("startAction");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedWidth(200);
        row->addWidget(btn);

        // Truncate path for display
        QString displayPath = filePath;
        const int maxLen = 40;
        if (displayPath.length() > maxLen) {
            displayPath = "\u2026" + displayPath.right(maxLen - 1);
        }

        auto *pathLabel = new QLabel(displayPath, m_recentSection);
        pathLabel->setStyleSheet("color: #4D4D4D; font-size: 11px;");
        pathLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        row->addWidget(pathLabel);
        row->addStretch();

        m_recentLayout->addLayout(row);

        connect(btn, &QPushButton::clicked, this, [this, filePath] {
            emit loadDocument(filePath);
        });
    }
}

void StartScreen::onNew()
{
    emit newDocument();
}

void StartScreen::onLoad()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "Open Screenplay", "",
        "ScreenQt Files (*.sqt);;Fountain Files (*.fountain);;All Files (*)"
    );
    if (!filePath.isEmpty()) {
        emit loadDocument(filePath);
    }
}
