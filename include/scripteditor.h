#pragma once
#include <QTextEdit>
#include <QFont>
#include <QScreen>
#include <QGuiApplication>
#include <QUndoStack>

class ScriptEditor : public QTextEdit {
    Q_OBJECT
public:
    enum ElementType {
        SceneHeading = 0,
        Action,
        CharacterName,
        Dialogue,
        Parenthetical,
        Shot,
        Transition,
        ElementCount
    };

    enum class UndoGroupType {
        Word,
        Punctuation,
        Whitespace,
        Bulk,
        Other
    };

    explicit ScriptEditor(QWidget *parent = nullptr);
    
    void applyFormat(ElementType type);
    void formatDocument(); // Apply formatting to all blocks based on their userState


public slots:
    void undo();
    void redo();
    void zoomInText();
    void zoomOutText();
    void resetZoom();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

private:
    ElementType nextType(ElementType t) const;
    ElementType previousType(ElementType t) const;
    ElementType currentElement() const;
    double dpiX() const;
    double inchToPx(double inches) const;

    UndoGroupType classifyChar(QChar ch) const;
    bool isNavigationKey(QKeyEvent *e) const;
    void applyFormatDirect(ElementType type);
    void buildFormats(ElementType type, QTextBlockFormat &bf, QTextCharFormat &cf) const;

    QUndoStack m_undoStack;
    bool m_suppressUndo = false;
    int m_zoomSteps = 0;

signals:
    void elementChanged(ElementType type);
};
