#include <QTest>
#include <QObject>
#include <QCoreApplication>
#include <QTextCursor>
#include <QTextBlock>
#include "scripteditor.h"

class ScriptEditorFormatTests : public QObject {
    Q_OBJECT

private:
    void focusEditor(QWidget *widget) {
        widget->show();
        widget->raise();
        widget->activateWindow();
        const bool exposed = QTest::qWaitForWindowExposed(widget);
        Q_UNUSED(exposed);
        widget->setFocus();
        QCoreApplication::processEvents();
    }

private slots:
    void formatChangePreservesTextWithContent() {
        ScriptEditor editor;
        focusEditor(&editor);

        QTest::keyClicks(&editor, "Test");
        QString originalText = editor.toPlainText();
        QCOMPARE(originalText, QString("TEST"));

        // Change format via tab
        QTest::keyClick(&editor, Qt::Key_Tab);
        QString afterTab = editor.toPlainText();
        QVERIFY2(!afterTab.isEmpty(), "Text should not disappear after tab");
        QVERIFY2(afterTab.contains("Test", Qt::CaseInsensitive), "Text content should be preserved");
    }

    void formatChangeToUppercaseAppliesCapitalization() {
        ScriptEditor editor;
        focusEditor(&editor);

        // Start with Scene Heading (uppercase)
        QTest::keyClicks(&editor, "hello world");
        QCOMPARE(editor.toPlainText(), QString("HELLO WORLD"));

        // Move to an uppercase element type (Scene Heading / Character / Shot / Transition)
        auto isUppercaseType = [](int state) {
            return state == static_cast<int>(ScriptEditor::SceneHeading)
                || state == static_cast<int>(ScriptEditor::CharacterName)
                || state == static_cast<int>(ScriptEditor::Shot)
                || state == static_cast<int>(ScriptEditor::Transition);
        };

        int safety = 0;
        while (!isUppercaseType(editor.textCursor().block().userState()) && safety < 10) {
            QTest::keyClick(&editor, Qt::Key_Tab);
            ++safety;
        }
        QVERIFY2(isUppercaseType(editor.textCursor().block().userState()), "Expected to reach an uppercase element type");

        // Type new text - should be uppercase
        QTest::keyClicks(&editor, " test");
        QString text = editor.toPlainText();
        QVERIFY2(text.endsWith("TEST") || text.endsWith(" TEST"), 
                 QString("New text in uppercase format should be uppercase, got: %1").arg(text).toUtf8().constData());
    }

    void cycleFormatMultipleTimesPreservesOriginal() {
        ScriptEditor editor;
        focusEditor(&editor);

        QTest::keyClicks(&editor, "MyTest");
        QString originalText = editor.toPlainText();

        // Cycle through several formats
        for (int i = 0; i < 5; ++i) {
            QTest::keyClick(&editor, Qt::Key_Tab);
        }

        // Undo all format changes
        for (int i = 0; i < 5; ++i) {
            QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        }

        QString afterUndos = editor.toPlainText();
        QCOMPARE(afterUndos, originalText);
    }

    void tabCyclesThroughAllFormats() {
        ScriptEditor editor;
        focusEditor(&editor);

        QTest::keyClicks(&editor, "Test");
        
        int initialState = editor.textCursor().block().userState();
        QSet<int> seenStates;
        seenStates.insert(initialState);

        // Cycle through formats until we return to start
        for (int i = 0; i < 10; ++i) {
            QTest::keyClick(&editor, Qt::Key_Tab);
            int currentState = editor.textCursor().block().userState();
            
            if (currentState == initialState) {
                // Should have seen all format types
                QVERIFY2(seenStates.size() == static_cast<int>(ScriptEditor::ElementCount),
                         QString("Expected %1 format types, saw %2")
                         .arg(ScriptEditor::ElementCount)
                         .arg(seenStates.size())
                         .toUtf8().constData());
                return;
            }
            seenStates.insert(currentState);
        }
        
        QFAIL("Tab should cycle back to initial format");
    }

    void shiftTabCyclesBackward() {
        ScriptEditor editor;
        focusEditor(&editor);

        int initialState = editor.textCursor().block().userState();
        QTest::keyClick(&editor, Qt::Key_Tab);
        int nextState = editor.textCursor().block().userState();
        QVERIFY(nextState != initialState);

        QTest::keyClick(&editor, Qt::Key_Backtab, Qt::ShiftModifier);
        int backState = editor.textCursor().block().userState();
        QCOMPARE(backState, initialState);
    }

    void newDocumentStartsAndStaysSceneHeadingOnFirstTyping() {
        ScriptEditor editor;
        focusEditor(&editor);

        QCOMPARE(editor.textCursor().block().userState(), static_cast<int>(ScriptEditor::SceneHeading));

        QTest::keyClick(&editor, Qt::Key_A);
        QCOMPARE(editor.textCursor().block().userState(), static_cast<int>(ScriptEditor::SceneHeading));
    }
};

QTEST_MAIN(ScriptEditorFormatTests)
#include "scripteditor_format_test.moc"
