#include <QCoreApplication>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include "documentsettings.h"
#include "pageview.h"
#include "scripteditor.h"

class DocumentSettingsTests : public QObject {
    Q_OBJECT

private slots:
    void sqtRoundTripPreservesDocumentSettings()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString filePath = tempDir.filePath("metadata_roundtrip.sqt");

        PageView original;
        original.editor()->setPlainText("INT. OFFICE\nA desk covered in notes.");

        DocumentSettings settings;
        settings.hasTitlePage = true;
        settings.titlePage.title = "Deep Night";
        settings.titlePage.author = "Jane Doe";
        settings.titlePage.credit = "Written by";
        settings.titlePage.contact = "jane@example.com";
        settings.titlePage.draftDate = "2026-02-26";
        settings.pageNumbering.enabled = true;
        settings.pageNumbering.startNumber = 3;
        settings.pageNumbering.numberTitlePage = false;
        original.setDocumentSettings(settings);

        QVERIFY(original.saveToFile(filePath));

        PageView loaded;
        QVERIFY(loaded.loadFromFile(filePath));

        const DocumentSettings loadedSettings = loaded.documentSettings();
        QVERIFY(loadedSettings.hasTitlePage);
        QCOMPARE(loadedSettings.titlePage.title, settings.titlePage.title);
        QCOMPARE(loadedSettings.titlePage.author, settings.titlePage.author);
        QCOMPARE(loadedSettings.titlePage.credit, settings.titlePage.credit);
        QCOMPARE(loadedSettings.titlePage.contact, settings.titlePage.contact);
        QCOMPARE(loadedSettings.titlePage.draftDate, settings.titlePage.draftDate);
        QCOMPARE(loadedSettings.pageNumbering.enabled, settings.pageNumbering.enabled);
        QCOMPARE(loadedSettings.pageNumbering.startNumber, settings.pageNumbering.startNumber);
        QCOMPARE(loadedSettings.pageNumbering.numberTitlePage, settings.pageNumbering.numberTitlePage);
    }

    void exportPdfWithTitlePageAndNumberingOptions()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString filePath = tempDir.filePath("title_page_export.pdf");

        PageView view;
        view.editor()->setPlainText("INT. LAB - NIGHT\nA prototype hums to life.");

        DocumentSettings settings;
        settings.hasTitlePage = true;
        settings.titlePage.title = "Prototype";
        settings.titlePage.author = "Alex Writer";
        settings.titlePage.credit = "Written by";
        settings.pageNumbering.enabled = true;
        settings.pageNumbering.startNumber = 1;
        settings.pageNumbering.numberTitlePage = false;
        view.setDocumentSettings(settings);

        QVERIFY(view.exportToPdf(filePath));

        QFileInfo info(filePath);
        QVERIFY(info.exists());
        QVERIFY(info.size() > 0);
    }
};

QTEST_MAIN(DocumentSettingsTests)
#include "document_settings_test.moc"
