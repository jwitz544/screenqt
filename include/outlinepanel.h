#pragma once

#include <QWidget>

class QListWidget;
class QListWidgetItem;
class ScriptEditor;

class OutlinePanel : public QWidget {
    Q_OBJECT
public:
    explicit OutlinePanel(QWidget *parent = nullptr);

    void setEditor(ScriptEditor *editor);

private slots:
    void refreshOutline();
    void goToScene(QListWidgetItem *item);
    void syncSelectionToCursor();

private:
    ScriptEditor *m_editor = nullptr;
    QListWidget *m_sceneList = nullptr;
    bool m_updatingSelection = false;
};
