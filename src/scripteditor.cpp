#include "scripteditor.h"
#include "spellcheckservice.h"
#include <QTextCursor>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextDocument>
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
#include <QFontDatabase>
#include <QFontInfo>
#include <QContextMenuEvent>
#include <QMenu>
#include <QTimer>
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

    // Screenplay font baseline: 12 pt, preferring production screenplay faces.
    QString chosenFamily = "Courier New";
    const QStringList preferredFamilies = {
        "Courier Final Draft",
        "Courier Prime",
        "Courier Screenplay",
        "Courier New"
    };
    const QStringList availableFamilies = QFontDatabase().families();
    for (const QString &candidate : preferredFamilies) {
        if (availableFamilies.contains(candidate)) {
            chosenFamily = candidate;
            break;
        }
    }

    QFont f(chosenFamily);
    f.setPointSizeF(12.0);
    f.setFixedPitch(true);
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
        "  padding: 2px;"
        "  outline: none;"
        "  font-size: 11px;"
        "}"
        "QAbstractItemView#scriptEditorCompleterPopup::item {"
        "  padding: 5px 8px;"
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

    m_spellChecker = std::make_unique<BasicSpellChecker>();
    m_spellcheckTimer = new QTimer(this);
    m_spellcheckTimer->setSingleShot(true);
    m_spellcheckTimer->setInterval(250);
    connect(m_spellcheckTimer, &QTimer::timeout, this, &ScriptEditor::refreshSpellcheck);

    connect(document(), &QTextDocument::contentsChanged, this, [this] {
        rebuildFindMatches();
        scheduleSpellcheckRefresh();
    });

    scheduleSpellcheckRefresh();
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

void ScriptEditor::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();

    QTextCursor tokenCursor;
    const QString token = wordUnderCursor(&tokenCursor);
    if (m_spellcheckEnabled && m_spellChecker && !token.isEmpty()) {
        bool isMisspelled = false;
        for (const Range &range : m_spellingRanges) {
            if (range.start == tokenCursor.selectionStart() && range.length == tokenCursor.selectionEnd() - tokenCursor.selectionStart()) {
                isMisspelled = true;
                break;
            }
        }

        if (isMisspelled) {
            menu->addSeparator();
            const QStringList suggestions = m_spellChecker->suggestionsFor(token);
            if (suggestions.isEmpty()) {
                QAction *noSuggestionAction = menu->addAction("No suggestions");
                noSuggestionAction->setEnabled(false);
            } else {
                for (const QString &suggestion : suggestions) {
                    QAction *replaceAction = menu->addAction(suggestion);
                    connect(replaceAction, &QAction::triggered, this, [this, tokenCursor, suggestion] {
                        replaceRangeText(tokenCursor.selectionStart(), tokenCursor.selectionEnd() - tokenCursor.selectionStart(), suggestion);
                    });
                }
            }

            QAction *addToDictionaryAction = menu->addAction("Add to Dictionary");
            connect(addToDictionaryAction, &QAction::triggered, this, [this, token] {
                if (m_spellChecker) {
                    m_spellChecker->addWord(token);
                    scheduleSpellcheckRefresh();
                }
            });
        }
    }

    menu->exec(event->globalPos());
    delete menu;
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
    rebuildFindMatches();
    scheduleSpellcheckRefresh();
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

void ScriptEditor::setFindQuery(const QString &query)
{
    const QString normalized = query;
    if (m_findQuery == normalized) {
        return;
    }
    m_findQuery = normalized;
    rebuildFindMatches();
}

void ScriptEditor::setFindOptions(bool caseSensitive, bool wholeWord)
{
    if (m_findCaseSensitive == caseSensitive && m_findWholeWord == wholeWord) {
        return;
    }
    m_findCaseSensitive = caseSensitive;
    m_findWholeWord = wholeWord;
    rebuildFindMatches();
}

bool ScriptEditor::findNext()
{
    if (m_findMatches.isEmpty()) {
        return false;
    }

    const int currentPos = textCursor().selectionEnd();
    int nextIndex = 0;
    for (int i = 0; i < m_findMatches.size(); ++i) {
        if (m_findMatches[i].start >= currentPos) {
            nextIndex = i;
            break;
        }
    }

    if (m_activeFindIndex >= 0 && m_activeFindIndex + 1 < m_findMatches.size()) {
        if (m_findMatches[m_activeFindIndex].start < currentPos) {
            nextIndex = m_activeFindIndex + 1;
        }
    }

    applyFindMatchAtIndex(nextIndex % m_findMatches.size());
    return true;
}

bool ScriptEditor::findPrevious()
{
    if (m_findMatches.isEmpty()) {
        return false;
    }

    const int currentPos = textCursor().selectionStart();
    int prevIndex = m_findMatches.size() - 1;
    for (int i = m_findMatches.size() - 1; i >= 0; --i) {
        if (m_findMatches[i].start < currentPos) {
            prevIndex = i;
            break;
        }
    }

    applyFindMatchAtIndex(prevIndex);
    return true;
}

int ScriptEditor::findMatchCount() const
{
    return m_findMatches.size();
}

int ScriptEditor::activeFindMatchIndex() const
{
    return m_activeFindIndex;
}

void ScriptEditor::setSpellcheckEnabled(bool enabled)
{
    if (m_spellcheckEnabled == enabled) {
        return;
    }

    m_spellcheckEnabled = enabled;
    if (!m_spellcheckEnabled) {
        m_spellingRanges.clear();
        refreshExtraSelections();
        return;
    }

    scheduleSpellcheckRefresh();
}

