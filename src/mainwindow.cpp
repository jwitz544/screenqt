#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFont>
#include <QFontInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSettings>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabBar>
#include <QTextBlock>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>

#include "characterspanel.h"
#include "elementtypepanel.h"
#include "findbar.h"
#include "fountainio.h"
#include "outlinepanel.h"
#include "pageview.h"
#include "screenplayio.h"
#include "scripteditor.h"
#include "startscreen.h"
#include "titlepage_dialog.h"

// ---------------------------------------------------------------------------
// UI constants  — VS Code Dark+ inspired, neutral dark palette
// ---------------------------------------------------------------------------
namespace UiSpacing {
constexpr int SidebarWidth    = 280;
constexpr int SidebarMinWidth = 200;
constexpr int SidebarMaxWidth = 500;
}

static QString buildAppStyleSheet()
{
    return QStringLiteral(
        // ── Base chrome ──────────────────────────────────────────────────────
        "QMainWindow { background: #1E1E1E; color: #D4D4D4; }"
        "QWidget      { color: #D4D4D4; }"
        "QMainWindow::separator { background: #474747; width: 1px; height: 1px; }"

        // ── Menu bar ─────────────────────────────────────────────────────────
        "QMenuBar {"
        "  background: #333333;"
        "  border-bottom: 1px solid #474747;"
        "}"
        "QMenuBar::item {"
        "  padding: 4px 10px;"
        "  background: transparent;"
        "  color: #CCCCCC;"
        "  font-size: 12px;"
        "}"
        "QMenuBar::item:selected { background: #505050; }"

        // ── Menu popup ───────────────────────────────────────────────────────
        "QMenu {"
        "  background: #252526;"
        "  border: 1px solid #454545;"
        "  padding: 4px 0px;"
        "}"
        "QMenu::item {"
        "  padding: 5px 32px 5px 14px;"
        "  font-size: 12px;"
        "  color: #CCCCCC;"
        "}"
        "QMenu::item:selected { background: #094771; color: #FFFFFF; }"
        "QMenu::separator { height: 1px; background: #454545; margin: 3px 0; }"

        // ── Dock tab bar ─────────────────────────────────────────────────────
        "QMainWindow QTabBar { background: #252526; }"
        "QMainWindow QTabBar::tab {"
        "  background: #2D2D2D;"
        "  color: #858585;"
        "  padding: 4px 14px;"
        "  border: none;"
        "  border-right: 1px solid #3C3C3C;"
        "  font-size: 10px;"
        "  font-weight: 700;"
        "  letter-spacing: 1px;"
        "  min-width: 64px;"
        "}"
        "QMainWindow QTabBar::tab:selected {"
        "  background: #252526;"
        "  color: #D4D4D4;"
        "  border-bottom: 2px solid #007ACC;"
        "}"
        "QMainWindow QTabBar::tab:hover { background: #2A2D2E; color: #CCCCCC; }"

        // ── Dock widgets ─────────────────────────────────────────────────────
        "QDockWidget {"
        "  color: #D4D4D4;"
        "  titlebar-close-icon: none;"
        "  titlebar-normal-icon: none;"
        "}"
        "QDockWidget::title {"
        "  background: #252526;"
        "  color: #858585;"
        "  padding: 5px 0px 5px 12px;"
        "  text-align: left;"
        "  border: none;"
        "  border-bottom: 1px solid #3C3C3C;"
        "  font-size: 10px;"
        "  font-weight: 700;"
        "  letter-spacing: 1px;"
        "}"
        "QDockWidget > QWidget { background: #252526; }"

        // ── Side panels ──────────────────────────────────────────────────────
        "QWidget#elementTypePanel, QWidget#outlinePanel, QWidget#charactersPanel {"
        "  background: #252526;"
        "}"
        "QLabel#panelSectionHeader {"
        "  color: #858585;"
        "  font-size: 10px;"
        "  font-weight: 700;"
        "  letter-spacing: 1px;"
        "  padding: 8px 12px 4px 12px;"
        "}"
        "QLabel#panelMeta {"
        "  color: #6F6F6F;"
        "  font-size: 10px;"
        "  padding: 0px 12px 4px 12px;"
        "}"
        "QFrame#panelDivider { background: #3C3C3C; max-height: 1px; }"

        // ── Element type buttons ──────────────────────────────────────────────
        "QPushButton#sidebarItem {"
        "  background: transparent;"
        "  color: #CCCCCC;"
        "  text-align: left;"
        "  padding: 0px 8px 0px 20px;"
        "  border: none;"
        "  border-left: 2px solid transparent;"
        "  font-size: 12px;"
        "  font-weight: 400;"
        "  min-height: 24px;"
        "  max-height: 24px;"
        "}"
        "QPushButton#sidebarItem:hover { background: #2A2D2E; color: #D4D4D4; }"
        "QPushButton#sidebarItem:checked {"
        "  background: #37373F;"
        "  border-left: 2px solid #007ACC;"
        "  color: #FFFFFF;"
        "  font-weight: 500;"
        "}"

        // ── Scene / character lists ───────────────────────────────────────────
        "QListWidget#sceneList {"
        "  background: transparent;"
        "  border: none;"
        "  outline: none;"
        "}"
        "QListWidget#sceneList::item {"
        "  padding: 0px 8px 0px 20px;"
        "  border: none;"
        "  color: #CCCCCC;"
        "  font-size: 12px;"
        "  min-height: 22px;"
        "}"
        "QListWidget#sceneList::item:hover { background: #2A2D2E; }"
        "QListWidget#sceneList::item:selected {"
        "  background: #094771;"
        "  color: #FFFFFF;"
        "}"

        // ── Editor scroll area ────────────────────────────────────────────────
        "QScrollArea#editorScrollArea { background: #1E1E1E; border: none; }"
        "QScrollArea#editorScrollArea > QWidget > QWidget { background: transparent; }"

        // ── Scrollbars ────────────────────────────────────────────────────────
        "QScrollBar:vertical { background: transparent; width: 10px; margin: 0px; }"
        "QScrollBar::handle:vertical {"
        "  background: #424242;"
        "  border-radius: 5px;"
        "  min-height: 20px;"
        "  margin: 1px 2px;"
        "}"
        "QScrollBar::handle:vertical:hover { background: #686868; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"

        // ── Script editor text area ───────────────────────────────────────────
        "QTextEdit#scriptEditor { color: #1E2127; background: transparent; border: none; }"
        "QTextEdit { selection-background-color: #264F78; selection-color: #FFFFFF; }"

        // ── Autocomplete popup ────────────────────────────────────────────────
        "QAbstractItemView#scriptEditorCompleterPopup {"
        "  background: #252526;"
        "  color: #D4D4D4;"
        "  border: 1px solid #454545;"
        "  border-radius: 3px;"
        "  padding: 2px;"
        "  outline: none;"
        "  font-size: 12px;"
        "}"
        "QAbstractItemView#scriptEditorCompleterPopup::item { padding: 3px 10px; min-height: 22px; }"
        "QAbstractItemView#scriptEditorCompleterPopup::item:hover { background: #2A2D2E; }"
        "QAbstractItemView#scriptEditorCompleterPopup::item:selected {"
        "  background: #094771; color: #FFFFFF;"
        "}"

        // ── Find bar ─────────────────────────────────────────────────────────
        "QFrame#findBar {"
        "  background: #252526;"
        "  border-bottom: 1px solid #474747;"
        "}"
        "QFrame#findBar QLabel {"
        "  color: #858585;"
        "  font-size: 11px;"
        "  font-weight: 600;"
        "  min-width: 52px;"
        "}"
        "QFrame#findBar QLineEdit {"
        "  background: #3C3C3C;"
        "  border: 1px solid #3C3C3C;"
        "  border-radius: 3px;"
        "  padding: 3px 7px;"
        "  color: #D4D4D4;"
        "  font-size: 12px;"
        "  selection-background-color: #264F78;"
        "}"
        "QFrame#findBar QLineEdit:focus { border-color: #007ACC; }"
        "QFrame#findBar QPushButton {"
        "  background: #3C3C3C;"
        "  color: #CCCCCC;"
        "  border: 1px solid #3C3C3C;"
        "  border-radius: 3px;"
        "  padding: 3px 10px;"
        "  font-size: 11px;"
        "  min-height: 22px;"
        "}"
        "QFrame#findBar QPushButton:hover { background: #4C4C4C; border-color: #686868; }"
        "QFrame#findBar QPushButton:pressed { background: #0E639C; border-color: #007ACC; color: #FFFFFF; }"
        "QFrame#findBar QCheckBox { color: #CCCCCC; font-size: 11px; spacing: 4px; }"
        "QFrame#findBar QCheckBox::indicator {"
        "  width: 13px; height: 13px;"
        "  border: 1px solid #686868; border-radius: 2px; background: transparent;"
        "}"
        "QFrame#findBar QCheckBox::indicator:checked { background: #007ACC; border-color: #007ACC; }"
        "QLabel#panelMeta { color: #6F6F6F; font-size: 10px; }"

        // ── Start screen ──────────────────────────────────────────────────────
        "QWidget#startScreen { background: #1E1E1E; }"
        "QLabel#startAppName { color: #D4D4D4; font-size: 42px; font-weight: 200; }"
        "QLabel#startTagline { color: #6F6F6F; font-size: 13px; font-weight: 400; }"
        "QLabel#startSectionHeader {"
        "  color: #858585; font-size: 10px; font-weight: 700; letter-spacing: 2px;"
        "}"
        "QPushButton#startAction {"
        "  background: transparent;"
        "  color: #CCCCCC;"
        "  border: none;"
        "  text-align: left;"
        "  padding: 4px 0px;"
        "  font-size: 13px;"
        "  font-weight: 400;"
        "}"
        "QPushButton#startAction:hover { color: #4FC1FF; }"

        // ── Status bar ────────────────────────────────────────────────────────
        "QStatusBar { background: #007ACC; color: #FFFFFF; border: none; }"
        "QStatusBar::item { border: none; }"
        "QStatusBar QLabel {"
        "  color: #FFFFFF; font-size: 11px; padding: 0px 10px;"
        "  background: transparent;"
        "}"
        "QStatusBar QLabel:hover { background: rgba(255,255,255,0.15); }"

        // ── General ───────────────────────────────────────────────────────────
        "QPushButton:disabled { color: #6F6F6F; }"
    );
}

