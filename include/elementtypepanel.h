#pragma once
#include <QWidget>
#include <QPushButton>
#include <QCheckBox>
#include "scripteditor.h"

class ElementTypePanel : public QWidget {
    Q_OBJECT
public:
    explicit ElementTypePanel(QWidget *parent = nullptr);
    void setCurrentType(ScriptEditor::ElementType type);
    void setPageView(class PageView *pageView);

signals:
    void typeSelected(ScriptEditor::ElementType type);

private:
    void updateHighlight(ScriptEditor::ElementType type);
    QPushButton *m_buttons[ScriptEditor::ElementCount];
    ScriptEditor::ElementType m_currentType;
    QCheckBox *m_debugCheckbox;
    class PageView *m_pageView;
};
