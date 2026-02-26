#pragma once

#include <QWidget>

class QListWidget;
class QListWidgetItem;
class QLabel;
class ScriptEditor;

class CharactersPanel : public QWidget {
    Q_OBJECT
public:
    explicit CharactersPanel(QWidget *parent = nullptr);

    void setEditor(ScriptEditor *editor);

private slots:
    void refreshCharacters();
    void goToCharacter(QListWidgetItem *item);

private:
    ScriptEditor *m_editor = nullptr;
    QListWidget *m_characterList = nullptr;
    QLabel *m_characterCountLabel = nullptr;
};