// ---------------------------------------------------------------------------
// MainWindow
// ---------------------------------------------------------------------------
static constexpr int kLayoutStateVersion       = 1;
static constexpr int kDefaultZoomSteps         = 2;
static constexpr int kZoomCalibrationRevision  = 1;
static constexpr int kMaxRecentFiles           = 10;
static constexpr int kAutoSaveIntervalMs       = 2 * 60 * 1000; // 2 minutes
static constexpr int kCursorUpdateDelayMs      = 150;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("ScreenQt");
    resize(900, 700);
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks | QMainWindow::GroupedDragging);
    setStyleSheet(buildAppStyleSheet());

    // Load persisted settings
    QSettings settings("ScreenQt", "ScreenQt");
    m_persistedZoomSteps = settings.value("editor/zoomSteps", 0).toInt();
    const int storedCalibRev = settings.value("editor/zoomCalibrationRevision", 0).toInt();
    if (storedCalibRev < kZoomCalibrationRevision && m_persistedZoomSteps == 0) {
        m_persistedZoomSteps = kDefaultZoomSteps;
        settings.setValue("editor/zoomSteps", m_persistedZoomSteps);
    }
    settings.setValue("editor/zoomCalibrationRevision", kZoomCalibrationRevision);

    // Central stack
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    m_startScreen = new StartScreen();
    m_stack->addWidget(m_startScreen);

    setupMenus();
    setupDocks();
    setupStatusBar();
    setupConnections();
    setupAutoSave();

    // Load recent files
    loadRecentFiles();

    // Restore geometry / dock state
    const QByteArray savedGeometry = settings.value("window/geometry").toByteArray();
    if (!savedGeometry.isEmpty()) {
        restoreGeometry(savedGeometry);
    }
    const QByteArray savedState = settings.value("window/state").toByteArray();
    if (!savedState.isEmpty()) {
        restoreState(savedState, kLayoutStateVersion);
    }
    normalizeDockTabBars();

    m_elementDock->hide();
    m_outlineDock->hide();
    m_charactersDock->hide();

    m_stack->setCurrentWidget(m_startScreen);
}

