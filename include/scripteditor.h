#pragma once
#include <QTextEdit>
#include <QFont>
#include <QScreen>
#include <QGuiApplication>

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

    explicit ScriptEditor(QWidget *parent = nullptr);
    
    void applyFormat(ElementType type);
    void formatDocument(); // Apply formatting to all blocks based on their userState

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    ElementType nextType(ElementType t) const;
    ElementType currentElement() const;
    double dpiX() const;
    double inchToPx(double inches) const;

signals:
    void elementChanged(ElementType type);
};
