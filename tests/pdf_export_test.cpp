#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTest>

#include "documentsettings.h"
#include "pageview.h"
#include "scripteditor.h"

#if defined(SCREENQT_HAS_QTPDF)
#include <QPdfDocument>
#endif

class PdfExportTests : public QObject {
    Q_OBJECT

private:
    int countPagesFromPdfObjects(const QString &filePath)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return -1;
        }

        const QByteArray data = file.readAll();
        const QString text = QString::fromLatin1(data);
        const QRegularExpression pageObjectRegex("/Type\\s*/Page\\b");

        int count = 0;
        QRegularExpressionMatchIterator it = pageObjectRegex.globalMatch(text);
        while (it.hasNext()) {
            it.next();
            ++count;
        }
        return count;
    }

private slots:
    void exportedPdfPageCountMatchesViewPageCount()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString filePath = tempDir.filePath("page_count_match.pdf");

        PageView view;
        view.editor()->clear();

        QTextCursor cursor(view.editor()->document());
        for (int i = 0; i < 220; ++i) {
            cursor.insertText(QString("LINE %1").arg(i));
            if (i < 219) {
                cursor.insertText("\n");
            }
        }
        QCoreApplication::processEvents();

        const int expectedBodyPages = view.pageCount();
        QVERIFY(expectedBodyPages >= 2);

        DocumentSettings settings;
        settings.hasTitlePage = false;
        settings.pageNumbering.enabled = true;
        view.setDocumentSettings(settings);

        QVERIFY(view.exportToPdf(filePath));

        const int exportedPages = countPagesFromPdfObjects(filePath);
        QVERIFY(exportedPages > 0);
        QCOMPARE(exportedPages, expectedBodyPages);
    }

    void titlePageIncreasesExportedPageCountByOne()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString filePath = tempDir.filePath("title_page_offset.pdf");

        PageView view;
        view.editor()->setPlainText("INT. OFFICE - DAY\nA quiet room.");
        QCoreApplication::processEvents();

        const int bodyPages = view.pageCount();
        QVERIFY(bodyPages >= 1);

        DocumentSettings settings;
        settings.hasTitlePage = true;
        settings.titlePage.title = "PARITY TEST";
        settings.titlePage.author = "QA WRITER";
        settings.pageNumbering.enabled = true;
        settings.pageNumbering.numberTitlePage = false;
        settings.pageNumbering.startNumber = 1;
        view.setDocumentSettings(settings);

        QVERIFY(view.exportToPdf(filePath));

        const int exportedPages = countPagesFromPdfObjects(filePath);
        QCOMPARE(exportedPages, bodyPages + 1);
    }

    void longDialogueExportKeepsMultiPageParity()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString filePath = tempDir.filePath("dialogue_parity.pdf");

        PageView view;
        view.editor()->clear();

        QTextCursor cursor(view.editor()->document());
        cursor.insertText("JOE");
        QTextBlock charBlock = cursor.block();
        charBlock.setUserState(static_cast<int>(ScriptEditor::CharacterName));

        cursor.insertText("\n");
        const QString longDialogue = QString("the line continues ").repeated(900);
        cursor.insertText(longDialogue);
        QTextBlock dialogueBlock = cursor.block();
        dialogueBlock.setUserState(static_cast<int>(ScriptEditor::Dialogue));

        view.editor()->formatDocument();
        QCoreApplication::processEvents();

        QVERIFY2(view.pageCount() >= 2, "Expected multi-page dialogue in view");
        QVERIFY2(!view.continuationMarkers().isEmpty(), "Expected continuation markers in view");

        QVERIFY(view.exportToPdf(filePath));
        const int exportedPages = countPagesFromPdfObjects(filePath);
        QCOMPARE(exportedPages, view.pageCount());
    }

    void renderedPdfLooksLikeDocumentPageWhenQtPdfAvailable()
    {
#if defined(SCREENQT_HAS_QTPDF)
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString filePath = tempDir.filePath("render_parity.pdf");

        PageView view;
        view.editor()->setPlainText("INT. LAB - NIGHT\nA machine hums.");
        QVERIFY(view.exportToPdf(filePath));

        QPdfDocument pdf;
        QCOMPARE(pdf.load(filePath), QPdfDocument::NoError);
        QVERIFY(pdf.pageCount() >= 1);

        const QImage pageImage = pdf.render(0, QSize(1200, 1600));
        QVERIFY(!pageImage.isNull());

        const QColor center = pageImage.pixelColor(pageImage.width() / 2, pageImage.height() / 2);
        QVERIFY2(center.lightness() > 120, "Expected light paper-like center area in exported PDF render");
#else
        QSKIP("QtPdf module unavailable; raster parity check skipped.");
#endif
    }
};

QTEST_MAIN(PdfExportTests)
#include "pdf_export_test.moc"