MainWindow::~MainWindow() = default;

// ---------------------------------------------------------------------------
// Setup helpers
// ---------------------------------------------------------------------------
void MainWindow::setupMenus()
{
    QMenuBar *bar = menuBar();

    // ── File menu ──────────────────────────────────────────────────────────
    QMenu *fileMenu = bar->addMenu("&File");

    m_saveAction = fileMenu->addAction("&Save");
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setEnabled(false);

    m_saveAsAction = fileMenu->addAction("Save &As...");
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    m_saveAsAction->setEnabled(false);

    m_openAction = fileMenu->addAction("&Open...");
    m_openAction->setShortcut(QKeySequence::Open);

    fileMenu->addSeparator();

    m_titlePageAction = fileMenu->addAction("&Title Page\u2026");
    m_titlePageAction->setEnabled(false);

    fileMenu->addSeparator();

    QMenu *importExportMenu = fileMenu->addMenu("&Import/Export");
    m_importFdxAction = importExportMenu->addAction("&Import Final Draft (FDX)...");
    m_exportFdxAction = importExportMenu->addAction("Export to &Final Draft (FDX)...");
    m_exportFdxAction->setEnabled(false);
    importExportMenu->addSeparator();
    m_importFountainAction = importExportMenu->addAction("Import &Fountain (.fountain)...");
    m_exportFountainAction = importExportMenu->addAction("Export as F&ountain (.fountain)...");
    m_exportFountainAction->setEnabled(false);
    importExportMenu->addSeparator();
    m_exportPdfAction = importExportMenu->addAction("Export to &PDF...");
    m_exportPdfAction->setEnabled(false);

    // ── Edit menu ──────────────────────────────────────────────────────────
    QMenu *editMenu = bar->addMenu("&Edit");

    m_undoAction = editMenu->addAction("&Undo");
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setEnabled(false);

    m_redoAction = editMenu->addAction("&Redo");
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setEnabled(false);

    editMenu->addSeparator();

    m_findAction = editMenu->addAction("&Find...");
    m_findAction->setShortcut(QKeySequence::Find);
    m_findAction->setEnabled(false);

    m_findNextAction = editMenu->addAction("Find &Next");
    m_findNextAction->setShortcut(QKeySequence::FindNext);
    m_findNextAction->setEnabled(false);

    m_findPreviousAction = editMenu->addAction("Find &Previous");
    m_findPreviousAction->setShortcut(QKeySequence::FindPrevious);
    m_findPreviousAction->setEnabled(false);

    editMenu->addSeparator();

    m_spellcheckAction = editMenu->addAction("&Spellcheck");
    m_spellcheckAction->setCheckable(true);
    m_spellcheckAction->setChecked(true);
    m_spellcheckAction->setEnabled(false);

    // ── View menu ──────────────────────────────────────────────────────────
    QMenu *viewMenu = bar->addMenu("&View");

    m_zoomInAction = viewMenu->addAction("Zoom &In");
    m_zoomInAction->setShortcut(QKeySequence::ZoomIn);
    m_zoomInAction->setEnabled(false);

    m_zoomOutAction = viewMenu->addAction("Zoom &Out");
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    m_zoomOutAction->setEnabled(false);

    m_resetZoomAction = viewMenu->addAction("Reset &Zoom");
    m_resetZoomAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    m_resetZoomAction->setEnabled(false);

    m_resetLayoutAction = viewMenu->addAction("Reset &Layout");

    // ── Tools menu ─────────────────────────────────────────────────────────
    QMenu *toolsMenu = bar->addMenu("&Tools");

    m_statsAction = toolsMenu->addAction("Script &Statistics\u2026");
    m_statsAction->setEnabled(false);

    // ── Wire actions ───────────────────────────────────────────────────────
    connect(m_saveAction, &QAction::triggered, this, [this] {
        if (doSave()) clearAutoSave();
    });

    connect(m_saveAsAction, &QAction::triggered, this, [this] {
        if (!m_currentPage) return;
        QString filePath = QFileDialog::getSaveFileName(this, "Save Screenplay As", "", "ScreenQt Files (*.sqt)");
        if (filePath.isEmpty()) return;
        if (m_currentPage->saveToFile(filePath)) {
            m_currentFilePath = filePath;
            addRecentFile(filePath);
            setDirty(false);
            clearAutoSave();
        } else {
            QMessageBox::warning(this, "Save Error", "Failed to save screenplay file.");
        }
    });

    connect(m_openAction, &QAction::triggered, this, [this] {
        if (!maybePromptSave()) return;
        QString filePath = QFileDialog::getOpenFileName(
            this, "Open Screenplay", "",
            "ScreenQt Files (*.sqt);;All Files (*)");
        if (filePath.isEmpty()) return;
        createPageView();
        if (m_currentPage->loadFromFile(filePath)) {
            m_currentFilePath = filePath;
            addRecentFile(filePath);
            setDirty(false);
            checkAutoSaveRecovery(filePath);
        } else {
            QMessageBox::warning(this, "Load Error", "Failed to load screenplay file.");
        }
    });

    connect(m_titlePageAction, &QAction::triggered, this, &MainWindow::openTitlePageDialog);

    connect(m_importFdxAction, &QAction::triggered, this, [this] {
        if (!maybePromptSave()) return;
        QString filePath = QFileDialog::getOpenFileName(this, "Import Final Draft", "", "Final Draft Files (*.fdx);;All Files (*)");
        if (filePath.isEmpty()) return;
        createPageView();
        if (m_currentPage->loadFromFile(filePath)) {
            m_currentFilePath.clear();
            setDirty(false);
        } else {
            QMessageBox::warning(this, "Import Error", "Failed to import Final Draft file.");
        }
    });

    connect(m_exportFdxAction, &QAction::triggered, this, [this] {
        if (!m_currentPage) return;
        QString filePath = QFileDialog::getSaveFileName(this, "Export as Final Draft", "", "Final Draft Files (*.fdx)");
        if (filePath.isEmpty()) return;
        if (m_currentPage->saveToFile(filePath)) {
            QMessageBox::information(this, "Export Successful", "Screenplay exported to Final Draft successfully.");
        } else {
            QMessageBox::warning(this, "Export Error", "Failed to export screenplay to Final Draft.");
        }
    });

    connect(m_importFountainAction, &QAction::triggered, this, [this] {
        if (!maybePromptSave()) return;
        QString filePath = QFileDialog::getOpenFileName(
            this, "Import Fountain", "",
            "Fountain Files (*.fountain *.txt);;All Files (*)");
        if (filePath.isEmpty()) return;
        createPageView();
        if (FountainIO::loadFountain(m_currentPage->editor(), filePath)) {
            m_currentFilePath.clear();
            setDirty(true);
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Import Error", "Failed to import Fountain file.");
        }
    });

    connect(m_exportFountainAction, &QAction::triggered, this, [this] {
        if (!m_currentPage) return;
        QString filePath = QFileDialog::getSaveFileName(
            this, "Export as Fountain", "",
            "Fountain Files (*.fountain)");
        if (filePath.isEmpty()) return;
        if (FountainIO::saveFountain(m_currentPage->editor(), filePath)) {
            QMessageBox::information(this, "Export Successful", "Screenplay exported as Fountain successfully.");
        } else {
            QMessageBox::warning(this, "Export Error", "Failed to export Fountain file.");
        }
    });

    connect(m_exportPdfAction, &QAction::triggered, this, [this] {
        if (!m_currentPage) return;
        QString filePath = QFileDialog::getSaveFileName(this, "Export Screenplay as PDF", "", "PDF Files (*.pdf)");
        if (filePath.isEmpty()) return;
        if (m_currentPage->exportToPdf(filePath)) {
            QMessageBox::information(this, "Export Successful", "Screenplay exported to PDF successfully.");
        } else {
            QMessageBox::warning(this, "Export Error", "Failed to export screenplay to PDF.");
        }
    });

    connect(m_undoAction, &QAction::triggered, this, [this] {
        if (m_currentPage) m_currentPage->editor()->undo();
    });
    connect(m_redoAction, &QAction::triggered, this, [this] {
        if (m_currentPage) m_currentPage->editor()->redo();
    });

    connect(m_findAction, &QAction::triggered, this, [this] {
        if (!m_currentFindBar) return;
        m_currentFindBar->show();
        m_currentFindBar->focusAndSelectAll();
    });

    connect(m_findNextAction, &QAction::triggered, this, [this] {
        if (m_currentPage) m_currentPage->editor()->findNext();
    });
    connect(m_findPreviousAction, &QAction::triggered, this, [this] {
        if (m_currentPage) m_currentPage->editor()->findPrevious();
    });

    connect(m_spellcheckAction, &QAction::toggled, this, [this](bool enabled) {
        if (m_currentPage) m_currentPage->editor()->setSpellcheckEnabled(enabled);
    });

    connect(m_zoomInAction, &QAction::triggered, this, [this] {
        if (!m_currentPage) return;
        m_currentPage->zoomInView();
        m_persistedZoomSteps = m_currentPage->zoomSteps();
    });
    connect(m_zoomOutAction, &QAction::triggered, this, [this] {
        if (!m_currentPage) return;
        m_currentPage->zoomOutView();
        m_persistedZoomSteps = m_currentPage->zoomSteps();
    });
    connect(m_resetZoomAction, &QAction::triggered, this, [this] {
        if (!m_currentPage) return;
        m_currentPage->resetZoom();
        m_persistedZoomSteps = m_currentPage->zoomSteps();
    });

    connect(m_resetLayoutAction, &QAction::triggered, this, &MainWindow::applyDefaultPanelLayout);

    connect(m_statsAction, &QAction::triggered, this, &MainWindow::showScriptStatistics);
}

