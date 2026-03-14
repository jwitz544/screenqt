#pragma once

#include <QMainWindow>
#include <QStringList>
#include <QString>

class QAction;
class QCloseEvent;
class QDockWidget;
class QLabel;
class QSettings;
class QStackedWidget;
class QTimer;

class CharactersPanel;
class ElementTypePanel;
class FindBar;
class OutlinePanel;
class PageView;
class StartScreen;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupMenus();
    void setupDocks();
    void setupConnections();
    void setupStatusBar();
    void setupAutoSave();
    void createPageView();

    void updateElementStatus(int elementType);
    void updatePageStatus(int pageCount);
    void updateCursorStatus();

    bool doSave();
    bool maybePromptSave();
    void setDirty(bool dirty);
    void updateWindowTitle();
    void saveSettings();

    void doAutoSave();
    void clearAutoSave();
    void checkAutoSaveRecovery(const QString &filePath);

    void addRecentFile(const QString &filePath);
    void loadRecentFiles();

    void showScriptStatistics();
    void openTitlePageDialog();

    void applyDefaultPanelLayout();
    void normalizeDockTabBars();
    void applyBalancedSidebarSplit();
    void applySidebarDefaultWidth();

    // Central widget
    QStackedWidget *m_stack        = nullptr;
    StartScreen    *m_startScreen  = nullptr;
    PageView       *m_currentPage  = nullptr;
    FindBar        *m_currentFindBar = nullptr;

    // State
    QString      m_currentFilePath;
    bool         m_isDirty = false;
    int          m_persistedZoomSteps = 0;
    QStringList  m_recentFiles;

    // Timers
    QTimer *m_autoSaveTimer      = nullptr;
    QTimer *m_cursorUpdateTimer  = nullptr;

    // File actions
    QAction *m_saveAction           = nullptr;
    QAction *m_saveAsAction         = nullptr;
    QAction *m_openAction           = nullptr;
    QAction *m_importFdxAction      = nullptr;
    QAction *m_exportFdxAction      = nullptr;
    QAction *m_exportPdfAction      = nullptr;
    QAction *m_titlePageAction      = nullptr;
    QAction *m_importFountainAction = nullptr;
    QAction *m_exportFountainAction = nullptr;

    // Edit actions
    QAction *m_undoAction         = nullptr;
    QAction *m_redoAction         = nullptr;
    QAction *m_findAction         = nullptr;
    QAction *m_findNextAction     = nullptr;
    QAction *m_findPreviousAction = nullptr;
    QAction *m_spellcheckAction   = nullptr;

    // View / Tools actions
    QAction *m_zoomInAction     = nullptr;
    QAction *m_zoomOutAction    = nullptr;
    QAction *m_resetZoomAction  = nullptr;
    QAction *m_resetLayoutAction = nullptr;
    QAction *m_statsAction      = nullptr;

    // Panels
    ElementTypePanel *m_typePanel       = nullptr;
    OutlinePanel     *m_outlinePanel    = nullptr;
    CharactersPanel  *m_charactersPanel = nullptr;

    // Docks
    QDockWidget *m_elementDock    = nullptr;
    QDockWidget *m_outlineDock    = nullptr;
    QDockWidget *m_charactersDock = nullptr;

    // Status bar labels
    QLabel *m_elementStatusLabel = nullptr;
    QLabel *m_sceneStatusLabel   = nullptr;
    QLabel *m_wordStatusLabel    = nullptr;
    QLabel *m_pageStatusLabel    = nullptr;
};
