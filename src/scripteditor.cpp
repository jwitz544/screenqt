#include "scripteditor.h"
#include <QTextCursor>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QKeyEvent>
#include <QScrollBar>

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

    // Start with Action element
    applyFormat(Action);

    // Emit element changes when cursor moves
    connect(this, &QTextEdit::cursorPositionChanged, this, [this]{
        emit elementChanged(currentElement());
    });
}

void ScriptEditor::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Tab && !e->modifiers()) {
        QTextCursor c = textCursor();
        QTextBlock block = c.block();
        int state = block.userState();
        ElementType current = (state >= 0 && state < ElementCount) ? static_cast<ElementType>(state) : Action;
        ElementType next = nextType(current);
        applyFormat(next);
        return; // swallow tab
    }
    if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && !e->modifiers()) {
        // Determine sensible default for the next line based on current element
        QTextCursor c = textCursor();
        QTextBlock block = c.block();
        int state = block.userState();
        ElementType current = (state >= 0 && state < ElementCount) ? static_cast<ElementType>(state) : Action;

        // If current block is empty, keep the same element (allows inserting blank lines)
        ElementType next;
        if (block.length() <= 1) { // Qt counts newline; empty text length is 1
            next = current;
        } else if (current == CharacterName) {
            next = Dialogue;      // After character, start dialogue
        } else if (current == Dialogue || current == Parenthetical) {
            next = Action;        // After dialogue, return to action by default
        } else {
            next = Action;        // Default for other elements
        }

        // Insert newline using base behavior, then apply format to new block
        QTextEdit::keyPressEvent(e);
        applyFormat(next);
        return;
    }
    QTextEdit::keyPressEvent(e);
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
    QTextCursor c = textCursor();
    QTextBlockFormat bf;
    QTextCharFormat cf;

    // Base: single spacing within element
    bf.setLineHeight(100, QTextBlockFormat::ProportionalHeight);

    // Determine margins based on element (inches)
    double leftIn = 0.0;
    double rightIn = 0.0;
    double widthIn = 6.0; // default full text width
    int spaceBeforePx = 0; // blank lines before

    // Capitalization
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

    // Compute right margin to achieve desired width within 6"
    rightIn = 6.0 - leftIn - widthIn;
    if (rightIn < 0.0) rightIn = 0.0;

    bf.setLeftMargin(inchToPx(leftIn));
    bf.setRightMargin(inchToPx(rightIn));
    bf.setTopMargin(spaceBeforePx);
    bf.setAlignment(align);

    // Apply formats to current block
    c.beginEditBlock();
    c.setBlockFormat(bf);
    cf.setFontCapitalization(caps);
    c.setBlockCharFormat(cf);

    // Track element type in block's user state
    QTextBlock block = c.block();
    block.setUserState(static_cast<int>(type));
    c.endEditBlock();

    emit elementChanged(type);
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
    QTextDocument *doc = document();
    QTextBlock block = doc->begin();
    QTextCursor cursor(doc);
    
    cursor.beginEditBlock();
    while (block.isValid()) {
        int state = block.userState();
        if (state >= 0 && state < ElementCount) {
            cursor.setPosition(block.position());
            ElementType type = static_cast<ElementType>(state);
            
            // Apply formatting based on element type
            QTextBlockFormat bf;
            QTextCharFormat cf;
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
            
            cursor.setBlockFormat(bf);
            cf.setFontCapitalization(caps);
            cursor.setBlockCharFormat(cf);
        }
        block = block.next();
    }
    cursor.endEditBlock();
}
