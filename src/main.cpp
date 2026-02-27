#include <QApplication>
#include <QMainWindow>
#include <QScreen>
#include <QScrollArea>
#include <QStatusBar>
#include <QLabel>
#include <QStackedWidget>
#include <QMenuBar>
#include <QMenu>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDateTime>
#include <QDockWidget>
#include <QList>
#include <QSettings>
#include <QFont>
#include <QFontInfo>
#include <QTabBar>
#include "pageview.h"
#include "scripteditor.h"
#include "findbar.h"
#include "startscreen.h"
#include "elementtypepanel.h"
#include "outlinepanel.h"
#include "characterspanel.h"

namespace UiSpacing {
constexpr int GridUnit = 8;
constexpr int PanelPadding = 12;
constexpr int PanelPaddingLarge = 16;
constexpr int ItemVerticalSpacing = 8;
constexpr int SidebarWidth = 320;
constexpr int SidebarMinWidth = 220;
constexpr int SidebarMaxWidth = 560;
constexpr int CompactItemPaddingV = 5;
constexpr int CompactItemPaddingH = 8;
}

namespace UiColors {
constexpr auto MainBackground = "#1B1D21";
constexpr auto SidebarBackground = "#15171B";
constexpr auto SurfaceBackground = "#1F232B";
constexpr auto HoverBackground = "#272C35";
constexpr auto ActiveBackground = "#2A3240";
constexpr auto Accent = "#7396D8";
constexpr auto TextPrimary = "#C9D1DD";
constexpr auto TextMuted = "#93A0B6";
constexpr auto MenuBackground = "#1D2026";
constexpr auto MenuPopupBackground = "#22262D";
constexpr auto ScrollThumb = "#475061";
}

