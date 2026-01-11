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
#include "pageview.h"
#include "scripteditor.h"
#include "startscreen.h"
#include "elementtypepanel.h"

int main(int argc, char *argv[])
{
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
    
    // Create start screen
    StartScreen *startScreen = new StartScreen();
    stack->addWidget(startScreen);
    
    // Function to create page view with scroll area
    QString currentFilePath;
    
    auto createPageView = [&window, stack, &currentPage, &currentFilePath, saveAction, saveAsAction, exportPdfAction]() -> PageView* {
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
        
        stack->setCurrentWidget(editorContainer);
        page->editor()->setFocus();
        saveAction->setEnabled(true);
        saveAsAction->setEnabled(true);
        exportPdfAction->setEnabled(true);
        qDebug() << "[Main] Switched to editor view";
        
        return page;
    };
    
    // Connect start screen signals
    QObject::connect(startScreen, &StartScreen::newDocument, [&]() {
        qDebug() << "[Main] New document requested";
        PageView *page = createPageView();
        page->loadSampleText(); // Start with sample text for now
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
