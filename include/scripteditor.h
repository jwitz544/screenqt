#pragma once
#include <QTextEdit>
#include <QFont>
#include <QScreen>
#include <QGuiApplication>
#include <QUndoStack>
#include <QTextCursor>
#include <QVector>
#include "spellcheckservice.h"
#include <memory>

class QCompleter;
class QStringListModel;
class QContextMenuEvent;
class QTimer;

class ScriptEditor : public QTextEdit {
    Q_OBJECT
public:
    enum ElementType {
        SceneHeading = 0,
        Action,
        CharacterName,
        Dialogue,
        Parenthetical,
        Shot,
        Transition,
        ElementCount
    };

    enum class UndoGroupType {
        Word,
        Punctuation,
        Whitespace,
        Bulk,
        Other
    };

    explicit ScriptEditor(QWidget *parent = nullptr);
    
    void applyFormat(ElementType type);
    void formatDocument(); // Apply formatting to all blocks based on their userState
    void setFindQuery(const QString &query);
    void setFindOptions(bool caseSensitive, bool wholeWord);
    bool findNext();
    bool findPrevious();
    int findMatchCount() const;
    int activeFindMatchIndex() const;
    void setSpellcheckEnabled(bool enabled);
    bool spellcheckEnabled() const;
    int spellcheckMisspellingCount() const;
    QStringList spellcheckSuggestions(const QString &word) const;


public slots:
    void clear();
    void undo();
    void redo();
    void zoomInText();
    void zoomOutText();
    void resetZoom();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    struct Range {
        int start = 0;
        int length = 0;
    };

    ElementType nextType(ElementType t) const;
    ElementType previousType(ElementType t) const;
    ElementType currentElement() const;
    double dpiX() const;
    double inchToPx(double inches) const;
    QStringList collectCharacterNames() const;
    QStringList sceneHeadingCompletions() const;
    QStringList completionCandidates(ElementType type, const QString &prefix) const;
    void showCompletionPopup(ElementType type, const QString &prefix);
    void hideCompletionPopup();
    void insertChosenCompletion(const QString &completion);
    QString resolveInlineCompletion(ElementType type, const QString &prefix) const;
    QTextDocument::FindFlags currentFindFlags() const;
    void rebuildFindMatches();
    void applyFindMatchAtIndex(int index);
    void refreshSpellcheck();
    void scheduleSpellcheckRefresh();
    QString wordUnderCursor(QTextCursor *wordCursor = nullptr) const;
    void replaceRangeText(int start, int length, const QString &replacement);
    void refreshExtraSelections();

    UndoGroupType classifyChar(QChar ch) const;
    bool isNavigationKey(QKeyEvent *e) const;
    void applyFormatDirect(ElementType type);
    void buildFormats(ElementType type, QTextBlockFormat &bf, QTextCharFormat &cf) const;

    QUndoStack m_undoStack;
    bool m_suppressUndo = false;
    int m_zoomSteps = 0;
    QCompleter *m_completer = nullptr;
    QStringListModel *m_completionModel = nullptr;
    QString m_completionPrefix;
    ElementType m_completionType = Action;
    QString m_findQuery;
    bool m_findCaseSensitive = false;
    bool m_findWholeWord = false;
    QVector<Range> m_findMatches;
    int m_activeFindIndex = -1;
    bool m_spellcheckEnabled = true;
    std::unique_ptr<ISpellChecker> m_spellChecker;
    QVector<Range> m_spellingRanges;
    QTimer *m_spellcheckTimer = nullptr;

signals:
    void elementChanged(ElementType type);
    void findResultsChanged(int activeIndex, int totalMatches);
};
