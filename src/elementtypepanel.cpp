#include "elementtypepanel.h"
#include <QVBoxLayout>
#include <QLabel>

ElementTypePanel::ElementTypePanel(QWidget *parent)
    : QWidget(parent), m_currentType(ScriptEditor::SceneHeading) {
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);
    
    // Title
    QLabel *titleLabel = new QLabel("Element Type", this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);
    
    // Buttons for each element type
    const QStringList typeNames = {
        "Scene Heading",
        "Action",
        "Character",
        "Dialogue",
        "Parenthetical",
        "Shot",
        "Transition"
    };
    
    for (int i = 0; i < ScriptEditor::ElementCount; ++i) {
        m_buttons[i] = new QPushButton(typeNames[i], this);
        m_buttons[i]->setCheckable(true);
        m_buttons[i]->setMinimumHeight(30);
        m_buttons[i]->setMaximumWidth(120);
        m_buttons[i]->setStyleSheet("text-align: left; padding-left: 8px;");
        
        int type = i;
        connect(m_buttons[i], &QPushButton::clicked, this, [this, type]() {
            emit typeSelected(static_cast<ScriptEditor::ElementType>(type));
        });
        
        layout->addWidget(m_buttons[i]);
    }
    
    layout->addStretch();
    
    // Update highlight for initial type
    updateHighlight(ScriptEditor::SceneHeading);
}

void ElementTypePanel::setCurrentType(ScriptEditor::ElementType type) {
    if (m_currentType != type) {
        m_currentType = type;
        updateHighlight(type);
    }
}

void ElementTypePanel::updateHighlight(ScriptEditor::ElementType type) {
    for (int i = 0; i < ScriptEditor::ElementCount; ++i) {
        bool isCurrent = (i == static_cast<int>(type));
        m_buttons[i]->setChecked(isCurrent);
        
        if (isCurrent) {
            m_buttons[i]->setStyleSheet(
                "QPushButton { "
                "  background-color: #0078d4; "
                "  color: white; "
                "  border: 2px solid #005a9e; "
                "  border-radius: 4px; "
                "  text-align: left; "
                "  padding-left: 8px; "
                "} "
                "QPushButton:hover { background-color: #106ebe; }"
            );
        } else {
            m_buttons[i]->setStyleSheet(
                "QPushButton { "
                "  background-color: #f0f0f0; "
                "  color: black; "
                "  border: 1px solid #ccc; "
                "  border-radius: 4px; "
                "  text-align: left; "
                "  padding-left: 8px; "
                "} "
                "QPushButton:hover { background-color: #e0e0e0; }"
            );
        }
    }
}
