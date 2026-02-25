#include "outlinepanel.h"

#include "scripteditor.h"

#include <QLabel>
#include <QListWidget>
#include <QSizePolicy>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QFrame>

namespace {
constexpr int ScenePositionRole = Qt::UserRole + 1;
}

OutlinePanel::OutlinePanel(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(90);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto *title = new QLabel("Outline", this);
    title->setStyleSheet("font-weight: 700; font-size: 13px; color: #111827; padding: 0 2px;");
    layout->addWidget(title);

    m_sceneCountLabel = new QLabel("0 scenes", this);
    m_sceneCountLabel->setStyleSheet("color: #6b7280; font-size: 11px; padding: 0 2px;");
    layout->addWidget(m_sceneCountLabel);

    QFrame *listCard = new QFrame(this);
    listCard->setStyleSheet(
        "QFrame {"
        "  background: #ffffff;"
        "  border: 1px solid #d0d7de;"
        "  border-radius: 10px;"
        "}"
    );
    auto *cardLayout = new QVBoxLayout(listCard);
    cardLayout->setContentsMargins(4, 4, 4, 4);
    cardLayout->setSpacing(0);

    m_sceneList = new QListWidget(listCard);
    m_sceneList->setStyleSheet(
        "QListWidget {"
        "  border: none;"
        "  border-radius: 8px;"
        "  background: #ffffff;"
        "  padding: 2px;"
        "}"
        "QListWidget::item {"
        "  padding: 7px 8px;"
        "  border-radius: 6px;"
        "  color: #24292f;"
        "}"
        "QListWidget::item:hover { background: #f3f4f6; }"
        "QListWidget::item:selected { background: #dbeafe; color: #1e3a8a; font-weight: 600; }"
        "QScrollBar:vertical { background: #f3f4f6; width: 10px; margin: 2px; border-radius: 5px; }"
        "QScrollBar::handle:vertical { background: #c2c9d3; min-height: 24px; border-radius: 5px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );
    cardLayout->addWidget(m_sceneList, 1);
    layout->addWidget(listCard, 1);

    connect(m_sceneList, &QListWidget::itemClicked, this, &OutlinePanel::goToScene);
}

void OutlinePanel::setEditor(ScriptEditor *editor)
{
    if (m_editor == editor) {
        return;
    }

    if (m_editor) {
        disconnect(m_editor->document(), &QTextDocument::contentsChanged, this, &OutlinePanel::refreshOutline);
        disconnect(m_editor, &QTextEdit::cursorPositionChanged, this, &OutlinePanel::syncSelectionToCursor);
    }

    m_editor = editor;

    if (m_editor) {
        connect(m_editor->document(), &QTextDocument::contentsChanged, this, &OutlinePanel::refreshOutline);
        connect(m_editor, &QTextEdit::cursorPositionChanged, this, &OutlinePanel::syncSelectionToCursor);
    }

    refreshOutline();
}

void OutlinePanel::refreshOutline()
{
    m_sceneList->clear();

    if (!m_editor) {
        return;
    }

    QTextDocument *doc = m_editor->document();
    int sceneNumber = 1;

    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        if (block.userState() != static_cast<int>(ScriptEditor::SceneHeading)) {
            continue;
        }

        const QString sceneText = block.text().trimmed();
        if (sceneText.isEmpty()) {
            continue;
        }

        const QString label = QString("%1. %2").arg(sceneNumber).arg(sceneText);
        auto *item = new QListWidgetItem(label, m_sceneList);
        item->setData(ScenePositionRole, block.position());
        sceneNumber++;
    }

    const int sceneCount = sceneNumber - 1;
    m_sceneCountLabel->setText(sceneCount == 1 ? "1 scene" : QString("%1 scenes").arg(sceneCount));

    if (sceneCount == 0) {
        auto *emptyItem = new QListWidgetItem("No scenes yet", m_sceneList);
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsSelectable);
        emptyItem->setForeground(QColor("#9ca3af"));
    }

    syncSelectionToCursor();
}

void OutlinePanel::goToScene(QListWidgetItem *item)
{
    if (!m_editor || !item) {
        return;
    }

    const int pos = item->data(ScenePositionRole).toInt();
    QTextCursor cursor = m_editor->textCursor();
    cursor.setPosition(pos);
    m_editor->setTextCursor(cursor);
    m_editor->setFocus();
}

void OutlinePanel::syncSelectionToCursor()
{
    if (!m_editor || m_updatingSelection || m_sceneList->count() == 0) {
        return;
    }

    const int cursorPos = m_editor->textCursor().position();
    int bestIndex = -1;

    for (int i = 0; i < m_sceneList->count(); ++i) {
        QListWidgetItem *item = m_sceneList->item(i);
        const int scenePos = item->data(ScenePositionRole).toInt();
        if (scenePos <= cursorPos) {
            bestIndex = i;
        } else {
            break;
        }
    }

    m_updatingSelection = true;
    if (bestIndex >= 0) {
        m_sceneList->setCurrentRow(bestIndex);
    } else {
        m_sceneList->clearSelection();
    }
    m_updatingSelection = false;
}
