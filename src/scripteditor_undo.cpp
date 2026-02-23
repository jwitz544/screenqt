#include "scripteditor_undo.h"
#include "scripteditor.h"
#include <QTextCursor>
#include <QTextBlock>

namespace ScriptEditorUndo {

QString normalizeSelectedText(const QString &text)
{
    QString normalized = text;
    normalized.replace(QChar(0x2029), QChar('\n'));
    return normalized;
}

CompoundCommand::CompoundCommand(const QString &text) : QUndoCommand(text) {}

InsertTextCommand::InsertTextCommand(ScriptEditor *editor, int pos, const QString &text,
                                     ScriptEditor::UndoGroupType type, bool allowMerge,
                                     QUndoCommand *parent)
    : QUndoCommand(parent), m_editor(editor), m_pos(pos), m_text(text),
      m_type(type), m_allowMerge(allowMerge) {}

int InsertTextCommand::id() const { return 1; }

bool InsertTextCommand::mergeWith(const QUndoCommand *other)
{
    const auto *o = dynamic_cast<const InsertTextCommand *>(other);
    
    qDebug() << "[InsertTextCommand::mergeWith] Attempting merge:";
    qDebug() << "  Current: pos=" << m_pos << "text='" << m_text << "' type=" << (int)m_type << "allowMerge=" << m_allowMerge;
    qDebug() << "  Other: pos=" << o->m_pos << "text='" << o->m_text << "' type=" << (int)o->m_type << "allowMerge=" << o->m_allowMerge;
    
    if (!o || !m_allowMerge || !o->m_allowMerge) {
        qDebug() << "  REJECTED: allowMerge check failed";
        return false;
    }
    if (o->m_pos != m_pos + m_text.length()) {
        qDebug() << "  REJECTED: position mismatch (expected=" << (m_pos + m_text.length()) << ")";
        return false;
    }
    
    // Allow same type to merge
    if (m_type == o->m_type) {
        qDebug() << "  ACCEPTED: same type merge";
        m_text += o->m_text;
        qDebug() << "  Result: text='" << m_text << "' type=" << (int)m_type;
        return true;
    }
    
    // Allow Word to merge with preceding Whitespace (space merges into next word)
    // But after merging, this becomes a Word command and won't accept more Whitespace
    if (m_type == ScriptEditor::UndoGroupType::Whitespace && 
        o->m_type == ScriptEditor::UndoGroupType::Word) {
        qDebug() << "  ACCEPTED: Whitespace merging with Word (space + word)";
        m_text += o->m_text;
        m_type = ScriptEditor::UndoGroupType::Word;  // Change type to Word after merging
        qDebug() << "  Result: text='" << m_text << "' type=" << (int)m_type << " (changed to Word)";
        return true;
    }
    
    qDebug() << "  REJECTED: no matching merge rule";
    return false;
}

void InsertTextCommand::redo()
{
    QTextCursor c(m_editor->document());
    c.setPosition(m_pos);
    c.insertText(m_text);
    c.setPosition(m_pos + m_text.length());
    m_editor->setTextCursor(c);
}

void InsertTextCommand::undo()
{
    QTextCursor c(m_editor->document());
    c.setPosition(m_pos);
    c.setPosition(m_pos + m_text.length(), QTextCursor::KeepAnchor);
    c.removeSelectedText();
    c.setPosition(m_pos);
    m_editor->setTextCursor(c);
}

DeleteTextCommand::DeleteTextCommand(ScriptEditor *editor, int pos, const QString &text,
                                     ScriptEditor::UndoGroupType type, bool allowMerge,
                                     bool backspace, QUndoCommand *parent)
    : QUndoCommand(parent), m_editor(editor), m_pos(pos), m_text(text),
      m_type(type), m_allowMerge(allowMerge), m_backspace(backspace) {}

int DeleteTextCommand::id() const { return 2; }

bool DeleteTextCommand::mergeWith(const QUndoCommand *other)
{
    const auto *o = dynamic_cast<const DeleteTextCommand *>(other);
    if (!o || !m_allowMerge || !o->m_allowMerge || m_type != o->m_type || m_backspace != o->m_backspace) {
        return false;
    }

    if (m_backspace) {
        if (o->m_pos + o->m_text.length() != m_pos) {
            return false;
        }
        m_pos = o->m_pos;
        m_text = o->m_text + m_text;
        return true;
    }

    if (o->m_pos != m_pos) {
        return false;
    }
    m_text += o->m_text;
    return true;
}

void DeleteTextCommand::redo()
{
    QTextCursor c(m_editor->document());
    c.setPosition(m_pos);
    c.setPosition(m_pos + m_text.length(), QTextCursor::KeepAnchor);
    c.removeSelectedText();
    c.setPosition(m_pos);
    m_editor->setTextCursor(c);
}

void DeleteTextCommand::undo()
{
    QTextCursor c(m_editor->document());
    c.setPosition(m_pos);
    c.insertText(m_text);
    c.setPosition(m_pos + m_text.length());
    m_editor->setTextCursor(c);
}

FormatCommand::FormatCommand(ScriptEditor *editor, int blockPos, const QTextBlockFormat &newBlock,
                             const QTextCharFormat &newChar, int newState, QUndoCommand *parent)
    : QUndoCommand(parent), m_editor(editor), m_blockPos(blockPos), m_newBlock(newBlock),
      m_newChar(newChar), m_newState(newState) {}

void FormatCommand::redo()
{
    if (!m_captured) {
        QTextCursor c(m_editor->document());
        c.setPosition(m_blockPos);
        QTextBlock block = c.block();
        m_oldBlock = block.blockFormat();
        m_oldChar = block.charFormat();
        m_oldState = block.userState();
        m_oldText = block.text();
        m_captured = true;
    }
    apply(m_newBlock, m_newChar, m_newState);
}

void FormatCommand::undo()
{
    applyWithText(m_oldBlock, m_oldChar, m_oldState, m_oldText);
}

void FormatCommand::apply(const QTextBlockFormat &bf, const QTextCharFormat &cf, int state)
{
    QTextCursor c(m_editor->document());
    c.setPosition(m_blockPos);
    c.setBlockFormat(bf);
    c.setBlockCharFormat(cf);
    QTextBlock block = c.block();
    block.setUserState(state);
    if (m_editor->textCursor().block().position() == m_blockPos) {
        emit m_editor->elementChanged(static_cast<ScriptEditor::ElementType>(state));
    }
}

void FormatCommand::applyWithText(const QTextBlockFormat &bf, const QTextCharFormat &cf, int state, const QString &text)
{
    QTextCursor c(m_editor->document());
    c.setPosition(m_blockPos);
    QTextBlock block = c.block();
    
    // Replace text with original (preserving original capitalization)
    c.select(QTextCursor::BlockUnderCursor);
    c.insertText(text);
    
    // Apply formats
    c.setPosition(m_blockPos);
    c.setBlockFormat(bf);
    c.setBlockCharFormat(cf);
    block = c.block();
    block.setUserState(state);
    
    if (m_editor->textCursor().block().position() == m_blockPos) {
        emit m_editor->elementChanged(static_cast<ScriptEditor::ElementType>(state));
    }
}

} // namespace ScriptEditorUndo
