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
#include <QDateTime>
#include <QDockWidget>
#include <QList>
#include <QSettings>
#include "pageview.h"
#include "scripteditor.h"
#include "startscreen.h"
#include "elementtypepanel.h"
#include "outlinepanel.h"

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

    QMainWindow window;
    window.setWindowTitle("ScreenQt");
    window.resize(900, 700);
    window.setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks | QMainWindow::GroupedDragging);
    window.setStyleSheet(
        "QMainWindow { background: #eef1f5; }"
        "QMenuBar {"
        "  background: #f8fafc;"
        "  border-bottom: 1px solid #d9dee6;"
        "  padding: 2px 6px;"
        "}"
        "QMenuBar::item {"
        "  spacing: 8px;"
        "  padding: 6px 10px;"
        "  border-radius: 6px;"
        "  color: #1f2937;"
        "}"
        "QMenuBar::item:selected { background: #e8eef8; }"
        "QMenu {"
        "  background: #ffffff;"
        "  border: 1px solid #d0d7de;"
        "  padding: 6px;"
        "}"
        "QMenu::item {"
        "  padding: 6px 24px;"
        "  border-radius: 6px;"
        "}"
        "QMenu::item:selected { background: #e8eef8; }"
        "QDockWidget {"
        "  font-size: 12px;"
        "  titlebar-close-icon: none;"
        "  titlebar-normal-icon: none;"
        "}"
        "QDockWidget::title {"
        "  background: #f8fafc;"
        "  border: 1px solid #d0d7de;"
        "  border-bottom: none;"
        "  color: #374151;"
        "  padding: 6px 10px;"
        "  text-align: left;"
        "}"
    );
    qDebug() << "[Main] Window created and resized to 900x700";

    QSettings settings("ScreenQt", "ScreenQt");
    constexpr int layoutStateVersion = 1;
    int persistedZoomSteps = settings.value("editor/zoomSteps", 0).toInt();

    // Create stacked widget to switch between start screen and editor
    QStackedWidget *stack = new QStackedWidget(&window);
    window.setCentralWidget(stack);
    
    // Pointer to current page (for save functionality)
    PageView *currentPage = nullptr;
    
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

    QDockWidget *elementDock = new QDockWidget("Elements", &window);
    elementDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    elementDock->setWidget(typePanel);

    QDockWidget *outlineDock = new QDockWidget("Outline", &window);
    outlineDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    outlineDock->setWidget(outlinePanel);

    window.addDockWidget(Qt::RightDockWidgetArea, elementDock);
    window.addDockWidget(Qt::RightDockWidgetArea, outlineDock);
    window.splitDockWidget(elementDock, outlineDock, Qt::Vertical);

    elementDock->setMinimumWidth(150);
    outlineDock->setMinimumWidth(150);

    const QByteArray savedGeometry = settings.value("window/geometry").toByteArray();
    if (!savedGeometry.isEmpty()) {
        window.restoreGeometry(savedGeometry);
    }

    const QByteArray savedState = settings.value("window/state").toByteArray();
    if (!savedState.isEmpty()) {
        window.restoreState(savedState, layoutStateVersion);
    }

    elementDock->hide();
    outlineDock->hide();

    auto applyDefaultPanelLayout = [&window, elementDock, outlineDock]() {
        if (elementDock->isFloating()) {
            elementDock->setFloating(false);
        }
        if (outlineDock->isFloating()) {
            outlineDock->setFloating(false);
        }

        window.addDockWidget(Qt::RightDockWidgetArea, elementDock);
        window.addDockWidget(Qt::RightDockWidgetArea, outlineDock);
        window.splitDockWidget(elementDock, outlineDock, Qt::Vertical);

        QList<QDockWidget*> docks{elementDock, outlineDock};
        QList<int> sizes{1, 1};
        window.resizeDocks(docks, sizes, Qt::Vertical);

        elementDock->show();
        outlineDock->show();
    };
    
    // Function to create page view with scroll area
    QString currentFilePath;
    
    auto createPageView = [&window, stack, &currentPage, &currentFilePath, &persistedZoomSteps, saveAction, saveAsAction, exportFdxAction, exportPdfAction, undoAction, redoAction, zoomInAction, zoomOutAction, resetZoomAction, typePanel, outlinePanel, elementDock, outlineDock]() -> PageView* {
        PageView *page = new PageView();
        currentPage = page;
        currentFilePath.clear();
        qDebug() << "[Main] PageView created";
        page->setZoomSteps(persistedZoomSteps);
        
        QScrollArea *scroll = new QScrollArea();
        scroll->setWidget(page);
        scroll->setWidgetResizable(true);
        scroll->setBackgroundRole(QPalette::Dark);
        scroll->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        
        typePanel->setPageView(page);
        outlinePanel->setEditor(page->editor());
        
        // Create container widget for editor + panel
        QWidget *editorContainer = new QWidget();
        QHBoxLayout *containerLayout = new QHBoxLayout(editorContainer);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(0);
        containerLayout->addWidget(scroll, 1);
        
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
        zoomInAction->setEnabled(true);
        zoomOutAction->setEnabled(true);
        resetZoomAction->setEnabled(true);
        elementDock->show();
        outlineDock->show();
        qDebug() << "[Main] Before setting initial undo/redo state: isUndoAvailable=" << page->editor()->document()->isUndoAvailable() << "isRedoAvailable=" << page->editor()->document()->isRedoAvailable();
        undoAction->setEnabled(page->editor()->document()->isUndoAvailable());
        redoAction->setEnabled(page->editor()->document()->isRedoAvailable());
        qDebug() << "[Main] After setting initial undo/redo state: undoAction enabled=" << undoAction->isEnabled() << "redoAction enabled=" << redoAction->isEnabled();
        qDebug() << "[Main] Switched to editor view";
        
        return page;
    };
    
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