bool ScriptEditor::spellcheckEnabled() const
{
    return m_spellcheckEnabled;
}

int ScriptEditor::spellcheckMisspellingCount() const
{
    return m_spellingRanges.size();
}

QStringList ScriptEditor::spellcheckSuggestions(const QString &word) const
{
    if (!m_spellChecker) {
        return {};
    }
    return m_spellChecker->suggestionsFor(word);
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

QTextDocument::FindFlags ScriptEditor::currentFindFlags() const
{
    QTextDocument::FindFlags flags;
    if (m_findCaseSensitive) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    if (m_findWholeWord) {
        flags |= QTextDocument::FindWholeWords;
    }
    return flags;
}

void ScriptEditor::rebuildFindMatches()
{
    m_findMatches.clear();
    m_activeFindIndex = -1;

    const QString needle = m_findQuery.trimmed();
    if (needle.isEmpty()) {
        refreshExtraSelections();
        emit findResultsChanged(-1, 0);
        return;
    }

    QTextCursor cursor(document());
    const QTextDocument::FindFlags flags = currentFindFlags();

    while (true) {
        cursor = document()->find(needle, cursor, flags);
        if (cursor.isNull()) {
            break;
        }

        Range range;
        range.start = cursor.selectionStart();
        range.length = cursor.selectionEnd() - cursor.selectionStart();
        if (range.length > 0) {
            m_findMatches.append(range);
        }
    }

    if (!m_findMatches.isEmpty()) {
        applyFindMatchAtIndex(0);
    } else {
        refreshExtraSelections();
        emit findResultsChanged(-1, 0);
    }
}

void ScriptEditor::applyFindMatchAtIndex(int index)
{
    if (m_findMatches.isEmpty()) {
        m_activeFindIndex = -1;
        refreshExtraSelections();
        emit findResultsChanged(-1, 0);
        return;
    }

    m_activeFindIndex = qBound(0, index, m_findMatches.size() - 1);
    const Range &range = m_findMatches[m_activeFindIndex];

    QTextCursor cursor(document());
    cursor.setPosition(range.start);
    cursor.setPosition(range.start + range.length, QTextCursor::KeepAnchor);
    setTextCursor(cursor);
    ensureCursorVisible();

    refreshExtraSelections();
    emit findResultsChanged(m_activeFindIndex, m_findMatches.size());
}

void ScriptEditor::refreshSpellcheck()
{
    if (!m_spellcheckEnabled || !m_spellChecker || !m_spellChecker->isAvailable()) {
        m_spellingRanges.clear();
        refreshExtraSelections();
        return;
    }

    m_spellingRanges.clear();
    const QList<Misspelling> misspellings = m_spellChecker->checkText(toPlainText());
    m_spellingRanges.reserve(misspellings.size());
    for (const Misspelling &item : misspellings) {
        if (item.length <= 0) {
            continue;
        }
        Range range;
        range.start = item.start;
        range.length = item.length;
        m_spellingRanges.append(range);
    }

    refreshExtraSelections();
}

void ScriptEditor::scheduleSpellcheckRefresh()
{
    if (!m_spellcheckTimer) {
        return;
    }
    m_spellcheckTimer->start();
}

QString ScriptEditor::wordUnderCursor(QTextCursor *wordCursor) const
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    if (wordCursor) {
        *wordCursor = cursor;
    }
    return cursor.selectedText();
}

void ScriptEditor::replaceRangeText(int start, int length, const QString &replacement)
{
    QTextCursor cursor(document());
    cursor.setPosition(start);
    cursor.setPosition(start + length, QTextCursor::KeepAnchor);
    setTextCursor(cursor);

    QUndoCommand *cmdParent = new CompoundCommand("replace");
    new DeleteTextCommand(this, start, normalizeSelectedText(cursor.selectedText()), UndoGroupType::Bulk, false, false, cmdParent);
    new InsertTextCommand(this, start, replacement, UndoGroupType::Bulk, false, cmdParent);
    m_undoStack.push(cmdParent);

    scheduleSpellcheckRefresh();
}

void ScriptEditor::refreshExtraSelections()
{
    QList<QTextEdit::ExtraSelection> selections;

    QTextCharFormat misspelledFormat;
    misspelledFormat.setUnderlineColor(QColor("#E06C75"));
    misspelledFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

    for (const Range &range : m_spellingRanges) {
        QTextEdit::ExtraSelection sel;
        QTextCursor cursor(document());
        cursor.setPosition(range.start);
        cursor.setPosition(range.start + range.length, QTextCursor::KeepAnchor);
        sel.cursor = cursor;
        sel.format = misspelledFormat;
        selections.append(sel);
    }

    QTextCharFormat matchFormat;
    matchFormat.setBackground(QColor("#7B93C466"));
    matchFormat.setForeground(QColor("#101319"));

    QTextCharFormat activeMatchFormat;
    activeMatchFormat.setBackground(QColor("#98B7F0"));
    activeMatchFormat.setForeground(QColor("#101319"));

    for (int i = 0; i < m_findMatches.size(); ++i) {
        const Range &range = m_findMatches[i];
        QTextEdit::ExtraSelection sel;
        QTextCursor cursor(document());
        cursor.setPosition(range.start);
        cursor.setPosition(range.start + range.length, QTextCursor::KeepAnchor);
        sel.cursor = cursor;
        sel.format = (i == m_activeFindIndex) ? activeMatchFormat : matchFormat;
        selections.append(sel);
    }

    setExtraSelections(selections);
}