void MainWindow::setupDocks()
{
    m_typePanel       = new ElementTypePanel();
    m_outlinePanel    = new OutlinePanel();
    m_charactersPanel = new CharactersPanel();

    m_elementDock = new QDockWidget("ELEMENTS", this);
    m_elementDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_elementDock->setWidget(m_typePanel);

    m_outlineDock = new QDockWidget("OUTLINE", this);
    m_outlineDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_outlineDock->setWidget(m_outlinePanel);

    m_charactersDock = new QDockWidget("CHARACTERS", this);
    m_charactersDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_charactersDock->setWidget(m_charactersPanel);

    addDockWidget(Qt::RightDockWidgetArea, m_elementDock);
    addDockWidget(Qt::RightDockWidgetArea, m_outlineDock);
    addDockWidget(Qt::RightDockWidgetArea, m_charactersDock);
    splitDockWidget(m_elementDock, m_outlineDock, Qt::Vertical);
    tabifyDockWidget(m_outlineDock, m_charactersDock);
    m_outlineDock->raise();

    m_elementDock->setMinimumWidth(UiSpacing::SidebarMinWidth);
    m_elementDock->setMaximumWidth(UiSpacing::SidebarMaxWidth);
    m_outlineDock->setMinimumWidth(UiSpacing::SidebarMinWidth);
    m_outlineDock->setMaximumWidth(UiSpacing::SidebarMaxWidth);
    m_charactersDock->setMinimumWidth(UiSpacing::SidebarMinWidth);
    m_charactersDock->setMaximumWidth(UiSpacing::SidebarMaxWidth);
}

