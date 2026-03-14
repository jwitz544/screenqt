#include "elementtypepanel.h"
#include "pageview.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QVBoxLayout>

ElementTypePanel::ElementTypePanel(QWidget *parent)
    : QWidget(parent), m_currentType(ScriptEditor::SceneHeading), m_pageView(nullptr)
{
    setObjectName("elementTypePanel");
    setMinimumWidth(200);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Section header
    auto *header = new QLabel("FORMAT", this);
    header->setObjectName("panelSectionHeader");
    root->addWidget(header);

    // Hairline divider
    auto *line = new QFrame(this);
    line->setObjectName("panelDivider");
    line->setFixedHeight(1);
    root->addWidget(line);

    root->addSpacing(4);

    // Element type items
    static const struct { const char *label; const char *badge; } items[] = {
        { "Scene Heading",  "SH" },
        { "Action",         "A"  },
        { "Character",      "C"  },
        { "Dialogue",       "D"  },
        { "Parenthetical",  "P"  },
        { "Shot",           "SH" },
        { "Transition",     "T"  },
    };

    for (int i = 0; i < ScriptEditor::ElementCount; ++i) {
        auto *row = new QWidget(this);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 10, 0);
        rowLayout->setSpacing(0);

        m_buttons[i] = new QPushButton(items[i].label, row);
        m_buttons[i]->setObjectName("sidebarItem");
        m_buttons[i]->setCheckable(true);
        m_buttons[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_buttons[i]->setMinimumHeight(24);
        m_buttons[i]->setMaximumHeight(24);

        auto *badge = new QLabel(items[i].badge, row);
        badge->setStyleSheet("color: #4D4D4D; font-size: 10px; font-family: monospace;");
        badge->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
        badge->setFixedWidth(20);

        rowLayout->addWidget(m_buttons[i]);
        rowLayout->addWidget(badge);
        root->addWidget(row);

        const int type = i;
        connect(m_buttons[i], &QPushButton::clicked, this, [this, type] {
            emit typeSelected(static_cast<ScriptEditor::ElementType>(type));
        });
    }

    root->addStretch();

    auto *tabHint = new QLabel("Tab \u2192 cycle  \u21e7Tab \u2190 back", this);
    tabHint->setStyleSheet("color: #3E3E3E; font-size: 10px; padding: 6px 12px 10px 20px;");
    root->addWidget(tabHint);

    updateHighlight(ScriptEditor::SceneHeading);
}

void ElementTypePanel::setCurrentType(ScriptEditor::ElementType type)
{
    if (m_currentType != type) {
        m_currentType = type;
        updateHighlight(type);
    }
}

void ElementTypePanel::updateHighlight(ScriptEditor::ElementType type)
{
    for (int i = 0; i < ScriptEditor::ElementCount; ++i) {
        m_buttons[i]->setChecked(i == static_cast<int>(type));
    }
}

void ElementTypePanel::setPageView(PageView *pageView)
{
    m_pageView = pageView;
    if (m_pageView) {
        m_pageView->setDebugMode(false);
    }
}
