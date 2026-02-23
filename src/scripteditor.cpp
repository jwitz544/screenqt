#include "scripteditor.h"
#include <QTextCursor>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QKeyEvent>
#include <QScrollBar>
#include <QDebug>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QTextLayout>
#include <QMouseEvent>
#include <QFocusEvent>
#include <QClipboard>
#include "scripteditor_undo.h"

using ScriptEditorUndo::CompoundCommand;
using ScriptEditorUndo::DeleteTextCommand;
using ScriptEditorUndo::FormatCommand;
using ScriptEditorUndo::InsertTextCommand;
using ScriptEditorUndo::normalizeSelectedText;

ScriptEditor::ScriptEditor(QWidget *parent)
    : QTextEdit(parent)
{
    // Basic font: Courier 12
    QFont f("Courier New", 12);
    f.setStyleHint(QFont::TypeWriter);
    setFont(f);

    // Line wrap will be set by PageView based on printable width
    setLineWrapMode(QTextEdit::FixedPixelWidth);

    // Minimal margins - page view handles the actual page margins
    setViewportMargins(0, 0, 0, 0);

    // Disable document undo/redo (custom undo stack handles it)
    qDebug() << "[ScriptEditor] Constructor: Disabling document undo/redo";
    document()->setUndoRedoEnabled(false);
    setUndoRedoEnabled(false);

    // Start with Action element without pushing undo state
    applyFormatDirect(Action);
    qDebug() << "[ScriptEditor] Constructor: After applyFormatDirect, isUndoAvailable:" << document()->isUndoAvailable();

    // Emit element changes when cursor moves
    connect(this, &QTextEdit::cursorPositionChanged, this, [this]{
        emit elementChanged(currentElement());
    });
    
    // Log when text changes to track undo availability
    connect(document(), &QTextDocument::contentsChanged, this, [this]{
        qDebug() << "[ScriptEditor] contentsChanged, isUndoAvailable:" << document()->isUndoAvailable() << "isRedoAvailable:" << document()->isRedoAvailable();
    });
    connect(this, &QTextEdit::undoAvailable, [](bool avail){
        qDebug() << "Undo available:" << avail;
    });
}

