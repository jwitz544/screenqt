#include "startscreen.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QDebug>

StartScreen::StartScreen(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("startScreen");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);
    layout->setAlignment(Qt::AlignCenter);
    
    QLabel *title = new QLabel("ScreenQt", this);
    title->setObjectName("startTitle");
    QFont titleFont = title->font();
    titleFont.setPointSize(32);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    
    QPushButton *newBtn = new QPushButton("New Screenplay", this);
    newBtn->setObjectName("startPrimaryButton");
    newBtn->setMinimumSize(200, 50);
    
    QPushButton *loadBtn = new QPushButton("Load Screenplay", this);
    loadBtn->setObjectName("startSecondaryButton");
    loadBtn->setMinimumSize(200, 50);
    
    layout->addWidget(title);
    layout->addSpacing(24);
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
        "ScreenQt Files (*.sqt);;All Files (*)"
    );
    
    if (!filePath.isEmpty()) {
        qDebug() << "[StartScreen] Selected file:" << filePath;
        emit loadDocument(filePath);
    }
}
