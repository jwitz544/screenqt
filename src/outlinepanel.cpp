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
    setObjectName("outlinePanel");
    setMinimumHeight(90);
    setMinimumWidth(240);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);

    m_sceneCountLabel = new QLabel("0 scenes", this);
    m_sceneCountLabel->setObjectName("panelMeta");
    layout->addWidget(m_sceneCountLabel);

    QFrame *sceneListCard = new QFrame(this);
    sceneListCard->setObjectName("panelGroup");
    auto *sceneCardLayout = new QVBoxLayout(sceneListCard);
    sceneCardLayout->setContentsMargins(4, 4, 4, 4);
    sceneCardLayout->setSpacing(0);

    m_sceneList = new QListWidget(sceneListCard);
    m_sceneList->setObjectName("sceneList");
    m_sceneList->setSpacing(0);
    m_sceneList->setUniformItemSizes(true);
    sceneCardLayout->addWidget(m_sceneList, 1);
    layout->addWidget(sceneListCard, 1);

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
        item->setSizeHint(QSize(item->sizeHint().width(), 26));
        sceneNumber++;
    }

    const int sceneCount = sceneNumber - 1;
    m_sceneCountLabel->setText(sceneCount == 1 ? "1 scene" : QString("%1 scenes").arg(sceneCount));

    if (sceneCount == 0) {
        auto *emptyItem = new QListWidgetItem("No scenes yet", m_sceneList);
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsSelectable);
        emptyItem->setForeground(QColor("#7f8ca3"));
        emptyItem->setSizeHint(QSize(emptyItem->sizeHint().width(), 24));
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
