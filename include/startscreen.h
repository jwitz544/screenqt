#pragma once
#include <QStringList>
#include <QWidget>

class QVBoxLayout;

class StartScreen : public QWidget {
    Q_OBJECT
public:
    explicit StartScreen(QWidget *parent = nullptr);

    void setRecentFiles(const QStringList &files);

signals:
    void newDocument();
    void loadDocument(const QString &filePath);

private:
    void onNew();
    void onLoad();
    void buildRecentSection();

    QVBoxLayout  *m_recentLayout   = nullptr;
    QWidget      *m_recentSection  = nullptr;
    QStringList   m_recentFiles;
};
