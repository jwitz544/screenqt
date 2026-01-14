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
#include "pageview.h"
#include "scripteditor.h"
#include "startscreen.h"
#include "elementtypepanel.h"

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
    qDebug() << "[Main] Window created and resized to 900x700";

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
    
    QAction *exportPdfAction = fileMenu->addAction("Export to &PDF...");
    exportPdfAction->setEnabled(false);
    
    // Edit menu with undo/redo
    QMenu *editMenu = menuBar->addMenu("&Edit");
    
    QAction *undoAction = editMenu->addAction("&Undo");
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setEnabled(false);
    
    QAction *redoAction = editMenu->addAction("&Redo");
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setEnabled(false);
    
    // Create start screen
    StartScreen *startScreen = new StartScreen();
    stack->addWidget(startScreen);
    
    // Function to create page view with scroll area
    QString currentFilePath;
    
    auto createPageView = [&window, stack, &currentPage, &currentFilePath, saveAction, saveAsAction, exportPdfAction, undoAction, redoAction]() -> PageView* {
        PageView *page = new PageView();
        currentPage = page;
        currentFilePath.clear();
        qDebug() << "[Main] PageView created";
        
        QScrollArea *scroll = new QScrollArea();
        scroll->setWidget(page);
        scroll->setWidgetResizable(true);
        scroll->setBackgroundRole(QPalette::Dark);
        scroll->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        
        // Create element type panel
        ElementTypePanel *typePanel = new ElementTypePanel();
        
        // Create container widget for editor + panel
        QWidget *editorContainer = new QWidget();
        QHBoxLayout *containerLayout = new QHBoxLayout(editorContainer);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(10);
        containerLayout->addWidget(scroll, 1);
        containerLayout->addWidget(typePanel, 0, Qt::AlignRight | Qt::AlignTop);
        
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
        exportPdfAction->setEnabled(true);
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
            QString filePath = QFileDialog::getSaveFileName(&window, "Save Screenplay", "", "Screenplay Files (*.sqt)");
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
        QString filePath = QFileDialog::getOpenFileName(&window, "Open Screenplay", "", "Screenplay Files (*.sqt)");
        if (filePath.isEmpty()) return;
        qDebug() << "[Main] Open document requested:" << filePath;
        PageView *page = createPageView();
        if (page->loadFromFile(filePath)) {
            currentFilePath = filePath;
        } else {
            QMessageBox::warning(&window, "Load Error", "Failed to load screenplay file.");
        }
    });
    
    QObject::connect(saveAsAction, &QAction::triggered, [&]() {
        if (!currentPage) return;
        
        QString filePath = QFileDialog::getSaveFileName(&window, "Save Screenplay As", "", "Screenplay Files (*.sqt)");
        if (filePath.isEmpty()) return;
        
        if (currentPage->saveToFile(filePath)) {
            currentFilePath = filePath;
            qDebug() << "[Main] Saved as:" << currentFilePath;
        } else {
            QMessageBox::warning(&window, "Save Error", "Failed to save screenplay file.");
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
    
    stack->setCurrentWidget(startScreen);
    qDebug() << "[Main] Start screen displayed";

    window.show();
    qDebug() << "[Main] Window shown, entering event loop";
    
    return app.exec();
}
