#include "characterspanel.h"

#include "scripteditor.h"

#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QSizePolicy>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>

namespace {
constexpr int CharacterPositionRole = Qt::UserRole + 2;
}

CharactersPanel::CharactersPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("charactersPanel");
    setMinimumHeight(90);
    setMinimumWidth(240);
    setMaximumWidth(240);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);

    auto *title = new QLabel("Characters", this);
    title->setObjectName("panelTitle");
    layout->addWidget(title);

    m_characterCountLabel = new QLabel("0 characters", this);
    m_characterCountLabel->setObjectName("panelMeta");
    layout->addWidget(m_characterCountLabel);

    QFrame *listCard = new QFrame(this);
    listCard->setObjectName("panelGroup");
    auto *cardLayout = new QVBoxLayout(listCard);
    cardLayout->setContentsMargins(4, 4, 4, 4);
    cardLayout->setSpacing(0);

    m_characterList = new QListWidget(listCard);
    m_characterList->setObjectName("sceneList");
    m_characterList->setSpacing(2);
    cardLayout->addWidget(m_characterList, 1);
    layout->addWidget(listCard, 1);

    connect(m_characterList, &QListWidget::itemClicked, this, &CharactersPanel::goToCharacter);
}

void CharactersPanel::setEditor(ScriptEditor *editor)
{
    if (m_editor == editor) {
        return;
    }

    if (m_editor) {
        disconnect(m_editor->document(), &QTextDocument::contentsChanged, this, &CharactersPanel::refreshCharacters);
    }

    m_editor = editor;

    if (m_editor) {
        connect(m_editor->document(), &QTextDocument::contentsChanged, this, &CharactersPanel::refreshCharacters);
    }

    refreshCharacters();
}

void CharactersPanel::refreshCharacters()
{
    m_characterList->clear();

    if (!m_editor) {
        return;
    }

    QMap<QString, int> characters;
    QTextDocument *doc = m_editor->document();

    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        if (block.userState() != static_cast<int>(ScriptEditor::CharacterName)) {
            continue;
        }

        const QString characterName = block.text().trimmed().toUpper();
        if (characterName.isEmpty() || characters.contains(characterName)) {
            continue;
        }

        characters.insert(characterName, block.position());
    }

    for (auto it = characters.cbegin(); it != characters.cend(); ++it) {
        auto *item = new QListWidgetItem(it.key(), m_characterList);
        item->setData(CharacterPositionRole, it.value());
    }

    const int characterCount = characters.size();
    m_characterCountLabel->setText(characterCount == 1 ? "1 character" : QString("%1 characters").arg(characterCount));

    if (characterCount == 0) {
        auto *emptyItem = new QListWidgetItem("No characters yet", m_characterList);
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsSelectable);
        emptyItem->setForeground(QColor("#7f8ca3"));
    }
}

void CharactersPanel::goToCharacter(QListWidgetItem *item)
{
    if (!m_editor || !item) {
        return;
    }

    const int pos = item->data(CharacterPositionRole).toInt();
    QTextCursor cursor = m_editor->textCursor();
    cursor.setPosition(pos);
    m_editor->setTextCursor(cursor);
    m_editor->setFocus();
}
