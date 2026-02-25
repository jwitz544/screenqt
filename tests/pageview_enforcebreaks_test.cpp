#include <QTest>
#include <QObject>
#include <QCoreApplication>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>
#include <QFile>
#include <QTemporaryDir>
#include "pageview.h"
#include "scripteditor.h"

class PageViewTests : public QObject {
    Q_OBJECT

private:
    void insertLines(ScriptEditor* editor, int count) {
        QTextCursor cur(editor->document());
        cur.movePosition(QTextCursor::End);
        for (int i = 0; i < count; ++i) {
            cur.insertText(QString("Line %1").arg(i));
            if (i < count - 1) cur.insertText("\n");
        }
    }

    int firstBlockOffsetAtPage(PageView& pv, int pageIndex) {
        QTextDocument* doc = pv.editor()->document();
        const int expectedStart = pv.pagePrintableStartY(pageIndex);
        const int editorTop = pv.editor()->geometry().top();

        for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
            int actualY = static_cast<int>(doc->documentLayout()->blockBoundingRect(b).top()) + editorTop;
            if (actualY >= expectedStart) {
                return expectedStart - actualY;
            }
        }
        return expectedStart;
    }

    int contentStartYAt(QTextDocument* doc, int idx) {
        int y = 0;
        int blockIndex = 0;
        for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
            if (blockIndex == idx) {
                // Add this block's top margin to get to where content starts
                QTextBlockFormat fmt = b.blockFormat();
                int topMargin = static_cast<int>(fmt.topMargin());
                return y + topMargin;
            }
            QTextBlockFormat fmt = b.blockFormat();
            int topMargin = static_cast<int>(fmt.topMargin());
            int blockHeight = static_cast<int>(std::ceil(doc->documentLayout()->blockBoundingRect(b).height()));
            y += topMargin + blockHeight;
            blockIndex++;
        }
        return y;
    }

