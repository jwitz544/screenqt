#pragma once

#include "scripteditor.h"
#include <QString>
#include <QUndoCommand>
#include <QTextBlockFormat>
#include <QTextCharFormat>

namespace ScriptEditorUndo {

QString normalizeSelectedText(const QString &text);

class CompoundCommand : public QUndoCommand {
public:
    explicit CompoundCommand(const QString &text = QString());
};

class InsertTextCommand : public QUndoCommand {
public:
    InsertTextCommand(ScriptEditor *editor, int pos, const QString &text,
                      ScriptEditor::UndoGroupType type, bool allowMerge,
                      QUndoCommand *parent = nullptr);

    int id() const override;
    bool mergeWith(const QUndoCommand *other) override;
    void redo() override;
    void undo() override;

private:
    ScriptEditor *m_editor;
    int m_pos;
    QString m_text;
    ScriptEditor::UndoGroupType m_type;
    bool m_allowMerge;
};

class DeleteTextCommand : public QUndoCommand {
public:
    DeleteTextCommand(ScriptEditor *editor, int pos, const QString &text,
                      ScriptEditor::UndoGroupType type, bool allowMerge,
                      bool backspace, QUndoCommand *parent = nullptr);

    int id() const override;
    bool mergeWith(const QUndoCommand *other) override;
    void redo() override;
    void undo() override;

private:
    ScriptEditor *m_editor;
    int m_pos;
    QString m_text;
    ScriptEditor::UndoGroupType m_type;
    bool m_allowMerge;
    bool m_backspace;
};

class FormatCommand : public QUndoCommand {
public:
    FormatCommand(ScriptEditor *editor, int blockPos, const QTextBlockFormat &newBlock,
                  const QTextCharFormat &newChar, int newState, QUndoCommand *parent = nullptr);

    void redo() override;
    void undo() override;

private:
    void apply(const QTextBlockFormat &bf, const QTextCharFormat &cf, int state);
    void applyWithText(const QTextBlockFormat &bf, const QTextCharFormat &cf, int state, const QString &text);

    ScriptEditor *m_editor;
    int m_blockPos;
    QTextBlockFormat m_newBlock;
    QTextCharFormat m_newChar;
    int m_newState = 0;
    bool m_captured = false;
    QTextBlockFormat m_oldBlock;
    QTextCharFormat m_oldChar;
    int m_oldState = 0;
    QString m_oldText;
};

} // namespace ScriptEditorUndo