void MainWindow::setupConnections()
{
    connect(m_startScreen, &StartScreen::newDocument, this, [this] {
        if (!maybePromptSave()) return;
        setWindowTitle("ScreenQt");
        if (m_elementStatusLabel) m_elementStatusLabel->setText("Ready");
        if (m_pageStatusLabel)    m_pageStatusLabel->setText("");
        createPageView();
    });

    connect(m_startScreen, &StartScreen::loadDocument, this, [this](const QString &filePath) {
        if (!maybePromptSave()) return;

        // Determine format
        const QString ext = QFileInfo(filePath).suffix().toLower();

        createPageView();

        if (ext == "fountain") {
            if (FountainIO::loadFountain(m_currentPage->editor(), filePath)) {
                m_currentFilePath.clear();
                setDirty(true);
                updateWindowTitle();
            } else {
                QMessageBox::warning(this, "Load Error", "Failed to load Fountain file.");
            }
            return;
        }

        if (m_currentPage->loadFromFile(filePath)) {
            m_currentFilePath = filePath;
            addRecentFile(filePath);
            setDirty(false);
            checkAutoSaveRecovery(filePath);
        } else {
            QMessageBox::warning(this, "Load Error", "Failed to load screenplay file.");
        }
    });
}

// ---------------------------------------------------------------------------
// Auto-save
// ---------------------------------------------------------------------------
void MainWindow::setupAutoSave()
{
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(kAutoSaveIntervalMs);
    m_autoSaveTimer->start();
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::doAutoSave);

    m_cursorUpdateTimer = new QTimer(this);
    m_cursorUpdateTimer->setInterval(kCursorUpdateDelayMs);
    m_cursorUpdateTimer->setSingleShot(true);
    connect(m_cursorUpdateTimer, &QTimer::timeout, this, &MainWindow::updateCursorStatus);
}

void MainWindow::doAutoSave()
{
    if (!m_currentPage || !m_isDirty) return;

    QString autoSavePath;
    if (!m_currentFilePath.isEmpty()) {
        autoSavePath = m_currentFilePath + ".bak";
    } else {
        const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(appData + "/autosave");
        autoSavePath = appData + "/autosave/untitled.sqt.bak";
    }

    DocumentSettings docSettings = m_currentPage->documentSettings();
    ScreenplayIO::saveDocument(m_currentPage->editor(), autoSavePath, &docSettings);
}

void MainWindow::clearAutoSave()
{
    if (!m_currentFilePath.isEmpty()) {
        const QString bakPath = m_currentFilePath + ".bak";
        if (QFileInfo::exists(bakPath)) {
            QFile::remove(bakPath);
        }
    }
}

