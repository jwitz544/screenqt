#include "outlinepanel.h"

#include "scripteditor.h"

#include <QLabel>
#include <QListWidget>
#include <QSizePolicy>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>

namespace {
constexpr int ScenePositionRole = Qt::UserRole + 1;
}

OutlinePanel::OutlinePanel(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(90);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto *title = new QLabel("Outline", this);
    title->setStyleSheet("font-weight: 600; font-size: 13px; color: #202124; padding: 0 2px;");
    layout->addWidget(title);

    m_sceneList = new QListWidget(this);
    m_sceneList->setStyleSheet(
        "QListWidget {"
        "  border: 1px solid #d0d7de;"
        "  border-radius: 8px;"
        "  background: #ffffff;"
        "  padding: 2px;"
        "}"
        "QListWidget::item {"
        "  padding: 6px 8px;"
        "  border-radius: 6px;"
        "  color: #24292f;"
        "}"
        "QListWidget::item:hover { background: #f3f4f6; }"
        "QListWidget::item:selected { background: #dbeafe; color: #1e3a8a; }"
    );
    layout->addWidget(m_sceneList, 1);

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