void ScriptEditor::keyPressEvent(QKeyEvent *e)
{
    qDebug() << "[ScriptEditor] keyPressEvent: key=" << e->key() << "text=" << e->text() << "modifiers=" << e->modifiers();

    if (isNavigationKey(e)) {
        QTextEdit::keyPressEvent(e);
        return;
    }

    if (e->matches(QKeySequence::Undo)) {
        qDebug() << "[ScriptEditor] Undo keystroke intercepted. Stack has" << m_undoStack.count() << "commands, canUndo=" << m_undoStack.canUndo();
        m_undoStack.undo();
        qDebug() << "[ScriptEditor] After undo: Stack has" << m_undoStack.count() << "commands, canUndo=" << m_undoStack.canUndo();
        return;
    }

    if (e->matches(QKeySequence::Redo)) {
        qDebug() << "[ScriptEditor] Redo keystroke intercepted";
        m_undoStack.redo();
        return;
    }

    if (e->matches(QKeySequence::Paste)) {
        QTextCursor cursor = textCursor();
        QString text = QGuiApplication::clipboard()->text();
        if (text.isEmpty()) {
            return;
        }

        QUndoCommand *cmdParent = nullptr;
        int insertPos = cursor.position();
        if (cursor.hasSelection()) {
            cmdParent = new CompoundCommand("paste");
            int selStart = cursor.selectionStart();
            QString selText = normalizeSelectedText(cursor.selectedText());
            new DeleteTextCommand(this, selStart, selText, UndoGroupType::Bulk, false, false, cmdParent);
            insertPos = selStart;
        }

        new InsertTextCommand(this, insertPos, text, UndoGroupType::Bulk, false, cmdParent);
        if (cmdParent) {
            m_undoStack.push(cmdParent);
        } else {
            m_undoStack.push(new InsertTextCommand(this, insertPos, text, UndoGroupType::Bulk, false));
        }
        return;
    }

    if (e->matches(QKeySequence::Cut)) {
        QTextCursor cursor = textCursor();
        if (!cursor.hasSelection()) {
            return;
        }
        QString selText = normalizeSelectedText(cursor.selectedText());
        QGuiApplication::clipboard()->setText(selText);
        int selStart = cursor.selectionStart();
        m_undoStack.push(new DeleteTextCommand(this, selStart, selText, UndoGroupType::Bulk, false, false));
        return;
    }

    if (e->key() == Qt::Key_Tab && !e->modifiers()) {
        QTextCursor c = textCursor();
        QTextBlock block = c.block();
        int state = block.userState();
        ElementType current = (state >= 0 && state < ElementCount) ? static_cast<ElementType>(state) : Action;
        ElementType next = nextType(current);
        applyFormat(next);
        e->accept();
        return;
    }

    if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && !e->modifiers()) {
        QTextCursor c = textCursor();
        QTextBlock block = c.block();
        int state = block.userState();
        ElementType current = (state >= 0 && state < ElementCount) ? static_cast<ElementType>(state) : Action;

        ElementType next;
        if (block.length() <= 1) {
            next = current;
        } else if (current == CharacterName) {
            next = Dialogue;
        } else if (current == Dialogue || current == Parenthetical) {
            next = Action;
        } else {
            next = Action;
        }

        int insertPos = c.position();
        QUndoCommand *cmdParent = new CompoundCommand("newline");
        new InsertTextCommand(this, insertPos, "\n", UndoGroupType::Bulk, false, cmdParent);

        QTextBlockFormat newBlock;
        QTextCharFormat newChar;
        buildFormats(next, newBlock, newChar);
        int newBlockPos = insertPos + 1;
        new FormatCommand(this, newBlockPos, newBlock, newChar, static_cast<int>(next), cmdParent);

        m_undoStack.push(cmdParent);
        return;
    }

    QTextCursor cursor = textCursor();
    bool hasSelection = cursor.hasSelection();

    if (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete) {
        if (hasSelection) {
            int selStart = cursor.selectionStart();
            QString selText = normalizeSelectedText(cursor.selectedText());
            m_undoStack.push(new DeleteTextCommand(this, selStart, selText, UndoGroupType::Bulk, false, false));
            return;
        }

        QTextCursor selCursor = cursor;
        int pos = cursor.position();
        if (e->key() == Qt::Key_Backspace) {
            if (pos == 0) {
                return;
            }
            selCursor.setPosition(pos - 1);
            selCursor.setPosition(pos, QTextCursor::KeepAnchor);
        } else {
            if (pos >= document()->characterCount() - 1) {
                return;
            }
            selCursor.setPosition(pos);
            selCursor.setPosition(pos + 1, QTextCursor::KeepAnchor);
        }

        QString selText = normalizeSelectedText(selCursor.selectedText());
        UndoGroupType type = classifyChar(selText.isEmpty() ? QChar(' ') : selText.at(0));
        bool allowMerge = (type == UndoGroupType::Word);
        bool backspace = (e->key() == Qt::Key_Backspace);
        int delPos = backspace ? pos - 1 : pos;

        m_undoStack.push(new DeleteTextCommand(this, delPos, selText, type, allowMerge, backspace));
        return;
    }

    QString text = e->text();
    if (!text.isEmpty()) {
        ElementType current = currentElement();
        if (text.at(0).isLetter() && (current == CharacterName || current == SceneHeading || current == Shot || current == Transition)) {
            text = text.toUpper();
        }

        int insertPos = cursor.position();
        if (hasSelection || text.size() > 1) {
            QUndoCommand *cmdParent = new CompoundCommand("replace");
            if (hasSelection) {
                int selStart = cursor.selectionStart();
                QString selText = normalizeSelectedText(cursor.selectedText());
                new DeleteTextCommand(this, selStart, selText, UndoGroupType::Bulk, false, false, cmdParent);
                insertPos = selStart;
            }
            new InsertTextCommand(this, insertPos, text, UndoGroupType::Bulk, false, cmdParent);
            m_undoStack.push(cmdParent);
            return;
        }

        UndoGroupType type = classifyChar(text.at(0));
        // Words, Whitespace, and Punctuation can all merge (will be merged into by next Word)
        bool allowMerge = (type == UndoGroupType::Word || type == UndoGroupType::Whitespace);
        qDebug() << "[ScriptEditor] Pushing InsertTextCommand: pos=" << insertPos << "text='" << text << "' type=" << (int)type << "allowMerge=" << allowMerge;
        m_undoStack.push(new InsertTextCommand(this, insertPos, text, type, allowMerge));
        return;
    }

    QTextEdit::keyPressEvent(e);
}

void ScriptEditor::mousePressEvent(QMouseEvent *e)
{
    QTextEdit::mousePressEvent(e);
}

void ScriptEditor::focusOutEvent(QFocusEvent *e)
{
    QTextEdit::focusOutEvent(e);
}

double ScriptEditor::dpiX() const
{
    QScreen *screen = QGuiApplication::primaryScreen();
    return screen ? screen->logicalDotsPerInchX() : 96.0;
}

double ScriptEditor::inchToPx(double inches) const
{
    return inches * dpiX();
}

ScriptEditor::ElementType ScriptEditor::nextType(ElementType t) const
{
    int ni = (static_cast<int>(t) + 1) % static_cast<int>(ElementCount);
    return static_cast<ElementType>(ni);
}

void ScriptEditor::applyFormat(ElementType type)
{
    if (m_suppressUndo) {
        applyFormatDirect(type);
        return;
    }

    QTextCursor c = textCursor();
    QTextBlockFormat bf;
    QTextCharFormat cf;
    buildFormats(type, bf, cf);
    int blockPos = c.block().position();
    m_undoStack.push(new FormatCommand(this, blockPos, bf, cf, static_cast<int>(type)));
}