void MainWindow::checkAutoSaveRecovery(const QString &filePath)
{
    const QString bakPath = filePath + ".bak";
    QFileInfo bakInfo(bakPath);
    QFileInfo mainInfo(filePath);

    if (!bakInfo.exists()) return;
    if (bakInfo.lastModified() <= mainInfo.lastModified()) return;

    const auto btn = QMessageBox::question(
        this,
        "Auto-Save Recovery",
        QString("A newer auto-saved version of \"%1\" was found.\n"
                "Would you like to restore it?").arg(mainInfo.fileName()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (btn == QMessageBox::Yes) {
        DocumentSettings docSettings;
        int lineCount = 0;
        if (ScreenplayIO::loadDocument(m_currentPage->editor(), bakPath, lineCount, &docSettings)) {
            m_currentPage->setDocumentSettings(docSettings);
            setDirty(true);
        }
    }
}

// ---------------------------------------------------------------------------
// Recent files
// ---------------------------------------------------------------------------
void MainWindow::loadRecentFiles()
{
    QSettings settings("ScreenQt", "ScreenQt");
    m_recentFiles = settings.value("recentFiles").toStringList();

    // Prune missing files
    QStringList pruned;
    for (const QString &f : std::as_const(m_recentFiles)) {
        if (QFileInfo::exists(f)) pruned.append(f);
    }
    m_recentFiles = pruned;

    m_startScreen->setRecentFiles(m_recentFiles);
}

void MainWindow::addRecentFile(const QString &filePath)
{
    m_recentFiles.removeAll(filePath);
    m_recentFiles.prepend(filePath);
    while (m_recentFiles.size() > kMaxRecentFiles) {
        m_recentFiles.removeLast();
    }

    QSettings settings("ScreenQt", "ScreenQt");
    settings.setValue("recentFiles", m_recentFiles);

    m_startScreen->setRecentFiles(m_recentFiles);
}

// ---------------------------------------------------------------------------
// Status bar
// ---------------------------------------------------------------------------
void MainWindow::setupStatusBar()
{
    QStatusBar *bar = statusBar();
    bar->setSizeGripEnabled(false);

    m_elementStatusLabel = new QLabel("Ready", bar);
    m_elementStatusLabel->setObjectName("statusElement");
    bar->addWidget(m_elementStatusLabel);

    m_sceneStatusLabel = new QLabel("", bar);
    m_sceneStatusLabel->setObjectName("statusScene");
    bar->addWidget(m_sceneStatusLabel);

    // Spacer
    QWidget *spacer = new QWidget(bar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    bar->addWidget(spacer, 1);

    m_wordStatusLabel = new QLabel("", bar);
    m_wordStatusLabel->setObjectName("statusWords");
    bar->addPermanentWidget(m_wordStatusLabel);

    m_pageStatusLabel = new QLabel("", bar);
    m_pageStatusLabel->setObjectName("statusPage");
    bar->addPermanentWidget(m_pageStatusLabel);
}

void MainWindow::updateElementStatus(int elementType)
{
    static const QStringList names = {
        "Scene Heading", "Action", "Character",
        "Dialogue", "Parenthetical", "Shot", "Transition"
    };
    const QString name = (elementType >= 0 && elementType < names.size())
        ? names[elementType] : "Action";
    if (m_elementStatusLabel) {
        m_elementStatusLabel->setText(name);
    }
}

void MainWindow::updatePageStatus(int pageCount)
{
    if (m_pageStatusLabel) {
        m_pageStatusLabel->setText(pageCount == 1
            ? "1 pg"
            : QString("%1 pgs").arg(pageCount));
    }
}

void MainWindow::updateCursorStatus()
{
    if (!m_currentPage) return;
    ScriptEditor *ed = m_currentPage->editor();
    QTextDocument *doc = ed->document();
    const QTextCursor cursor = ed->textCursor();

    int wordCount = 0;
    int sceneNum = 0;
    bool pastCursor = false;

    static const QRegularExpression whitespace("\\s+");

    QTextBlock b = doc->begin();
    while (b.isValid()) {
        const int state = b.userState();
        if (!pastCursor && b == cursor.block()) {
            pastCursor = true;
        }
        if (!pastCursor && state == ScriptEditor::SceneHeading) {
            ++sceneNum;
        }
        if (state == ScriptEditor::Action || state == ScriptEditor::Dialogue) {
            const QString text = b.text().trimmed();
            if (!text.isEmpty()) {
                wordCount += text.split(whitespace, Qt::SkipEmptyParts).count();
            }
        }
        b = b.next();
    }
    // Count scene heading at cursor block too
    if (cursor.block().userState() == ScriptEditor::SceneHeading && !pastCursor) {
        ++sceneNum;
    }

    if (m_sceneStatusLabel) {
        m_sceneStatusLabel->setText(sceneNum > 0 ? QString("Sc %1").arg(sceneNum) : "");
    }
    if (m_wordStatusLabel) {
        m_wordStatusLabel->setText(wordCount > 0
            ? (wordCount == 1 ? "1 word" : QString("%1 words").arg(wordCount))
            : "");
    }
}

// ---------------------------------------------------------------------------
// Title page
// ---------------------------------------------------------------------------
void MainWindow::openTitlePageDialog()
{
    if (!m_currentPage) return;

    TitlePageDialog dlg(m_currentPage->documentSettings(), this);
    if (dlg.exec() != QDialog::Accepted) return;

    DocumentSettings newSettings = dlg.result();
    m_currentPage->setDocumentSettings(newSettings);
    setDirty(true);
}

// ---------------------------------------------------------------------------
// Script statistics
// ---------------------------------------------------------------------------
void MainWindow::showScriptStatistics()
{
    if (!m_currentPage) return;

    ScriptEditor *ed = m_currentPage->editor();
    QTextDocument *doc = ed->document();

    int sceneCount = 0;
    int wordCount  = 0;
    QSet<QString> characters;

    static const QRegularExpression whitespace("\\s+");

    QTextBlock b = doc->begin();
    while (b.isValid()) {
        const int state = b.userState();
        if (state == ScriptEditor::SceneHeading) {
            ++sceneCount;
        } else if (state == ScriptEditor::Action || state == ScriptEditor::Dialogue) {
            const QString text = b.text().trimmed();
            if (!text.isEmpty()) {
                wordCount += text.split(whitespace, Qt::SkipEmptyParts).count();
            }
        } else if (state == ScriptEditor::CharacterName) {
            const QString name = b.text().trimmed();
            if (!name.isEmpty()) characters.insert(name);
        }
        b = b.next();
    }

    const int pageCount = m_currentPage->pageCount();
    const int runtimeMins = pageCount;

    QDialog dlg(this);
    dlg.setWindowTitle("Script Statistics");
    dlg.setMinimumWidth(300);
    dlg.setStyleSheet(
        "QDialog { background: #1E1E1E; color: #D4D4D4; }"
        "QWidget { background: #1E1E1E; color: #D4D4D4; }"
        "QLabel { font-size: 12px; padding: 3px 0px; }"
        "QLabel#statKey { color: #858585; }"
        "QLabel#statVal { color: #D4D4D4; font-weight: 600; }"
        "QDialogButtonBox QPushButton {"
        "  background: #3C3C3C; color: #CCCCCC; border: 1px solid #3C3C3C;"
        "  border-radius: 3px; padding: 5px 18px; font-size: 12px; min-width: 60px;"
        "}"
        "QDialogButtonBox QPushButton:default { background: #0E639C; border-color: #007ACC; color: #FFFFFF; }"
    );

    auto *root = new QVBoxLayout(&dlg);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(8);

    auto addStat = [&](const QString &key, const QString &value) {
        auto *row = new QHBoxLayout();
        auto *keyLbl = new QLabel(key, &dlg);
        keyLbl->setObjectName("statKey");
        auto *valLbl = new QLabel(value, &dlg);
        valLbl->setObjectName("statVal");
        valLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(keyLbl);
        row->addStretch();
        row->addWidget(valLbl);
        root->addLayout(row);
    };

    addStat("Pages",      QString::number(pageCount));
    addStat("Scenes",     QString::number(sceneCount));
    addStat("Characters", QString::number(characters.size()));
    addStat("Words",      QString::number(wordCount));
    addStat("Est. Runtime",
            runtimeMins == 1 ? "~1 min" : QString("~%1 min").arg(runtimeMins));

    root->addSpacing(8);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);

    dlg.exec();
}

// ---------------------------------------------------------------------------
// Page view creation
// ---------------------------------------------------------------------------
void MainWindow::createPageView()
{
    PageView *page = new PageView();
    m_currentPage = page;
    m_currentFilePath.clear();
    m_isDirty = false;

    page->setZoomSteps(m_persistedZoomSteps);

    QScrollArea *scroll = new QScrollArea();
    scroll->setObjectName("editorScrollArea");
    scroll->setWidget(page);
    scroll->setWidgetResizable(true);
    scroll->setBackgroundRole(QPalette::Dark);
    scroll->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    m_typePanel->setPageView(page);
    m_outlinePanel->setEditor(page->editor());
    m_charactersPanel->setEditor(page->editor());

    QWidget *editorContainer = new QWidget();
    QVBoxLayout *containerLayout = new QVBoxLayout(editorContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    FindBar *findBar = new FindBar(editorContainer);
    findBar->hide();
    containerLayout->addWidget(findBar, 0);
    containerLayout->addWidget(scroll, 1);
    m_currentFindBar = findBar;

    m_stack->addWidget(editorContainer);
    m_stack->setCurrentWidget(editorContainer);

    // Element type sync
    connect(page->editor(), &ScriptEditor::elementChanged, m_typePanel, &ElementTypePanel::setCurrentType);
    connect(m_typePanel, &ElementTypePanel::typeSelected, page->editor(), &ScriptEditor::applyFormat);

    // Status bar
    connect(page->editor(), &ScriptEditor::elementChanged, this,
            [this](ScriptEditor::ElementType t){ updateElementStatus(static_cast<int>(t)); });
    connect(page, &PageView::pageCountChanged, this, &MainWindow::updatePageStatus);
    updateElementStatus(static_cast<int>(ScriptEditor::SceneHeading));
    updatePageStatus(page->pageCount());

    // Cursor status (debounced)
    connect(page->editor(), &QTextEdit::cursorPositionChanged, this, [this] {
        m_cursorUpdateTimer->start();
    });
    // Also update on content change (word count changes)
    connect(page->editor()->document(), &QTextDocument::contentsChanged, this, [this] {
        m_cursorUpdateTimer->start();
    });

    // Undo/redo from custom stack
    connect(page->editor(), &ScriptEditor::undoAvailableChanged, m_undoAction, &QAction::setEnabled);
    connect(page->editor(), &ScriptEditor::redoAvailableChanged, m_redoAction, &QAction::setEnabled);
    m_undoAction->setEnabled(false);
    m_redoAction->setEnabled(false);

    // Dirty flag
    connect(page->editor()->document(), &QTextDocument::contentsChanged, this, [this] {
        setDirty(true);
    });

    // Find bar
    connect(findBar, &FindBar::queryChanged, page->editor(), &ScriptEditor::setFindQuery);
    connect(findBar, &FindBar::optionsChanged, page->editor(), &ScriptEditor::setFindOptions);
    connect(findBar, &FindBar::findNextRequested, page->editor(), &ScriptEditor::findNext);
    connect(findBar, &FindBar::findPreviousRequested, page->editor(), &ScriptEditor::findPrevious);
    connect(findBar, &FindBar::closeRequested, this, [findBar, editor = page->editor()] {
        findBar->hide();
        editor->setFocus();
    });
    connect(page->editor(), &ScriptEditor::findResultsChanged, findBar, [findBar](int activeIndex, int totalMatches) {
        findBar->setMatchStatus(activeIndex, totalMatches);
    });

    // Replace
    connect(findBar, &FindBar::replaceRequested, page->editor(), &ScriptEditor::replaceCurrent);
    connect(findBar, &FindBar::replaceAllRequested, this, [this, findBar](const QString &, const QString &replacement) {
        if (!m_currentPage) return;
        int n = m_currentPage->editor()->replaceAll(replacement);
        findBar->setReplaceStatus(n);
    });

    // Enable actions
    m_saveAction->setEnabled(true);
    m_saveAsAction->setEnabled(true);
    m_titlePageAction->setEnabled(true);
    m_exportFdxAction->setEnabled(true);
    m_exportFountainAction->setEnabled(true);
    m_exportPdfAction->setEnabled(true);
    m_findAction->setEnabled(true);
    m_findNextAction->setEnabled(true);
    m_findPreviousAction->setEnabled(true);
    m_spellcheckAction->setEnabled(true);
    m_spellcheckAction->setChecked(page->editor()->spellcheckEnabled());
    m_zoomInAction->setEnabled(true);
    m_zoomOutAction->setEnabled(true);
    m_resetZoomAction->setEnabled(true);
    m_statsAction->setEnabled(true);

    // Show docks
    m_elementDock->show();
    m_outlineDock->show();
    m_charactersDock->show();
    tabifyDockWidget(m_outlineDock, m_charactersDock);
    m_outlineDock->raise();
    normalizeDockTabBars();
    applySidebarDefaultWidth();
    applyBalancedSidebarSplit();

    page->editor()->setFocus();
    updateWindowTitle();
    updateCursorStatus();
}

// ---------------------------------------------------------------------------
// Dirty flag / title
// ---------------------------------------------------------------------------
void MainWindow::setDirty(bool dirty)
{
    if (m_isDirty == dirty) return;
    m_isDirty = dirty;
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    QString name = m_currentFilePath.isEmpty()
        ? "Untitled"
        : QFileInfo(m_currentFilePath).fileName();
    setWindowTitle(QString("ScreenQt \u2014 %1%2").arg(name, m_isDirty ? "*" : ""));
}

// ---------------------------------------------------------------------------
// Save helpers
// ---------------------------------------------------------------------------
bool MainWindow::doSave()
{
    if (!m_currentPage) return false;

    if (m_currentFilePath.isEmpty()) {
        QString filePath = QFileDialog::getSaveFileName(this, "Save Screenplay", "", "ScreenQt Files (*.sqt)");
        if (filePath.isEmpty()) return false;
        m_currentFilePath = filePath;
    }

    if (m_currentPage->saveToFile(m_currentFilePath)) {
        addRecentFile(m_currentFilePath);
        setDirty(false);
        return true;
    }

    QMessageBox::warning(this, "Save Error", "Failed to save screenplay file.");
    return false;
}

bool MainWindow::maybePromptSave()
{
    if (!m_isDirty) return true;

    auto btn = QMessageBox::question(
        this, "Unsaved Changes",
        "Save changes before continuing?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    if (btn == QMessageBox::Save)   return doSave();
    if (btn == QMessageBox::Cancel) return false;
    return true; // Discard
}

// ---------------------------------------------------------------------------
// Close event
// ---------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!maybePromptSave()) {
        event->ignore();
        return;
    }
    if (m_currentPage) {
        m_persistedZoomSteps = m_currentPage->zoomSteps();
    }
    saveSettings();
    event->accept();
}

void MainWindow::saveSettings()
{
    QSettings settings("ScreenQt", "ScreenQt");
    settings.setValue("editor/zoomSteps", m_persistedZoomSteps);
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState(kLayoutStateVersion));
    settings.setValue("recentFiles", m_recentFiles);
}

// ---------------------------------------------------------------------------
// Dock / layout helpers
// ---------------------------------------------------------------------------
void MainWindow::normalizeDockTabBars()
{
    const auto tabBars = findChildren<QTabBar *>();
    for (QTabBar *tabBar : tabBars) {
        tabBar->setElideMode(Qt::ElideNone);
        tabBar->setUsesScrollButtons(false);
        tabBar->setExpanding(true);
    }
}

void MainWindow::applyBalancedSidebarSplit()
{
    QList<QDockWidget *> docks{m_elementDock, m_outlineDock};
    const int half = qMax(1, height() / 2);
    resizeDocks(docks, {half, half}, Qt::Vertical);
}

void MainWindow::applySidebarDefaultWidth()
{
    QList<QDockWidget *> docks{m_elementDock, m_outlineDock, m_charactersDock};
    const int w = UiSpacing::SidebarWidth;
    resizeDocks(docks, {w, w, w}, Qt::Horizontal);
}

void MainWindow::applyDefaultPanelLayout()
{
    for (QDockWidget *d : {m_elementDock, m_outlineDock, m_charactersDock}) {
        if (d->isFloating()) d->setFloating(false);
    }

    addDockWidget(Qt::RightDockWidgetArea, m_elementDock);
    addDockWidget(Qt::RightDockWidgetArea, m_outlineDock);
    addDockWidget(Qt::RightDockWidgetArea, m_charactersDock);
    splitDockWidget(m_elementDock, m_outlineDock, Qt::Vertical);
    tabifyDockWidget(m_outlineDock, m_charactersDock);
    m_outlineDock->raise();
    normalizeDockTabBars();
    applySidebarDefaultWidth();
    applyBalancedSidebarSplit();

    m_elementDock->show();
    m_outlineDock->show();
    m_charactersDock->show();
}