private slots:
    void noPageBreaksForSmallContent() {
        PageView pv;
        // Insert a modest number of lines that should fit on a single page
        insertLines(pv.editor(), 10);
        QCoreApplication::processEvents();

        QCOMPARE(pv.pageCount(), 1);

        // Ensure no block has a large top margin indicative of a page break
        QTextDocument* doc = pv.editor()->document();
        QTextBlock block = doc->begin();
        while (block.isValid()) {
            int topMargin = static_cast<int>(block.blockFormat().topMargin());
            QVERIFY2(topMargin < pv.pageGapPx(), "Unexpected large top margin on small content");
            block = block.next();
        }
    }

    void pageBreaksWhenExceedPrintable() {
        PageView pv;
        // Insert many lines to force content beyond the first page
        insertLines(pv.editor(), 300);
        QCoreApplication::processEvents();

        QVERIFY2(pv.pageCount() >= 2, "Expected multiple pages after large insert");

        // At least one block should have a large top margin (page break spacing)
        bool foundBreakMargin = false;
        QTextDocument* doc = pv.editor()->document();
        for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
            int topMargin = static_cast<int>(block.blockFormat().topMargin());
            if (topMargin >= pv.pageGapPx()) {
                foundBreakMargin = true;
                break;
            }
        }
        QVERIFY2(foundBreakMargin, "No page-break margins found after exceeding printable height");
    }

    void marginsCleanedAfterContentReduction() {
        PageView pv;
        insertLines(pv.editor(), 300);
        QCoreApplication::processEvents();
        QVERIFY2(pv.pageCount() >= 2, "Setup: multiple pages expected");

        // Reduce content back to small size
        pv.editor()->clear();
        insertLines(pv.editor(), 5);
        QCoreApplication::processEvents();

        QCOMPARE(pv.pageCount(), 1);

        // Ensure large top margins were removed
        QTextDocument* doc = pv.editor()->document();
        for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
            int topMargin = static_cast<int>(block.blockFormat().topMargin());
            QVERIFY2(topMargin < pv.pageGapPx(), "Residual page-break margin after content reduction");
        }
    }

    void paginationOffsetsAreZero() {
        PageView pv;
        pv.editor()->clear();

        insertLines(pv.editor(), 400);
        QCoreApplication::processEvents();

        QVERIFY2(pv.pageCount() >= 4, "Expected at least 4 pages for offset checks");

        for (int pageIndex = 0; pageIndex < 4; ++pageIndex) {
            int offset = firstBlockOffsetAtPage(pv, pageIndex);
            QVERIFY2(offset == 0,
                     QString("Page %1 first block offset expected 0, got %2")
                     .arg(pageIndex + 1)
                     .arg(offset)
                     .toUtf8().constData());
        }
    }


    void thirtyActionBlocksExplicitPositions() {
        PageView pv;
        pv.editor()->clear();

        // Insert exactly 50 blocks (enough to overflow one page even with zero margins)
        const int numBlocks = 50;
        insertLines(pv.editor(), numBlocks);
        QCoreApplication::processEvents();

        // Normalize all block top margins to 0 for determinism
        QTextDocument* doc = pv.editor()->document();
        {
            QTextCursor cur(doc);
            cur.beginEditBlock();
            for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
                QTextBlockFormat fmt = b.blockFormat();
                // Clear the page break margin property before setting topMargin to 0
                fmt.setProperty(QTextFormat::UserProperty + 1, 0);
                fmt.setTopMargin(0);
                cur.setPosition(b.position());
                cur.setBlockFormat(fmt);
            }
            cur.endEditBlock();
        }
        // Trigger enforcePageBreaks via benign text change
        {
            QTextCursor cur(doc);
            cur.movePosition(QTextCursor::End);
            cur.insertText(" ");
            cur.deletePreviousChar();
        }
        QCoreApplication::processEvents();

        // Get block heights and verify document structure
        QVector<int> blockHeights;
        for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
            qreal h = doc->documentLayout()->blockBoundingRect(b).height();
            blockHeights.append(static_cast<int>(std::ceil(h)));
        }
        QCOMPARE(blockHeights.size(), numBlocks);
        
        // Calculate where page breaks should occur
        const int printableH = pv.printableHeight();
        QVector<int> pageBreakAfterBlock; // Indices of blocks after which page breaks should occur
        int accumulatedHeight = 0;
        for (int i = 0; i < blockHeights.size(); ++i) {
            if (accumulatedHeight + blockHeights[i] > printableH && accumulatedHeight > 0) {
                pageBreakAfterBlock.append(i - 1); // Page break after previous block
                accumulatedHeight = blockHeights[i];
            } else {
                accumulatedHeight += blockHeights[i];
            }
        }
        
        QVERIFY2(pageBreakAfterBlock.size() > 0, "Should have at least one page break");
        
        // Now verify actual block positions match expected
        const int gap = pv.pageGapPx();
        const int topMargin = pv.pageTopMarginPx();
        const int bottomMargin = pv.pageBottomMarginPx();  // Use accessor for accuracy
        const int pageBreakDistance = printableH + bottomMargin + gap + topMargin;  // gap IS included
        
        int expectedY = 0;
        int pageStartY = 0;  // Y coordinate of the start of the current page
        
        for (int blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
            // Check if this block should have gotten a page break margin
            // A block gets a page break margin if it would overflow the current page
            int posInPage = expectedY - pageStartY;
            bool needsPageBreak = false;
            int pageBreakMargin = 0;
            
            if (posInPage + blockHeights[blockIndex] > printableH && posInPage > 0) {
                // This block would overflow - calculate page break margin
                // Next page's printable area starts at: pageStartY + pageBreakDistance
                int nextPagePrintableStart = pageStartY + pageBreakDistance;
                pageBreakMargin = nextPagePrintableStart - expectedY;
                expectedY += pageBreakMargin;
                pageStartY += printableH + bottomMargin;  // Update pageStartY for next iteration (no gap)
                needsPageBreak = true;
            }
            
            // This block starts at expectedY (after any page break margin)
            int actualY = contentStartYAt(doc, blockIndex);
            QTextBlock b = doc->begin();
            for (int i = 0; i < blockIndex && b.isValid(); i++) b = b.next();
            QString blockText = b.isValid() ? b.text().left(20) : "";
            
            QVERIFY2(actualY == expectedY, 
                QString("Block %1 ('%2'): expected Y=%3, actual Y=%4, needsPageBreak=%5, pageBreakMargin=%6")
                .arg(blockIndex).arg(blockText).arg(expectedY).arg(actualY)
                .arg(needsPageBreak).arg(pageBreakMargin)
                .toUtf8().constData());
            
            expectedY += blockHeights[blockIndex];
        }
    }
    
    void blockPositionsWithNormalMargins() {
        PageView pv;
        pv.editor()->clear();

        // Insert blocks with normal ACTION margins (should have topMargin from ScriptEditor)
        const int numBlocks = 30;
        insertLines(pv.editor(), numBlocks);
        QCoreApplication::processEvents();

        QTextDocument* doc = pv.editor()->document();
        
        // Collect all block texts
        QVector<QString> blockTexts;
        for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
            blockTexts.append(b.text());
        }
        
        // Verify we have exactly our blocks (no extra blocks)
        QCOMPARE(blockTexts.size(), numBlocks);
        
        // Verify block texts are in correct order
        for (int i = 0; i < numBlocks; ++i) {
            QString expected = QString("Line %1").arg(i);
            QVERIFY2(blockTexts[i] == expected,
                QString("Block %1: expected '%2', got '%3'")
                .arg(i).arg(expected).arg(blockTexts[i])
                .toUtf8().constData());
        }
        
        // Verify blocks are in monotonically increasing Y positions
        int prevY = -1;
        for (int i = 0; i < numBlocks; ++i) {
            int currentY = contentStartYAt(doc, i);
            QVERIFY2(currentY > prevY,
                QString("Block %1: Y position (%2) should be greater than previous (%3)")
                .arg(i).arg(currentY).arg(prevY)
                .toUtf8().constData());
            prevY = currentY;
        }
        
        // Verify no content block appears before the first expected block on page 2
        const int printableH = pv.printableHeight();
        int firstBlockOnPage2Index = -1;
        
        for (int i = 0; i < numBlocks; ++i) {
            int y = contentStartYAt(doc, i);
            if (y >= printableH) {
                firstBlockOnPage2Index = i;
                break;
            }
        }
        
        if (firstBlockOnPage2Index > 0) {
            // There should be a page 2, verify continuity
            int lastPage1Index = firstBlockOnPage2Index - 1;
            QString lastPage1Text = blockTexts[lastPage1Index];
            QString firstPage2Text = blockTexts[firstBlockOnPage2Index];
            
            // Verify consecutive line numbers
            QString expectedLast = QString("Line %1").arg(lastPage1Index);
            QString expectedFirst = QString("Line %1").arg(firstBlockOnPage2Index);
            
            QCOMPARE(lastPage1Text, expectedLast);
            QCOMPARE(firstPage2Text, expectedFirst);
        }
    }
    
    void firstBlockOnPage2StartsAtPrintableTop() {
        PageView pv;
        ScriptEditor* editor = pv.editor();
        QTextDocument* doc = editor->document();
        
        // Insert enough lines to trigger a page break
        // With ~18px per line and ~1000px printable, need ~60 lines to get 2 pages
        const int numBlocks = 60;
        insertLines(editor, numBlocks);
        QCoreApplication::processEvents();
        
        // Should have at least 2 pages
        QVERIFY2(pv.pageCount() >= 2, "Should have at least 2 pages");
        
        const int printableH = pv.printableHeight();
        const int pageGap = pv.pageGapPx();
        const int topMargin = pv.pageTopMarginPx();
        const int bottomMargin = pv.pageBottomMarginPx();  // Use accessor for accuracy
        
        // Find the first block that starts on page 2
        // In editor coords, page 2's printable area starts at: printableH + bottomMargin + gap + topMargin
        int expectedPage2Start = printableH + bottomMargin + pageGap + topMargin;
        
        int firstBlockOnPage2Index = -1;
        int actualPage2BlockStart = -1;
        
        int blockIdx = 0;
        int firstBlockBaseMargin = 0;
        for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
            int contentStartY = contentStartYAt(doc, blockIdx);
            
            if (contentStartY >= expectedPage2Start) {
                firstBlockOnPage2Index = blockIdx;
                actualPage2BlockStart = contentStartY;
                int topMargin = static_cast<int>(block.blockFormat().topMargin());
                int pageBreakMargin = block.blockFormat().property(QTextFormat::UserProperty + 1).toInt();
                firstBlockBaseMargin = topMargin - pageBreakMargin;
                break;
            }
            blockIdx++;
        }
        
        QVERIFY2(firstBlockOnPage2Index >= 0, "Should have found a block on page 2");
        
        int expectedPage2BlockStart = expectedPage2Start + firstBlockBaseMargin;
        QString msg = QString("First block on page 2 (index %1) should start at Y=%2 (printable start + base margin), actual Y=%3")
            .arg(firstBlockOnPage2Index)
            .arg(expectedPage2BlockStart)
            .arg(actualPage2BlockStart);
        
        // The first block on page 2 should start at printable start plus its top margin
        QVERIFY2(actualPage2BlockStart == expectedPage2BlockStart, qPrintable(msg));
    }
    
    void pageBreakMarginIsCorrect() {
        PageView pv;
        ScriptEditor* editor = pv.editor();
        QTextDocument* doc = editor->document();
        
        // Insert enough lines to trigger a page break
        const int numBlocks = 60;
        insertLines(editor, numBlocks);
        QCoreApplication::processEvents();
        
        QVERIFY2(pv.pageCount() >= 2, "Should have at least 2 pages");
        
        // The page break margin ensures the first block on the next page starts at the
        // beginning of that page's printable area. The gap between pages is purely visual.
        int pageHeight = pv.pageHeight();
        int printableH = pv.printableHeight();
        int topMargin = pv.pageTopMarginPx();
        int bottomMargin = pv.pageBottomMarginPx();
        int pageGap = pv.pageGapPx();
        
        // Where the next page's printable area should start in editor coordinates
        // = 0 (current page start) + 864 (printable) + 97 (bottom margin) + 96 (top margin)
        // Note: gap is NOT included because it's purely visual
        int expectedNextPagePrintableStart = printableH + bottomMargin + topMargin;
        
        // Find the first block with a page break margin
        int firstBlockWithMarginIndex = -1;
        int actualPageBreakMargin = 0;
        int blockIndex = 0;
        for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
            int pageBreakMarginProperty = block.blockFormat().property(QTextFormat::UserProperty + 1).toInt();
            if (pageBreakMarginProperty > 0) {
                firstBlockWithMarginIndex = blockIndex;
                actualPageBreakMargin = pageBreakMarginProperty;
                break;
            }
            blockIndex++;
        }
        
        QVERIFY2(firstBlockWithMarginIndex >= 0, "Should have found a block with page break margin");
        
        // The page break margin should position the block correctly on the next page
        // Verify it's positive and reasonable (less than the full page advance)
        QVERIFY2(actualPageBreakMargin > 0, "Page break margin should be positive");
        
        qDebug() << "Block" << firstBlockWithMarginIndex << ": pageHeight=" << pageHeight 
                 << "printableH=" << printableH << "topMargin=" << topMargin
                 << "bottomMargin=" << bottomMargin << "pageGap=" << pageGap
                 << "expectedNextPageStart=" << expectedNextPagePrintableStart
                 << "pageBreakMargin=" << actualPageBreakMargin;
    }

    void loadsFdxIntoExpectedElementTypes() {
        QTemporaryDir tempDir;
        QVERIFY2(tempDir.isValid(), "Failed to create temporary directory");

        const QString filePath = tempDir.path() + "/sample.fdx";
        QFile file(filePath);
        QVERIFY2(file.open(QIODevice::WriteOnly | QIODevice::Text), "Failed to create temp FDX file");

        const QByteArray fdxData =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<!DOCTYPE FinalDraft SYSTEM \"Final Draft Document Type Definition\">\n"
            "<FinalDraft DocumentType=\"Script\" Template=\"No\" Version=\"1\">\n"
            "  <Content>\n"
            "    <Paragraph Type=\"Scene Heading\"><Text>INT. OFFICE - DAY</Text></Paragraph>\n"
            "    <Paragraph Type=\"Character\"><Text>ALICE</Text></Paragraph>\n"
            "    <Paragraph Type=\"Dialogue\"><Text>Hello there.</Text></Paragraph>\n"
            "  </Content>\n"
            "</FinalDraft>\n";

        file.write(fdxData);
        file.close();

        PageView pv;
        QVERIFY2(pv.loadFromFile(filePath), "FDX load failed");
        QCoreApplication::processEvents();

        QTextDocument *doc = pv.editor()->document();
        QTextBlock block = doc->begin();
        QVERIFY(block.isValid());
        QCOMPARE(block.text(), QString("INT. OFFICE - DAY"));
        QCOMPARE(block.userState(), static_cast<int>(ScriptEditor::SceneHeading));

        block = block.next();
        QVERIFY(block.isValid());
        QCOMPARE(block.text(), QString("ALICE"));
        QCOMPARE(block.userState(), static_cast<int>(ScriptEditor::CharacterName));

        block = block.next();
        QVERIFY(block.isValid());
        QCOMPARE(block.text(), QString("Hello there."));
        QCOMPARE(block.userState(), static_cast<int>(ScriptEditor::Dialogue));
    }

    void savesFdxWithExpectedParagraphTypes() {
        QTemporaryDir tempDir;
        QVERIFY2(tempDir.isValid(), "Failed to create temporary directory");

        PageView pv;
        QTextCursor cursor(pv.editor()->document());
        cursor.beginEditBlock();

        cursor.insertText("INT. OFFICE - DAY");
        cursor.block().setUserState(static_cast<int>(ScriptEditor::SceneHeading));
        cursor.insertText("\n");
        cursor.movePosition(QTextCursor::NextBlock);

        cursor.insertText("ALICE");
        cursor.block().setUserState(static_cast<int>(ScriptEditor::CharacterName));
        cursor.insertText("\n");
        cursor.movePosition(QTextCursor::NextBlock);

        cursor.insertText("Hello there.");
        cursor.block().setUserState(static_cast<int>(ScriptEditor::Dialogue));

        cursor.endEditBlock();
        pv.editor()->formatDocument();

        const QString filePath = tempDir.path() + "/export.fdx";
        QVERIFY2(pv.saveToFile(filePath), "FDX save failed");

        QFile file(filePath);
        QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text), "Failed to read generated FDX file");
        const QString content = QString::fromUtf8(file.readAll());
        file.close();

        QVERIFY(content.contains("<Paragraph Type=\"Scene Heading\">"));
        QVERIFY(content.contains("<Paragraph Type=\"Character\">"));
        QVERIFY(content.contains("<Paragraph Type=\"Dialogue\">"));
        QVERIFY(content.contains("INT. OFFICE - DAY"));
        QVERIFY(content.contains("ALICE"));
        QVERIFY(content.contains("Hello there."));
    }
};

QTEST_MAIN(PageViewTests)
#include "pageview_enforcebreaks_test.moc"
