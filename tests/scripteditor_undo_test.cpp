#include <QTest>
#include <QObject>
#include <QCoreApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QTextCursor>
#include <QTextBlock>
#include "scripteditor.h"

class ScriptEditorUndoTests : public QObject {
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
    void undoPerWordWhitespaceAndPunctuation() {
        ScriptEditor editor;
        focusEditor(&editor);

        QTest::keyClicks(&editor, "hello world");
        QCOMPARE(editor.toPlainText().trimmed(), QString("HELLO WORLD"));

        QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText().trimmed(), QString("HELLO"));

        QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        QVERIFY(editor.toPlainText().trimmed().isEmpty());

        QTest::keyClick(&editor, Qt::Key_Y, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText().trimmed(), QString("HELLO"));

        QTest::keyClick(&editor, Qt::Key_Y, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText().trimmed(), QString("HELLO WORLD"));

        editor.clear();
        QTest::keyClicks(&editor, "hello,world");
        QCOMPARE(editor.toPlainText().trimmed(), QString("HELLO,WORLD"));

        QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText().trimmed(), QString("HELLO,"));

        QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        QVERIFY(editor.toPlainText().trimmed().isEmpty());
    }

    void undoPasteIsSingleStep() {
        ScriptEditor editor;
        focusEditor(&editor);

        QGuiApplication::clipboard()->setText("alpha beta");
        QTest::keyClick(&editor, Qt::Key_V, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText().trimmed(), QString("alpha beta"));

        QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        QVERIFY(editor.toPlainText().trimmed().isEmpty());

        QTest::keyClick(&editor, Qt::Key_Y, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText().trimmed(), QString("alpha beta"));
    }

    void undoCutIsSingleStep() {
        ScriptEditor editor;
        focusEditor(&editor);

        QTest::keyClicks(&editor, "alpha beta");
        QTextCursor c = editor.textCursor();
        c.setPosition(0);
        c.setPosition(6, QTextCursor::KeepAnchor); // "alpha "
        editor.setTextCursor(c);

        QTest::keyClick(&editor, Qt::Key_X, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText().trimmed(), QString("BETA"));

        QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText().trimmed(), QString("ALPHA BETA"));
    }

    void undoBulkDeleteIsSingleStep() {
        ScriptEditor editor;
        focusEditor(&editor);

        QTest::keyClicks(&editor, "hello world");
        QTextCursor c = editor.textCursor();
        c.setPosition(0);
        c.setPosition(6, QTextCursor::KeepAnchor); // "hello "
        editor.setTextCursor(c);

        QTest::keyClick(&editor, Qt::Key_Backspace);
        QCOMPARE(editor.toPlainText().trimmed(), QString("WORLD"));

        QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText().trimmed(), QString("HELLO WORLD"));
    }

    void undoMidWordEditIsSingleStep() {
        ScriptEditor editor;
        focusEditor(&editor);

        QTest::keyClicks(&editor, "hello");
        QTest::keyClick(&editor, Qt::Key_Left);
        QTest::keyClick(&editor, Qt::Key_Left);
        QTest::keyClicks(&editor, "XY");
        QCOMPARE(editor.toPlainText().trimmed(), QString("HELXYLO"));

        QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText().trimmed(), QString("HELLO"));
    }

    void undoBackspaceMergesWordDeletes() {
        ScriptEditor editor;
        focusEditor(&editor);

        QTest::keyClicks(&editor, "hello,world");
        for (int i = 0; i < 5; ++i) {
            QTest::keyClick(&editor, Qt::Key_Backspace);
        }
        QCOMPARE(editor.toPlainText().trimmed(), QString("HELLO,"));

        QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText().trimmed(), QString("HELLO,WORLD"));
    }

    void tabFormatUndoIsSingleStep() {
        ScriptEditor editor;
        focusEditor(&editor);

        int initialState = editor.textCursor().block().userState();
        QTest::keyClick(&editor, Qt::Key_Tab);
        int afterState = editor.textCursor().block().userState();
        QVERIFY2(afterState != initialState, "Tab should change the element type");

        QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(editor.textCursor().block().userState(), initialState);
    }

    void undoFormatChangeRestoresOriginalCapitalization() {
        ScriptEditor editor;
        focusEditor(&editor);

        // Type in Action format (mixed case)
        QTest::keyClicks(&editor, "Test");
        QString originalText = editor.toPlainText().trimmed();
        QCOMPARE(originalText, QString("TEST"));

        // Tab to uppercase format
        QTest::keyClick(&editor, Qt::Key_Tab);
        
        // Undo should restore original text with original capitalization
        QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        QString afterUndo = editor.toPlainText().trimmed();
        QCOMPARE(afterUndo, QString("TEST"));
    }
};

QTEST_MAIN(ScriptEditorUndoTests)
#include "scripteditor_undo_test.moc"
