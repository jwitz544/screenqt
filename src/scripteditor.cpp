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
#include <QCompleter>
#include <QStringListModel>
#include <QAbstractItemView>
#include <QApplication>
#include <QSet>
#include "scripteditor_undo.h"

using ScriptEditorUndo::CompoundCommand;
using ScriptEditorUndo::DeleteTextCommand;
using ScriptEditorUndo::FormatCommand;
using ScriptEditorUndo::InsertTextCommand;
using ScriptEditorUndo::normalizeSelectedText;

ScriptEditor::ScriptEditor(QWidget *parent)
    : QTextEdit(parent)
{
    setObjectName("scriptEditor");

    // Basic screenplay font
    QFont f("Courier New");
    f.setPointSizeF(15.0);
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

    m_completionModel = new QStringListModel(this);
    m_completer = new QCompleter(m_completionModel, this);
    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setWrapAround(false);
    m_completer->setMaxVisibleItems(8);
    m_completer->popup()->setObjectName("scriptEditorCompleterPopup");
    m_completer->popup()->setFont(QApplication::font());
    m_completer->popup()->setStyleSheet(
        "QAbstractItemView#scriptEditorCompleterPopup {"
        "  background: #22262D;"
        "  color: #C9D1DD;"
        "  border: 1px solid #303846;"
        "  border-radius: 6px;"
        "  padding: 4px;"
        "  outline: none;"
        "  font-size: 12px;"
        "}"
        "QAbstractItemView#scriptEditorCompleterPopup::item {"
        "  padding: 6px 8px;"
        "  border-radius: 4px;"
        "}"
        "QAbstractItemView#scriptEditorCompleterPopup::item:hover { background: #272C35; }"
        "QAbstractItemView#scriptEditorCompleterPopup::item:selected {"
        "  background: #2A3240;"
        "  color: #C9D1DD;"
        "}"
    );
    connect(m_completer, qOverload<const QString &>(&QCompleter::activated), this, [this](const QString &completion) {
        insertChosenCompletion(completion);
    });

    // Start with Scene Heading element without pushing undo state
    applyFormatDirect(SceneHeading);
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

    if (m_completer && m_completer->popup()->isVisible()) {
        switch (e->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
        case Qt::Key_Up:
        case Qt::Key_Down:
            e->ignore();
            return;
        default:
            break;
        }
    }

    if (isNavigationKey(e)) {
        hideCompletionPopup();
        QTextEdit::keyPressEvent(e);
        return;
    }

    if (e->matches(QKeySequence::Undo)) {
        hideCompletionPopup();
        qDebug() << "[ScriptEditor] Undo keystroke intercepted. Stack has" << m_undoStack.count() << "commands, canUndo=" << m_undoStack.canUndo();
        m_undoStack.undo();
        qDebug() << "[ScriptEditor] After undo: Stack has" << m_undoStack.count() << "commands, canUndo=" << m_undoStack.canUndo();
        return;
    }

    if (e->matches(QKeySequence::Redo)) {
        hideCompletionPopup();
        qDebug() << "[ScriptEditor] Redo keystroke intercepted";
        m_undoStack.redo();
        return;
    }

    if (e->matches(QKeySequence::Paste)) {
        hideCompletionPopup();
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
        hideCompletionPopup();
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

    if (e->key() == Qt::Key_Backtab || (e->key() == Qt::Key_Tab && e->modifiers() == Qt::ShiftModifier)) {
        hideCompletionPopup();
        QTextCursor c = textCursor();
        QTextBlock block = c.block();
        int state = block.userState();
        ElementType current = (state >= 0 && state < ElementCount) ? static_cast<ElementType>(state) : SceneHeading;
        ElementType prev = previousType(current);
        applyFormat(prev);
        e->accept();
        return;
    }

    if (e->key() == Qt::Key_Tab && !e->modifiers()) {
        hideCompletionPopup();
        QTextCursor c = textCursor();
        QTextBlock block = c.block();
        int state = block.userState();
        ElementType current = (state >= 0 && state < ElementCount) ? static_cast<ElementType>(state) : SceneHeading;
        ElementType next = nextType(current);
        applyFormat(next);
        e->accept();
        return;
    }

    if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && !e->modifiers()) {
        hideCompletionPopup();
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
        hideCompletionPopup();
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

        QString insertText = text;
        QString completionSuffix;
        if (current == CharacterName || current == SceneHeading) {
            const QString prefix = cursor.block().text().left(cursor.positionInBlock()) + text;
            completionSuffix = resolveInlineCompletion(current, prefix);
            if (!completionSuffix.isEmpty()) {
                insertText += completionSuffix;
            }
        }

        UndoGroupType type = classifyChar(insertText.at(0));
        // Words, whitespace, and punctuation can merge for natural undo groupings.
        bool allowMerge = completionSuffix.isEmpty() && (type == UndoGroupType::Word ||
                   type == UndoGroupType::Whitespace ||
                   type == UndoGroupType::Punctuation);
        qDebug() << "[ScriptEditor] Pushing InsertTextCommand: pos=" << insertPos << "text='" << insertText << "' type=" << (int)type << "allowMerge=" << allowMerge;
        m_undoStack.push(new InsertTextCommand(this, insertPos, insertText, type, allowMerge));

        if (!completionSuffix.isEmpty()) {
            QTextCursor completionCursor = textCursor();
            completionCursor.setPosition(insertPos + text.size());
            completionCursor.setPosition(insertPos + insertText.size(), QTextCursor::KeepAnchor);
            setTextCursor(completionCursor);
        }

        if (current == CharacterName || current == SceneHeading) {
            const QTextCursor updatedCursor = textCursor();
            const QString completionPrefix = updatedCursor.block().text().left(updatedCursor.positionInBlock()).toUpper();
            showCompletionPopup(current, completionPrefix);
        } else {
            hideCompletionPopup();
        }
        return;
    }

    hideCompletionPopup();

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

QStringList ScriptEditor::collectCharacterNames() const
{
    QStringList names;
    QSet<QString> seen;

    for (QTextBlock block = document()->begin(); block.isValid(); block = block.next()) {
        if (block.userState() != static_cast<int>(CharacterName)) {
            continue;
        }

        const QString normalized = block.text().trimmed().toUpper();
        if (normalized.isEmpty() || seen.contains(normalized)) {
            continue;
        }

        seen.insert(normalized);
        names.append(normalized);
    }

    return names;
}

QStringList ScriptEditor::sceneHeadingCompletions() const
{
    return {
        "INT. ",
        "EXT. ",
        "INT./EXT. ",
        "EST. ",
        "I/E. "
    };
}

QStringList ScriptEditor::completionCandidates(ElementType type, const QString &prefix) const
{
    const QString normalizedPrefix = prefix.toUpper();
    if (normalizedPrefix.trimmed().isEmpty()) {
        return {};
    }

    QStringList pool;
    if (type == CharacterName) {
        pool = collectCharacterNames();
    } else if (type == SceneHeading) {
        pool = sceneHeadingCompletions();
    } else {
        return {};
    }

    QStringList matches;
    for (const QString &candidate : pool) {
        if (candidate.startsWith(normalizedPrefix) && candidate != normalizedPrefix) {
            matches.append(candidate);
        }
    }
    return matches;
}

void ScriptEditor::showCompletionPopup(ElementType type, const QString &prefix)
{
    if (!m_completer || !m_completionModel) {
        return;
    }

    const QStringList matches = completionCandidates(type, prefix);
    if (matches.size() <= 1) {
        hideCompletionPopup();
        return;
    }

    m_completionType = type;
    m_completionPrefix = prefix.toUpper();
    m_completionModel->setStringList(matches);

    QRect popupRect = cursorRect();
    popupRect.setWidth(220);
    m_completer->complete(popupRect);
}

void ScriptEditor::hideCompletionPopup()
{
    if (m_completer) {
        m_completer->popup()->hide();
    }
    m_completionPrefix.clear();
}

void ScriptEditor::insertChosenCompletion(const QString &completion)
{
    if (m_completionPrefix.isEmpty() || !completion.startsWith(m_completionPrefix)) {
        return;
    }

    const QString suffix = completion.mid(m_completionPrefix.size());
    if (suffix.isEmpty()) {
        hideCompletionPopup();
        return;
    }

    const int insertPos = textCursor().position();
    m_undoStack.push(new InsertTextCommand(this, insertPos, suffix, UndoGroupType::Bulk, false));
    hideCompletionPopup();
}

QString ScriptEditor::resolveInlineCompletion(ElementType type, const QString &prefix) const
{
    const QStringList matches = completionCandidates(type, prefix);
    if (matches.size() != 1) {
        return QString();
    }

    const QString normalizedPrefix = prefix.toUpper();
    return matches.first().mid(normalizedPrefix.size());
}

ScriptEditor::ElementType ScriptEditor::nextType(ElementType t) const
{
    int ni = (static_cast<int>(t) + 1) % static_cast<int>(ElementCount);
    return static_cast<ElementType>(ni);
}

ScriptEditor::ElementType ScriptEditor::previousType(ElementType t) const
{
    int i = static_cast<int>(t);
    int pi = (i - 1 + static_cast<int>(ElementCount)) % static_cast<int>(ElementCount);
    return static_cast<ElementType>(pi);
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

void ScriptEditor::clear()
{
    QTextEdit::clear();
    m_undoStack.clear();
    applyFormatDirect(SceneHeading);
}

void ScriptEditor::redo()
{
    qDebug() << "[ScriptEditor] Redo called, redoAvailable:" << m_undoStack.canRedo();
    m_undoStack.redo();
}

void ScriptEditor::zoomInText()
{
    if (m_zoomSteps >= 20) {
        return;
    }
    zoomIn(1);
    m_zoomSteps++;
}

void ScriptEditor::zoomOutText()
{
    if (m_zoomSteps <= -8) {
        return;
    }
    zoomOut(1);
    m_zoomSteps--;
}

void ScriptEditor::resetZoom()
{
    if (m_zoomSteps > 0) {
        zoomOut(m_zoomSteps);
    } else if (m_zoomSteps < 0) {
        zoomIn(-m_zoomSteps);
    }
    m_zoomSteps = 0;
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