QString buildAppStyleSheet()
{
    return QStringLiteral(
        "QMainWindow { background: %1; color: %2; }"
        "QWidget { color: %2; }"
        "QMainWindow::separator { background: #1F232B; width: 2px; height: 2px; }"
        "QMenuBar {"
        "  background: %3;"
        "  padding: 0px 6px;"
        "  min-height: 20px;"
        "}"
        "QMenuBar::item {"
        "  spacing: 8px;"
        "  padding: %12px %13px;"
        "  color: %2;"
        "  border-radius: 4px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QMenuBar::item:selected { background: %4; }"
        "QMenu {"
        "  background: %5;"
        "  padding: 2px;"
        "}"
        "QMenu::item {"
        "  padding: %12px %13px;"
        "  border-radius: 4px;"
        "  font-size: 12px;"
        "}"
        "QMenu::item:selected { background: %4; }"
        "QMainWindow QTabBar::tab {"
        "  background: %5;"
        "  color: %7;"
        "  padding: 2px 6px;"
        "  margin-right: 2px;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-size: 10px;"
        "  font-weight: 500;"
        "}"
        "QMainWindow QTabBar::tab:selected {"
        "  background: %8;"
        "  color: %2;"
        "  font-weight: 600;"
        "}"
        "QMainWindow QTabBar::tab:hover { background: %4; color: %2; }"
        "QDockWidget {"
        "  background: %6;"
        "  font-size: 12px;"
        "  titlebar-close-icon: none;"
        "  titlebar-normal-icon: none;"
        "}"
        "QDockWidget::title {"
        "  background: %6;"
        "  color: %7;"
        "  padding: 3px 8px 2px 8px;"
        "  text-align: left;"
        "  border: none;"
        "  font-size: 10px;"
        "  font-weight: 600;"
        "}"
        "QDockWidget > QWidget { background: %6; }"
        "QWidget#elementTypePanel, QWidget#outlinePanel, QWidget#charactersPanel { background: %6; }"
        "QLabel#panelTitle {"
        "  color: %2;"
        "  font-size: 11px;"
        "  font-weight: 600;"
        "}"
        "QLabel#panelMeta {"
        "  color: %7;"
        "  font-size: 9px;"
        "  font-weight: 500;"
        "}"
        "QFrame#panelGroup {"
        "  background: %8;"
        "  border-radius: 8px;"
        "}"
        "QPushButton#sidebarItem {"
        "  background: transparent;"
        "  color: %2;"
        "  text-align: left;"
        "  padding: %12px %13px;"
        "  border: none;"
        "  border-left: 2px solid transparent;"
        "  font-size: 11px;"
        "  font-weight: 500;"
        "}"
        "QPushButton#sidebarItem:hover { background: %4; }"
        "QPushButton#sidebarItem:checked {"
        "  background: %9;"
        "  border-left: 2px solid %10;"
        "  font-weight: 600;"
        "}"
        "QListWidget#sceneList {"
        "  background: transparent;"
        "  border: none;"
        "  outline: none;"
        "  padding: 1px;"
        "}"
        "QListWidget#sceneList::item {"
        "  padding: %12px %13px;"
        "  border-left: 2px solid transparent;"
        "  border-radius: 6px;"
        "  color: %2;"
        "  font-size: 11px;"
        "  font-weight: 500;"
        "}"
        "QListWidget#sceneList::item:hover { background: %4; }"
        "QListWidget#sceneList::item:selected {"
        "  background: %9;"
        "  border-left: 2px solid %10;"
        "  font-weight: 600;"
        "}"
        "QScrollArea#editorScrollArea { background: %1; border: none; }"
        "QScrollArea#editorScrollArea > QWidget > QWidget { background: transparent; }"
        "QScrollBar:vertical { background: transparent; width: 8px; margin: 1px; }"
        "QScrollBar::handle:vertical { background: %11; border-radius: 5px; min-height: 24px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QAbstractItemView#scriptEditorCompleterPopup {"
        "  background: %5;"
        "  color: %2;"
        "  border: 1px solid #303846;"
        "  border-radius: 6px;"
        "  padding: 2px;"
        "  outline: none;"
        "  font-size: 11px;"
        "}"
        "QAbstractItemView#scriptEditorCompleterPopup::item {"
        "  padding: %12px %13px;"
        "  border-radius: 4px;"
        "}"
        "QAbstractItemView#scriptEditorCompleterPopup::item:hover { background: %4; }"
        "QAbstractItemView#scriptEditorCompleterPopup::item:selected {"
        "  background: %9;"
        "  color: %2;"
        "}"
        "QTextEdit#scriptEditor {"
        "  color: #1E2127;"
        "  background: transparent;"
        "  border: none;"
        "}"
        "QTextEdit { selection-background-color: #7B93C4; selection-color: #101319; }"
        "QFrame#findBar {"
        "  background: %5;"
        "  border-bottom: 1px solid #303846;"
        "}"
        "QFrame#findBar QLabel { color: %7; font-size: 11px; font-weight: 600; }"
        "QFrame#findBar QLineEdit {"
        "  background: %8;"
        "  border: 1px solid #303846;"
        "  border-radius: 4px;"
        "  padding: 4px 6px;"
        "  color: %2;"
        "}"
        "QFrame#findBar QPushButton, QFrame#findBar QCheckBox {"
        "  background: %8;"
        "  color: %2;"
        "  border: 1px solid #303846;"
        "  border-radius: 4px;"
        "  padding: 3px 6px;"
        "  font-size: 10px;"
        "}"
        "QFrame#findBar QPushButton:hover, QFrame#findBar QCheckBox:hover { background: %4; }"
        "QWidget#startScreen { background: %1; }"
        "QLabel#startTitle { color: %2; font-size: 32px; font-weight: 700; }"
        "QPushButton#startPrimaryButton, QPushButton#startSecondaryButton {"
        "  padding: 9px 14px;"
        "  border-radius: 6px;"
        "  background: %8;"
        "  color: %2;"
        "  font-size: 14px;"
        "  font-weight: 500;"
        "}"
        "QPushButton#startPrimaryButton:hover, QPushButton#startSecondaryButton:hover { background: %4; }"
        "QPushButton:disabled { color: %7; }"
    )
        .arg(UiColors::MainBackground)
        .arg(UiColors::TextPrimary)
        .arg(UiColors::MenuBackground)
        .arg(UiColors::HoverBackground)
        .arg(UiColors::MenuPopupBackground)
        .arg(UiColors::SidebarBackground)
        .arg(UiColors::TextMuted)
        .arg(UiColors::SurfaceBackground)
        .arg(UiColors::ActiveBackground)
        .arg(UiColors::Accent)
        .arg(UiColors::ScrollThumb)
        .arg(UiSpacing::CompactItemPaddingV)
        .arg(UiSpacing::CompactItemPaddingH);
}

