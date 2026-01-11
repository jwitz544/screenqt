#include "startscreen.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QDebug>

StartScreen::StartScreen(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignCenter);
    
    QLabel *title = new QLabel("ScreenQt", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(32);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    
    QPushButton *newBtn = new QPushButton("New Screenplay", this);
    newBtn->setMinimumSize(200, 50);
    newBtn->setStyleSheet("font-size: 16px; padding: 10px;");
    
    QPushButton *loadBtn = new QPushButton("Load Screenplay", this);
    loadBtn->setMinimumSize(200, 50);
    loadBtn->setStyleSheet("font-size: 16px; padding: 10px;");
    
    layout->addWidget(title);
    layout->addSpacing(30);
    layout->addWidget(newBtn);
    layout->addWidget(loadBtn);
    layout->addStretch();
    
    connect(newBtn, &QPushButton::clicked, this, &StartScreen::onNew);
    connect(loadBtn, &QPushButton::clicked, this, &StartScreen::onLoad);
    
    qDebug() << "[StartScreen] Created";
}

void StartScreen::onNew()
{
    qDebug() << "[StartScreen] New document requested";
    emit newDocument();
}

void StartScreen::onLoad()
{
    qDebug() << "[StartScreen] Load document requested";
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open Screenplay",
        "",
        "Screenplay Files (*.sqt);;All Files (*)"
    );
    
    if (!filePath.isEmpty()) {
        qDebug() << "[StartScreen] Selected file:" << filePath;
        emit loadDocument(filePath);
    }
}