ScriptEditor::ElementType ScriptEditor::currentElement() const
{
    QTextCursor c = textCursor();
    QTextBlock block = c.block();
    int state = block.userState();
    return (state >= 0 && state < ElementCount) ? static_cast<ElementType>(state) : Action;
}

void ScriptEditor::formatDocument()
{
    qDebug() << "[ScriptEditor::formatDocument] START";
    QTextDocument *doc = document();
    QTextBlock block = doc->begin();
    QTextCursor cursor(doc);

    cursor.beginEditBlock();
    m_suppressUndo = true;
    while (block.isValid()) {
        int state = block.userState();
        if (state >= 0 && state < ElementCount) {
            cursor.setPosition(block.position());
            ElementType type = static_cast<ElementType>(state);
            QTextBlockFormat bf;
            QTextCharFormat cf;
            buildFormats(type, bf, cf);
            cursor.setBlockFormat(bf);
            cursor.setBlockCharFormat(cf);
        }
        block = block.next();
    }
    m_suppressUndo = false;
    cursor.endEditBlock();
    qDebug() << "[ScriptEditor::formatDocument] END";
}

void ScriptEditor::undo()
{
    qDebug() << "[ScriptEditor] Undo called, undoAvailable:" << m_undoStack.canUndo();
    m_undoStack.undo();
}

void ScriptEditor::redo()
{
    qDebug() << "[ScriptEditor] Redo called, redoAvailable:" << m_undoStack.canRedo();
    m_undoStack.redo();
}

ScriptEditor::UndoGroupType ScriptEditor::classifyChar(QChar ch) const
{
    if (ch.isSpace()) {
        return UndoGroupType::Whitespace;
    }
    if (ch.isLetterOrNumber() || ch == QLatin1Char('_')) {
        return UndoGroupType::Word;
    }
    return UndoGroupType::Punctuation;
}

bool ScriptEditor::isNavigationKey(QKeyEvent *e) const
{
    switch (e->key()) {
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Home:
    case Qt::Key_End:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        return true;
    default:
        return false;
    }
}

void ScriptEditor::applyFormatDirect(ElementType type)
{
    QTextCursor c = textCursor();
    QTextBlock block = c.block();
    QString blockText = block.text();
    int cursorPosInBlock = c.positionInBlock();
    
    QTextBlockFormat bf;
    QTextCharFormat cf;
    buildFormats(type, bf, cf);

    c.beginEditBlock();
    c.setBlockFormat(bf);
    c.setBlockCharFormat(cf);
    
    block = c.block();
    block.setUserState(static_cast<int>(type));
    
    // Restore cursor position
    c.setPosition(block.position() + cursorPosInBlock);
    setTextCursor(c);
    c.endEditBlock();

    emit elementChanged(type);
}

void ScriptEditor::buildFormats(ElementType type, QTextBlockFormat &bf, QTextCharFormat &cf) const
{
    bf = QTextBlockFormat();
    cf = QTextCharFormat();

    bf.setLineHeight(100, QTextBlockFormat::ProportionalHeight);

    double leftIn = 0.0;
    double rightIn = 0.0;
    double widthIn = 6.0;
    int spaceBeforePx = 0;

    QFont::Capitalization caps = QFont::MixedCase;
    Qt::Alignment align = Qt::AlignLeft;

    switch (type) {
    case SceneHeading:
        leftIn = 0.0; widthIn = 6.0; caps = QFont::AllUppercase; spaceBeforePx = static_cast<int>(fontMetrics().height());
        break;
    case Action:
        leftIn = 0.0; widthIn = 6.0; caps = QFont::MixedCase; spaceBeforePx = static_cast<int>(fontMetrics().height());
        break;
    case CharacterName:
        leftIn = 2.3; widthIn = 6.0 - 2.3; caps = QFont::AllUppercase; spaceBeforePx = static_cast<int>(fontMetrics().height());
        break;
    case Dialogue:
        leftIn = 1.0; widthIn = 3.5; caps = QFont::MixedCase; spaceBeforePx = 0;
        break;
    case Parenthetical:
        leftIn = 1.5; widthIn = 2.5; caps = QFont::MixedCase; spaceBeforePx = 0;
        break;
    case Shot:
        leftIn = 0.0; widthIn = 6.0; caps = QFont::AllUppercase; spaceBeforePx = static_cast<int>(fontMetrics().height());
        break;
    case Transition:
        leftIn = 0.0; widthIn = 6.0; caps = QFont::AllUppercase; spaceBeforePx = static_cast<int>(fontMetrics().height()); align = Qt::AlignRight;
        break;
    default:
        break;
    }

    rightIn = 6.0 - leftIn - widthIn;
    if (rightIn < 0.0) rightIn = 0.0;

    bf.setLeftMargin(inchToPx(leftIn));
    bf.setRightMargin(inchToPx(rightIn));
    bf.setTopMargin(spaceBeforePx);
    bf.setAlignment(align);

    cf.setFontCapitalization(caps);
}


