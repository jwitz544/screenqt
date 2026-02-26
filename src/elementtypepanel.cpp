#include "elementtypepanel.h"
#include "pageview.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QFrame>

ElementTypePanel::ElementTypePanel(QWidget *parent)
    : QWidget(parent), m_currentType(ScriptEditor::SceneHeading), m_pageView(nullptr) {
    setObjectName("elementTypePanel");
    setMinimumHeight(90);
    setMinimumWidth(240);
    setMaximumWidth(240);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);
    
    // Title
    QLabel *titleLabel = new QLabel("Element Type", this);
    titleLabel->setObjectName("panelTitle");
    layout->addWidget(titleLabel);

    QFrame *buttonGroup = new QFrame(this);
    buttonGroup->setObjectName("panelGroup");
    QVBoxLayout *buttonLayout = new QVBoxLayout(buttonGroup);
    buttonLayout->setContentsMargins(4, 4, 4, 4);
    buttonLayout->setSpacing(2);
    
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
        m_buttons[i] = new QPushButton(typeNames[i], buttonGroup);
        m_buttons[i]->setObjectName("sidebarItem");
        m_buttons[i]->setCheckable(true);
        m_buttons[i]->setMinimumHeight(26);
        m_buttons[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        
        int type = i;
        connect(m_buttons[i], &QPushButton::clicked, this, [this, type]() {
            emit typeSelected(static_cast<ScriptEditor::ElementType>(type));
        });
        
        buttonLayout->addWidget(m_buttons[i]);
    }

    layout->addWidget(buttonGroup);

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
    }
}

void ElementTypePanel::setPageView(PageView *pageView)
{
    m_pageView = pageView;
    if (m_pageView) {
        m_pageView->setDebugMode(false);
    }
}

