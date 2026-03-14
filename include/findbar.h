#pragma once

#include <QFrame>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

class FindBar : public QFrame {
    Q_OBJECT
public:
    explicit FindBar(QWidget *parent = nullptr);

    void focusAndSelectAll();
    void setMatchStatus(int currentIndex, int totalMatches);
    void setReplaceStatus(int count);
    QString findQuery() const;

signals:
    void queryChanged(const QString &query);
    void findNextRequested();
    void findPreviousRequested();
    void optionsChanged(bool caseSensitive, bool wholeWord);
    void closeRequested();
    void replaceRequested(const QString &replacement);
    void replaceAllRequested(const QString &findText, const QString &replacement);

private:
    QLineEdit   *m_input = nullptr;
    QPushButton *m_prevButton = nullptr;
    QPushButton *m_nextButton = nullptr;
    QCheckBox   *m_caseCheck = nullptr;
    QCheckBox   *m_wordCheck = nullptr;
    QPushButton *m_closeButton = nullptr;

    QLineEdit   *m_replaceInput = nullptr;
    QPushButton *m_replaceButton = nullptr;
    QPushButton *m_replaceAllButton = nullptr;
    QLabel      *m_replaceStatusLabel = nullptr;
};
