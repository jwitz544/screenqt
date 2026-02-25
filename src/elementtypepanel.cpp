#include "elementtypepanel.h"
#include "pageview.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSizePolicy>
#include <QFrame>

namespace {
QString buttonStyle(bool checked, bool isFirst, bool isLast)
{
    const QString radiusTop = isFirst ? "8" : "0";
    const QString radiusBottom = isLast ? "8" : "0";

    if (checked) {
        return QString(
            "QPushButton {"
            "  background-color: #1f6feb;"
            "  color: white;"
            "  border: 1px solid #1f6feb;"
            "  border-top-left-radius: %1px;"
            "  border-top-right-radius: %1px;"
            "  border-bottom-left-radius: %2px;"
            "  border-bottom-right-radius: %2px;"
            "  text-align: left;"
            "  padding: 8px 10px;"
            "  font-weight: 600;"
            "}"
            "QPushButton:hover { background-color: #2f81f7; border-color: #2f81f7; }"
        ).arg(radiusTop, radiusBottom);
    }

    return QString(
        "QPushButton {"
        "  background-color: #ffffff;"
        "  color: #1f2328;"
        "  border: 1px solid #d0d7de;"
        "  border-top-left-radius: %1px;"
        "  border-top-right-radius: %1px;"
        "  border-bottom-left-radius: %2px;"
        "  border-bottom-right-radius: %2px;"
        "  text-align: left;"
        "  padding: 8px 10px;"
        "}"
        "QPushButton:hover { background-color: #f6f8fa; }"
    ).arg(radiusTop, radiusBottom);
}
}

ElementTypePanel::ElementTypePanel(QWidget *parent)
    : QWidget(parent), m_currentType(ScriptEditor::SceneHeading), m_pageView(nullptr) {
    setMinimumHeight(90);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setStyleSheet("background: #f4f6fb;");
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    
    // Title
    QLabel *titleLabel = new QLabel("Element Type", this);
    titleLabel->setStyleSheet("font-weight: 700; font-size: 13px; color: #111827; padding: 0 2px;");
    layout->addWidget(titleLabel);

    QFrame *buttonGroup = new QFrame(this);
    buttonGroup->setStyleSheet(
        "QFrame {"
        "  background: #ffffff;"
        "  border: 1px solid #d0d7de;"
        "  border-radius: 10px;"
        "}"
    );
    QVBoxLayout *buttonLayout = new QVBoxLayout(buttonGroup);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(0);
    
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
        m_buttons[i]->setCheckable(true);
        m_buttons[i]->setMinimumHeight(32);
        m_buttons[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        
        int type = i;
        connect(m_buttons[i], &QPushButton::clicked, this, [this, type]() {
            emit typeSelected(static_cast<ScriptEditor::ElementType>(type));
        });
        
        buttonLayout->addWidget(m_buttons[i]);
    }

    layout->addWidget(buttonGroup);
    
    layout->addStretch();
    
    // Debug checkbox
    m_debugCheckbox = new QCheckBox("Debug Mode", this);
    m_debugCheckbox->setStyleSheet(
        "QCheckBox { color: #374151; font-size: 12px; spacing: 8px; padding-left: 2px; }"
        "QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #9ca3af; border-radius: 4px; background: white; }"
        "QCheckBox::indicator:checked { background: #1f6feb; border-color: #1f6feb; }"
    );
    m_debugCheckbox->setChecked(true);
    layout->addWidget(m_debugCheckbox);
    
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
        const bool isFirst = (i == 0);
        const bool isLast = (i == ScriptEditor::ElementCount - 1);
        m_buttons[i]->setChecked(isCurrent);
        m_buttons[i]->setStyleSheet(buttonStyle(isCurrent, isFirst, isLast));
    }
}

void ElementTypePanel::setPageView(PageView *pageView)
{
    disconnect(m_debugCheckbox, nullptr, this, nullptr);
    m_pageView = pageView;
    if (m_pageView) {
        // Connect checkbox to PageView's debug mode
        connect(m_debugCheckbox, &QCheckBox::stateChanged, this, [this](int state) {
            if (m_pageView) {
                m_pageView->setDebugMode(state == Qt::Checked);
            }
        });
        // Set initial state
        m_pageView->setDebugMode(m_debugCheckbox->isChecked());
    }
}

