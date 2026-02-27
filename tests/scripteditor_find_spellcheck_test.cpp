#include <QCoreApplication>
#include <QObject>
#include <QTest>

#include "scripteditor.h"

class ScriptEditorFindSpellcheckTests : public QObject {
    Q_OBJECT

private:
    void focusEditor(QWidget *widget)
    {
        widget->show();
        widget->raise();
        widget->activateWindow();
        const bool exposed = QTest::qWaitForWindowExposed(widget);
        Q_UNUSED(exposed);
        widget->setFocus();
        QCoreApplication::processEvents();
    }

    void waitForSpellcheck()
    {
        QTest::qWait(320);
        QCoreApplication::processEvents();
    }

private slots:
    void findCountsMatchesCaseInsensitiveByDefault()
    {
        ScriptEditor editor;
        focusEditor(&editor);

        editor.setPlainText("Hello world\nhello again\nHELLO there");
        editor.setFindQuery("hello");

        QCOMPARE(editor.findMatchCount(), 3);
        QCOMPARE(editor.activeFindMatchIndex(), 0);
    }

    void findHonorsCaseSensitivityAndWholeWord()
    {
        ScriptEditor editor;
        focusEditor(&editor);

        editor.setPlainText("he HE hero the\nHE");
        editor.setFindQuery("he");
        QCOMPARE(editor.findMatchCount(), 5);

        editor.setFindOptions(true, false);
        QCOMPARE(editor.findMatchCount(), 3);

        editor.setFindOptions(true, true);
        QCOMPARE(editor.findMatchCount(), 1);

        editor.setFindOptions(false, true);
        QCOMPARE(editor.findMatchCount(), 3);
    }

    void findNextAndPreviousNavigateMatches()
    {
        ScriptEditor editor;
        focusEditor(&editor);

        editor.setPlainText("alpha beta alpha gamma alpha");
        editor.setFindQuery("alpha");

        QCOMPARE(editor.findMatchCount(), 3);
        QCOMPARE(editor.activeFindMatchIndex(), 0);

        QVERIFY(editor.findNext());
        QCOMPARE(editor.activeFindMatchIndex(), 1);

        QVERIFY(editor.findNext());
        QCOMPARE(editor.activeFindMatchIndex(), 2);

        QVERIFY(editor.findPrevious());
        QCOMPARE(editor.activeFindMatchIndex(), 1);
    }

    void spellcheckDetectsMisspellingsAndCanBeDisabled()
    {
        ScriptEditor editor;
        focusEditor(&editor);

        editor.setSpellcheckEnabled(true);
        editor.setPlainText("This sentnce has erors.");
        waitForSpellcheck();

        QVERIFY2(editor.spellcheckMisspellingCount() >= 2,
                 "Expected at least 2 misspellings in test sentence");

        editor.setSpellcheckEnabled(false);
        QCoreApplication::processEvents();
        QCOMPARE(editor.spellcheckMisspellingCount(), 0);
    }

    void spellcheckProvidesSuggestionsForCommonTypos()
    {
        ScriptEditor editor;
        focusEditor(&editor);

        const QStringList suggestions = editor.spellcheckSuggestions("wrld");
        QVERIFY2(!suggestions.isEmpty(), "Expected non-empty suggestions for 'wrld'");
    }
};

QTEST_MAIN(ScriptEditorFindSpellcheckTests)
#include "scripteditor_find_spellcheck_test.moc"