// Custom message handler to add timestamps
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString formattedMsg = QString("[%1] %2").arg(timestamp, msg);
    
    // Call the default handler with the formatted message
    QByteArray localMsg = formattedMsg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "%s\n", localMsg.constData());
        break;
    case QtInfoMsg:
        fprintf(stderr, "%s\n", localMsg.constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s\n", localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", localMsg.constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", localMsg.constData());
        abort();
    }
}

int main(int argc, char *argv[])
{
    // Install custom message handler to add timestamps
    qInstallMessageHandler(customMessageHandler);
    QApplication app(argc, argv);
    qDebug() << "[Main] Application started";

    QFont appFont("Segoe UI");
    if (!QFontInfo(appFont).family().contains("Segoe UI", Qt::CaseInsensitive)) {
        appFont = QApplication::font();
    }
    appFont.setPointSize(9);
    appFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(appFont);

    QMainWindow window;
    window.setWindowTitle("ScreenQt");
    window.resize(900, 700);
    window.setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks | QMainWindow::GroupedDragging);
    window.setStyleSheet(buildAppStyleSheet());
    qDebug() << "[Main] Window created and resized to 900x700";

    QSettings settings("ScreenQt", "ScreenQt");
    constexpr int layoutStateVersion = 1;
    constexpr int defaultScreenplayZoomSteps = 2;
    constexpr int zoomCalibrationRevision = 1;

    int persistedZoomSteps = settings.value("editor/zoomSteps", 0).toInt();
    const int storedZoomCalibrationRevision = settings.value("editor/zoomCalibrationRevision", 0).toInt();
    if (storedZoomCalibrationRevision < zoomCalibrationRevision && persistedZoomSteps == 0) {
        persistedZoomSteps = defaultScreenplayZoomSteps;
        settings.setValue("editor/zoomSteps", persistedZoomSteps);
    }
    settings.setValue("editor/zoomCalibrationRevision", zoomCalibrationRevision);

    // Create stacked widget to switch between start screen and editor
    QStackedWidget *stack = new QStackedWidget(&window);
    window.setCentralWidget(stack);
    
    // Pointer to current page (for save functionality)
    PageView *currentPage = nullptr;
    FindBar *currentFindBar = nullptr;
    
    // Create menu bar
    QMenuBar *menuBar = window.menuBar();
    QMenu *fileMenu = menuBar->addMenu("&File");
    
    QAction *saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setEnabled(false); // Disabled until document is created
    
    QAction *saveAsAction = fileMenu->addAction("Save &As...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    saveAsAction->setEnabled(false);

    QAction *openAction = fileMenu->addAction("&Open...");
    openAction->setShortcut(QKeySequence::Open);
    
    fileMenu->addSeparator();

    QMenu *importExportMenu = fileMenu->addMenu("&Import/Export");
    QAction *importFdxAction = importExportMenu->addAction("&Import Final Draft (FDX)...");
    QAction *exportFdxAction = importExportMenu->addAction("Export to &Final Draft (FDX)...");
    exportFdxAction->setEnabled(false);
    
    QAction *exportPdfAction = importExportMenu->addAction("Export to &PDF...");
    exportPdfAction->setEnabled(false);
    
    // Edit menu with undo/redo
    QMenu *editMenu = menuBar->addMenu("&Edit");
    
    QAction *undoAction = editMenu->addAction("&Undo");
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setEnabled(false);
    
    QAction *redoAction = editMenu->addAction("&Redo");
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setEnabled(false);

    editMenu->addSeparator();
    QAction *findAction = editMenu->addAction("&Find...");
    findAction->setShortcut(QKeySequence::Find);
    findAction->setEnabled(false);

    QAction *findNextAction = editMenu->addAction("Find &Next");
    findNextAction->setShortcut(QKeySequence::FindNext);
    findNextAction->setEnabled(false);

    QAction *findPreviousAction = editMenu->addAction("Find &Previous");
    findPreviousAction->setShortcut(QKeySequence::FindPrevious);
    findPreviousAction->setEnabled(false);

    editMenu->addSeparator();
    QAction *spellcheckAction = editMenu->addAction("&Spellcheck");
    spellcheckAction->setCheckable(true);
    spellcheckAction->setChecked(true);
    spellcheckAction->setEnabled(false);

    QMenu *viewMenu = menuBar->addMenu("&View");
    QAction *zoomInAction = viewMenu->addAction("Zoom &In");
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    zoomInAction->setEnabled(false);

    QAction *zoomOutAction = viewMenu->addAction("Zoom &Out");
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    zoomOutAction->setEnabled(false);

    QAction *resetZoomAction = viewMenu->addAction("Reset &Zoom");
    resetZoomAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    resetZoomAction->setEnabled(false);

    QAction *resetLayoutAction = viewMenu->addAction("Reset &Layout");
    
    // Create start screen
    StartScreen *startScreen = new StartScreen();
    stack->addWidget(startScreen);

    // Right-side draggable panel blocks
    ElementTypePanel *typePanel = new ElementTypePanel();
    OutlinePanel *outlinePanel = new OutlinePanel();
    CharactersPanel *charactersPanel = new CharactersPanel();

    QDockWidget *elementDock = new QDockWidget("Elements", &window);
    elementDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    elementDock->setWidget(typePanel);

    QDockWidget *outlineDock = new QDockWidget("Outline", &window);
    outlineDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    outlineDock->setWidget(outlinePanel);

    QDockWidget *charactersDock = new QDockWidget("Characters", &window);
    charactersDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    charactersDock->setWidget(charactersPanel);

    auto normalizeDockTabBars = [&window]() {
        const auto tabBars = window.findChildren<QTabBar*>();
        for (QTabBar *tabBar : tabBars) {
            tabBar->setElideMode(Qt::ElideNone);
            tabBar->setUsesScrollButtons(false);
            tabBar->setExpanding(true);
        }
    };

    window.addDockWidget(Qt::RightDockWidgetArea, elementDock);
    window.addDockWidget(Qt::RightDockWidgetArea, outlineDock);
    window.addDockWidget(Qt::RightDockWidgetArea, charactersDock);
    window.splitDockWidget(elementDock, outlineDock, Qt::Vertical);
    window.tabifyDockWidget(outlineDock, charactersDock);
    outlineDock->raise();

    elementDock->setMinimumWidth(UiSpacing::SidebarMinWidth);
    elementDock->setMaximumWidth(UiSpacing::SidebarMaxWidth);
    outlineDock->setMinimumWidth(UiSpacing::SidebarMinWidth);
    outlineDock->setMaximumWidth(UiSpacing::SidebarMaxWidth);
    charactersDock->setMinimumWidth(UiSpacing::SidebarMinWidth);
    charactersDock->setMaximumWidth(UiSpacing::SidebarMaxWidth);

    const QByteArray savedGeometry = settings.value("window/geometry").toByteArray();
    if (!savedGeometry.isEmpty()) {
        window.restoreGeometry(savedGeometry);
    }

    const QByteArray savedState = settings.value("window/state").toByteArray();
    if (!savedState.isEmpty()) {
        window.restoreState(savedState, layoutStateVersion);
    }
    normalizeDockTabBars();

    elementDock->hide();
    outlineDock->hide();
    charactersDock->hide();

    auto applyBalancedSidebarSplit = [&window, elementDock, outlineDock]() {
        QList<QDockWidget*> docks{elementDock, outlineDock};
        const int half = qMax(1, window.height() / 2);
        QList<int> sizes{half, half};
        window.resizeDocks(docks, sizes, Qt::Vertical);
    };

    auto applySidebarDefaultWidth = [&window, elementDock, outlineDock, charactersDock]() {
        QList<QDockWidget*> docks{elementDock, outlineDock, charactersDock};
        QList<int> sizes{UiSpacing::SidebarWidth, UiSpacing::SidebarWidth, UiSpacing::SidebarWidth};
        window.resizeDocks(docks, sizes, Qt::Horizontal);
    };

    auto applyDefaultPanelLayout = [&window, elementDock, outlineDock, charactersDock, applyBalancedSidebarSplit, applySidebarDefaultWidth]() {
        if (elementDock->isFloating()) {
            elementDock->setFloating(false);
        }
        if (outlineDock->isFloating()) {
            outlineDock->setFloating(false);
        }
        if (charactersDock->isFloating()) {
            charactersDock->setFloating(false);
        }

        window.addDockWidget(Qt::RightDockWidgetArea, elementDock);
        window.addDockWidget(Qt::RightDockWidgetArea, outlineDock);
        window.addDockWidget(Qt::RightDockWidgetArea, charactersDock);
        window.splitDockWidget(elementDock, outlineDock, Qt::Vertical);
        window.tabifyDockWidget(outlineDock, charactersDock);
        outlineDock->raise();
        const auto tabBars = window.findChildren<QTabBar*>();
        for (QTabBar *tabBar : tabBars) {
            tabBar->setElideMode(Qt::ElideNone);
            tabBar->setUsesScrollButtons(false);
            tabBar->setExpanding(true);
        }
        applySidebarDefaultWidth();
        applyBalancedSidebarSplit();

        elementDock->show();
        outlineDock->show();
        charactersDock->show();
    };
    
    // Function to create page view with scroll area
    QString currentFilePath;
    
    auto createPageView = [&window, stack, &currentPage, &currentFindBar, &currentFilePath, &persistedZoomSteps, saveAction, saveAsAction, exportFdxAction, exportPdfAction, undoAction, redoAction, findAction, findNextAction, findPreviousAction, spellcheckAction, zoomInAction, zoomOutAction, resetZoomAction, typePanel, outlinePanel, charactersPanel, elementDock, outlineDock, charactersDock, applyBalancedSidebarSplit, applySidebarDefaultWidth, normalizeDockTabBars]() -> PageView* {
        PageView *page = new PageView();
        currentPage = page;
        currentFilePath.clear();
        qDebug() << "[Main] PageView created";
        page->setZoomSteps(persistedZoomSteps);
        
        QScrollArea *scroll = new QScrollArea();
        scroll->setObjectName("editorScrollArea");
        scroll->setWidget(page);
        scroll->setWidgetResizable(true);
        scroll->setBackgroundRole(QPalette::Dark);
        scroll->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        
        typePanel->setPageView(page);
        outlinePanel->setEditor(page->editor());
        charactersPanel->setEditor(page->editor());
        
        // Create container widget for editor + panel
        QWidget *editorContainer = new QWidget();
        QVBoxLayout *containerLayout = new QVBoxLayout(editorContainer);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(0);

        FindBar *findBar = new FindBar(editorContainer);
        findBar->hide();
        containerLayout->addWidget(findBar, 0);
        containerLayout->addWidget(scroll, 1);
        currentFindBar = findBar;
        
        stack->addWidget(editorContainer);
        
        // Connect element type changes from editor
        QObject::connect(page->editor(), &ScriptEditor::elementChanged, typePanel, &ElementTypePanel::setCurrentType);
        
        // Connect type panel clicks to editor
        QObject::connect(typePanel, &ElementTypePanel::typeSelected, page->editor(), &ScriptEditor::applyFormat);
        
        // Connect undo/redo actions
        qDebug() << "[Main] Connecting undo/redo actions";
        QObject::connect(undoAction, &QAction::triggered, page->editor(), &ScriptEditor::undo);
        QObject::connect(redoAction, &QAction::triggered, page->editor(), &ScriptEditor::redo);
        
        // Update undo/redo state when document changes
        QObject::connect(page->editor()->document(), &QTextDocument::undoAvailable, undoAction, &QAction::setEnabled);
        QObject::connect(page->editor()->document(), &QTextDocument::redoAvailable, redoAction, &QAction::setEnabled);
        
        // Log undo/redo state changes
        QObject::connect(page->editor()->document(), &QTextDocument::undoAvailable, [undoAction](bool available) {
            qDebug() << "[Main] undoAvailable signal received:" << available << "-> setting action enabled to:" << available;
            qDebug() << "[Main] undoAction->isEnabled() after signal:" << undoAction->isEnabled();
        });
        QObject::connect(page->editor()->document(), &QTextDocument::redoAvailable, [redoAction](bool available) {
            qDebug() << "[Main] redoAvailable signal received:" << available << "-> setting action enabled to:" << available;
            qDebug() << "[Main] redoAction->isEnabled() after signal:" << redoAction->isEnabled();
        });
        
        stack->setCurrentWidget(editorContainer);
        page->editor()->setFocus();
        saveAction->setEnabled(true);
        saveAsAction->setEnabled(true);
        exportFdxAction->setEnabled(true);
        exportPdfAction->setEnabled(true);
        findAction->setEnabled(true);
        findNextAction->setEnabled(true);
        findPreviousAction->setEnabled(true);
        spellcheckAction->setEnabled(true);
        spellcheckAction->setChecked(page->editor()->spellcheckEnabled());
        zoomInAction->setEnabled(true);
        zoomOutAction->setEnabled(true);
        resetZoomAction->setEnabled(true);
        elementDock->show();
        outlineDock->show();
        charactersDock->show();
        window.tabifyDockWidget(outlineDock, charactersDock);
        outlineDock->raise();
        normalizeDockTabBars();
        applySidebarDefaultWidth();
        applyBalancedSidebarSplit();
        qDebug() << "[Main] Before setting initial undo/redo state: isUndoAvailable=" << page->editor()->document()->isUndoAvailable() << "isRedoAvailable=" << page->editor()->document()->isRedoAvailable();
        undoAction->setEnabled(page->editor()->document()->isUndoAvailable());
        redoAction->setEnabled(page->editor()->document()->isRedoAvailable());
        qDebug() << "[Main] After setting initial undo/redo state: undoAction enabled=" << undoAction->isEnabled() << "redoAction enabled=" << redoAction->isEnabled();
        qDebug() << "[Main] Switched to editor view";

        QObject::connect(findBar, &FindBar::queryChanged, page->editor(), &ScriptEditor::setFindQuery);
        QObject::connect(findBar, &FindBar::optionsChanged, page->editor(), &ScriptEditor::setFindOptions);
        QObject::connect(findBar, &FindBar::findNextRequested, page->editor(), &ScriptEditor::findNext);
        QObject::connect(findBar, &FindBar::findPreviousRequested, page->editor(), &ScriptEditor::findPrevious);
        QObject::connect(findBar, &FindBar::closeRequested, [findBar, editor = page->editor()]() {
            findBar->hide();
            editor->setFocus();
        });

        QObject::connect(page->editor(), &ScriptEditor::findResultsChanged, findBar, [findBar](int activeIndex, int totalMatches) {
            findBar->setMatchStatus(activeIndex, totalMatches);
        });
        
        return page;
    };

    QObject::connect(findAction, &QAction::triggered, [&]() {
        if (!currentPage || !currentFindBar) {
            return;
        }
        currentFindBar->show();
        currentFindBar->focusAndSelectAll();
    });

    QObject::connect(findNextAction, &QAction::triggered, [&]() {
        if (!currentPage) {
            return;
        }
        currentPage->editor()->findNext();
    });

    QObject::connect(findPreviousAction, &QAction::triggered, [&]() {
        if (!currentPage) {
            return;
        }
        currentPage->editor()->findPrevious();
    });

    QObject::connect(spellcheckAction, &QAction::toggled, [&](bool enabled) {
        if (!currentPage) {
            return;
        }
        currentPage->editor()->setSpellcheckEnabled(enabled);
    });
    
    // Connect start screen signals
    QObject::connect(startScreen, &StartScreen::newDocument, [&]() {
        qDebug() << "[Main] New document requested";
        PageView *page = createPageView();
        // Start with blank document
    });
    
    QObject::connect(startScreen, &StartScreen::loadDocument, [&](const QString &filePath) {
        qDebug() << "[Main] Load document requested:" << filePath;
        PageView *page = createPageView();
        if (page->loadFromFile(filePath)) {
            currentFilePath = filePath;
        } else {
            QMessageBox::warning(&window, "Load Error", "Failed to load screenplay file.");
        }
    });
    
    // Connect save actions
    QObject::connect(saveAction, &QAction::triggered, [&]() {
        if (!currentPage) return;
        
        if (currentFilePath.isEmpty()) {
            // No file path yet, do Save As
            QString filePath = QFileDialog::getSaveFileName(&window, "Save Screenplay", "", "ScreenQt Files (*.sqt)");
            if (filePath.isEmpty()) return;
            currentFilePath = filePath;
        }
        
        if (currentPage->saveToFile(currentFilePath)) {
            qDebug() << "[Main] Saved to:" << currentFilePath;
        } else {
            QMessageBox::warning(&window, "Save Error", "Failed to save screenplay file.");
        }
    });

    // Connect open action (loads into a new page view)
    QObject::connect(openAction, &QAction::triggered, [&]() {
        QString filePath = QFileDialog::getOpenFileName(&window, "Open Screenplay", "", "ScreenQt Files (*.sqt);;All Files (*)");
        if (filePath.isEmpty()) return;
        qDebug() << "[Main] Open document requested:" << filePath;
        PageView *page = createPageView();
        if (page->loadFromFile(filePath)) {
            currentFilePath = filePath;
        } else {
            QMessageBox::warning(&window, "Load Error", "Failed to load screenplay file.");
        }
    });

    QObject::connect(importFdxAction, &QAction::triggered, [&]() {
        QString filePath = QFileDialog::getOpenFileName(&window, "Import Final Draft", "", "Final Draft Files (*.fdx);;All Files (*)");
        if (filePath.isEmpty()) return;

        qDebug() << "[Main] Import FDX requested:" << filePath;
        PageView *page = createPageView();
        if (page->loadFromFile(filePath)) {
            currentFilePath.clear();
        } else {
            QMessageBox::warning(&window, "Import Error", "Failed to import Final Draft file.");
        }
    });
    
    QObject::connect(saveAsAction, &QAction::triggered, [&]() {
        if (!currentPage) return;
        
        QString filePath = QFileDialog::getSaveFileName(&window, "Save Screenplay As", "", "ScreenQt Files (*.sqt)");
        if (filePath.isEmpty()) return;
        
        if (currentPage->saveToFile(filePath)) {
            currentFilePath = filePath;
            qDebug() << "[Main] Saved as:" << currentFilePath;
        } else {
            QMessageBox::warning(&window, "Save Error", "Failed to save screenplay file.");
        }
    });

    QObject::connect(exportFdxAction, &QAction::triggered, [&]() {
        if (!currentPage) return;

        QString filePath = QFileDialog::getSaveFileName(&window, "Export Screenplay as Final Draft", "", "Final Draft Files (*.fdx)");
        if (filePath.isEmpty()) return;

        if (currentPage->saveToFile(filePath)) {
            qDebug() << "[Main] Exported to FDX:" << filePath;
            QMessageBox::information(&window, "Export Successful", "Screenplay exported to Final Draft successfully.");
        } else {
            QMessageBox::warning(&window, "Export Error", "Failed to export screenplay to Final Draft.");
        }
    });
    
    QObject::connect(exportPdfAction, &QAction::triggered, [&]() {
        if (!currentPage) return;
        
        QString filePath = QFileDialog::getSaveFileName(&window, "Export Screenplay as PDF", "", "PDF Files (*.pdf)");
        if (filePath.isEmpty()) return;
        
        if (currentPage->exportToPdf(filePath)) {
            qDebug() << "[Main] Exported to PDF:" << filePath;
            QMessageBox::information(&window, "Export Successful", "Screenplay exported to PDF successfully.");
        } else {
            QMessageBox::warning(&window, "Export Error", "Failed to export screenplay to PDF.");
        }
    });

    QObject::connect(zoomInAction, &QAction::triggered, [&]() {
        if (!currentPage) return;
        currentPage->zoomInView();
        persistedZoomSteps = currentPage->zoomSteps();
    });

    QObject::connect(zoomOutAction, &QAction::triggered, [&]() {
        if (!currentPage) return;
        currentPage->zoomOutView();
        persistedZoomSteps = currentPage->zoomSteps();
    });

    QObject::connect(resetZoomAction, &QAction::triggered, [&]() {
        if (!currentPage) return;
        currentPage->resetZoom();
        persistedZoomSteps = currentPage->zoomSteps();
    });

    QObject::connect(resetLayoutAction, &QAction::triggered, [&]() {
        applyDefaultPanelLayout();
    });

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        if (currentPage) {
            persistedZoomSteps = currentPage->zoomSteps();
        }
        settings.setValue("editor/zoomSteps", persistedZoomSteps);
        settings.setValue("window/geometry", window.saveGeometry());
        settings.setValue("window/state", window.saveState(layoutStateVersion));
    });
    
    stack->setCurrentWidget(startScreen);
    qDebug() << "[Main] Start screen displayed";

    window.show();
    qDebug() << "[Main] Window shown, entering event loop";
    
    return app.exec();
}
