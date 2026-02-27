#pragma once

#include <QFrame>

class QCheckBox;
class QLineEdit;
class QPushButton;

class FindBar : public QFrame {
    Q_OBJECT
public:
    explicit FindBar(QWidget *parent = nullptr);

    void focusAndSelectAll();
    void setMatchStatus(int currentIndex, int totalMatches);

signals:
    void queryChanged(const QString &query);
    void findNextRequested();
    void findPreviousRequested();
    void optionsChanged(bool caseSensitive, bool wholeWord);
    void closeRequested();

private:
    QLineEdit *m_input = nullptr;
    QPushButton *m_prevButton = nullptr;
    QPushButton *m_nextButton = nullptr;
    QCheckBox *m_caseCheck = nullptr;
    QCheckBox *m_wordCheck = nullptr;
    QPushButton *m_closeButton = nullptr;
};
