#include "fountainio.h"
#include "scripteditor.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QVector>

namespace {

// ---------------------------------------------------------------------------
// Classification helpers
// ---------------------------------------------------------------------------

static bool isSceneHeadingLine(const QString &line)
{
    if (line.isEmpty()) return false;
    // Forced scene heading: starts with single dot
    if (line.startsWith('.') && !line.startsWith("..")) return true;

    static const QRegularExpression re(
        "^(INT\\.?|EXT\\.?|INT\\.?/EXT\\.?|INT\\./EXT\\.|I\\.?/E\\.?)[\\s\\.\\-/]",
        QRegularExpression::CaseInsensitiveOption
    );
    return re.match(line).hasMatch();
}

static bool isTransitionLine(const QString &line)
{
    if (line.isEmpty()) return false;
    if (line.startsWith('>')) return true; // forced transition
    const QString upper = line.toUpper();
    return (line == upper) && (line.endsWith("TO:") || line == "FADE OUT." || line == "FADE OUT");
}

static bool isCharacterLine(const QString &line)
{
    if (line.isEmpty()) return false;
    if (line.startsWith('@')) return true; // forced character
    if (line != line.toUpper()) return false;
    // Must contain at least one letter
    for (const QChar c : line) {
        if (c.isLetter()) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------------

bool saveAsFountain(ScriptEditor *editor, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    QTextDocument *doc = editor->document();
    QTextBlock block = doc->begin();
    int prevState = -1;

    while (block.isValid()) {
        const int state = block.userState();
        const QString text = block.text();

        // Determine whether to insert a blank line before this element
        bool needBlank = true;
        if (state == ScriptEditor::Dialogue || state == ScriptEditor::Parenthetical) {
            // Continuation of a character's speech — no blank line before
            if (prevState == ScriptEditor::CharacterName
                    || prevState == ScriptEditor::Dialogue
                    || prevState == ScriptEditor::Parenthetical) {
                needBlank = false;
            }
        }

        if (needBlank && prevState != -1) {
            out << "\n";
        }

        // Format the line
        switch (state) {
        case ScriptEditor::Parenthetical: {
            // Ensure wrapped in parens
            QString ptext = text.trimmed();
            if (!ptext.startsWith('(')) ptext.prepend('(');
            if (!ptext.endsWith(')'))  ptext.append(')');
            out << ptext << "\n";
            break;
        }
        default:
            out << text << "\n";
            break;
        }

        prevState = state;
        block = block.next();
    }

    file.close();
    return true;
}

// ---------------------------------------------------------------------------
// Load
// ---------------------------------------------------------------------------

bool loadFromFountain(ScriptEditor *editor, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    const QString text = in.readAll();
    file.close();

    QStringList rawLines = text.split('\n');

    // Normalise line endings (remove \r)
    for (QString &l : rawLines) {
        if (l.endsWith('\r')) l.chop(1);
    }

    // Skip Fountain title page block (key:value pairs at start before first blank line)
    int startLine = 0;
    {
        static const QRegularExpression titleKeyRe("^[A-Za-z ]+\\s*:");
        bool inTitleBlock = true;
        for (int i = 0; i < rawLines.size() && inTitleBlock; ++i) {
            const QString &l = rawLines[i];
            if (l.isEmpty()) {
                // Blank line ends title block
                startLine = i + 1;
                inTitleBlock = false;
            } else if (!titleKeyRe.match(l).hasMatch()) {
                // Not a key: line — no title block
                inTitleBlock = false;
                startLine = 0;
            }
        }
    }

    // Collect blocks: {elementType, text}
    struct Block {
        int type;
        QString text;
    };
    QVector<Block> blocks;

    // State machine
    enum class State { None, CharContext, DialogueContext };
    State state = State::None;

    for (int i = startLine; i < rawLines.size(); ++i) {
        const QString &line = rawLines[i];

        if (line.trimmed().isEmpty()) {
            state = State::None;
            continue;
        }

        switch (state) {
        case State::None: {
            if (isSceneHeadingLine(line)) {
                // Strip leading dot for forced scene headings
                QString heading = line;
                if (heading.startsWith('.') && !heading.startsWith("..")) heading = heading.mid(1);
                blocks.append({ScriptEditor::SceneHeading, heading});
            } else if (isTransitionLine(line)) {
                // Strip leading > for forced transitions
                QString trans = line;
                if (trans.startsWith('>')) trans = trans.mid(1).trimmed();
                blocks.append({ScriptEditor::Transition, trans});
            } else if (isCharacterLine(line)) {
                // Strip leading @ for forced character
                QString charName = line;
                if (charName.startsWith('@')) charName = charName.mid(1);
                // Strip dual dialogue marker
                if (charName.endsWith('^')) charName.chop(1);
                charName = charName.trimmed();
                blocks.append({ScriptEditor::CharacterName, charName});
                state = State::CharContext;
            } else {
                blocks.append({ScriptEditor::Action, line});
            }
            break;
        }
        case State::CharContext:
        case State::DialogueContext: {
            const QString trimmed = line.trimmed();
            if (trimmed.startsWith('(') && trimmed.endsWith(')')) {
                blocks.append({ScriptEditor::Parenthetical, trimmed});
                state = State::CharContext; // more dialogue may follow
            } else {
                blocks.append({ScriptEditor::Dialogue, line});
                state = State::DialogueContext;
            }
            break;
        }
        }
    }

    if (blocks.isEmpty()) return false;

    // Insert into editor
    editor->clear();
    QTextCursor cursor(editor->document());
    cursor.beginEditBlock();

    for (int i = 0; i < blocks.size(); ++i) {
        cursor.insertText(blocks[i].text);
        cursor.block().setUserState(blocks[i].type);
        if (i + 1 < blocks.size()) {
            cursor.insertText("\n");
            cursor.movePosition(QTextCursor::NextBlock);
        }
    }

    cursor.endEditBlock();
    return true;
}

} // namespace

namespace FountainIO {

bool saveFountain(ScriptEditor *editor, const QString &filePath)
{
    return saveAsFountain(editor, filePath);
}

bool loadFountain(ScriptEditor *editor, const QString &filePath)
{
    return loadFromFountain(editor, filePath);
}

} // namespace FountainIO
