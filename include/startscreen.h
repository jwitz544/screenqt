#pragma once
#include <QWidget>
#include <QPushButton>

class StartScreen : public QWidget {
    Q_OBJECT
public:
    explicit StartScreen(QWidget *parent = nullptr);

signals:
    void newDocument();
    void loadDocument(const QString &filePath);

private:
    void onNew();
    void onLoad();
};
