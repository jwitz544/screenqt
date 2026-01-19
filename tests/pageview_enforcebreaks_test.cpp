#include <QTest>
#include <QObject>
#include <QCoreApplication>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>
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

    int contentStartYAt(QTextDocument* doc, int idx) {
        int sumY = 0;
        int j = 0;
        for (QTextBlock bb = doc->begin(); bb.isValid() && j < idx; bb = bb.next(), ++j) {
            qreal hFj = doc->documentLayout()->blockBoundingRect(bb).height();
            int hj = static_cast<int>(std::ceil(hFj));
            int tmj = static_cast<int>(bb.blockFormat().topMargin());
            sumY += tmj + hj;
        }
        // current block top margin
        QTextBlock b = doc->begin();
        for (int k = 0; k < idx; ++k) b = b.next();
        int topM = static_cast<int>(b.blockFormat().topMargin());
        return sumY + topM;
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


    void thirtyActionBlocksExplicitPositions() {
        PageView pv;
        pv.editor()->clear();

        // Insert exactly 30 blocks
        insertLines(pv.editor(), 30);
        QCoreApplication::processEvents();

        // Normalize all block top margins to 0 for determinism
        QTextDocument* doc = pv.editor()->document();
        {
            QTextCursor cur(doc);
            cur.beginEditBlock();
            for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
                QTextBlockFormat fmt = b.blockFormat();
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

        // Compute heights and expected positions
        QVector<int> H; H.reserve(30);
        for (QTextBlock b = doc->begin(); b.isValid() && H.size() < 30; b = b.next()) {
            qreal hF = doc->documentLayout()->blockBoundingRect(b).height();
            H.push_back(static_cast<int>(std::ceil(hF)));
        }
        QCOMPARE(H.size(), 30);

        const int printableH = pv.printableHeight();
        // EnforcePageBreaks uses nextPageStart = printableH + top + bottom + gap
        QScreen *screen = QGuiApplication::primaryScreen();
        double dpiY = screen ? screen->logicalDotsPerInchY() : 96.0;
        int topMarginPx = static_cast<int>(std::round(dpiY * 1.0));
        int bottomMarginPx = static_cast<int>(std::round(dpiY * 1.0));
        const int gap = pv.pageGapPx();
        const int expectedPage2Start = printableH + topMarginPx + bottomMarginPx + gap;

        // Find first block that moves to page 2
        int k = -1; int acc = 0;
        for (int i = 0; i < H.size(); ++i) {
            if (acc + H[i] > printableH) { k = i; break; }
            acc += H[i];
        }
        QVERIFY2(k >= 0, "No block starts on page 2");

        // Expected Y for each of 30 blocks
        int E[30];
        int sum = 0;
        for (int i = 0; i < 30; ++i) {
            if (i < k) { E[i] = sum; sum += H[i]; }
            else if (i == k) { E[i] = expectedPage2Start; sum = H[i]; }
            else { E[i] = expectedPage2Start + sum; sum += H[i]; }
        }

        // Explicit asserts for each block
        QCOMPARE(contentStartYAt(doc, 0),  E[0]);
        QCOMPARE(contentStartYAt(doc, 1),  E[1]);
        QCOMPARE(contentStartYAt(doc, 2),  E[2]);
        QCOMPARE(contentStartYAt(doc, 3),  E[3]);
        QCOMPARE(contentStartYAt(doc, 4),  E[4]);
        QCOMPARE(contentStartYAt(doc, 5),  E[5]);
        QCOMPARE(contentStartYAt(doc, 6),  E[6]);
        QCOMPARE(contentStartYAt(doc, 7),  E[7]);
        QCOMPARE(contentStartYAt(doc, 8),  E[8]);
        QCOMPARE(contentStartYAt(doc, 9),  E[9]);
        QCOMPARE(contentStartYAt(doc, 10), E[10]);
        QCOMPARE(contentStartYAt(doc, 11), E[11]);
        QCOMPARE(contentStartYAt(doc, 12), E[12]);
        QCOMPARE(contentStartYAt(doc, 13), E[13]);
        QCOMPARE(contentStartYAt(doc, 14), E[14]);
        QCOMPARE(contentStartYAt(doc, 15), E[15]);
        QCOMPARE(contentStartYAt(doc, 16), E[16]);
        QCOMPARE(contentStartYAt(doc, 17), E[17]);
        QCOMPARE(contentStartYAt(doc, 18), E[18]);
        QCOMPARE(contentStartYAt(doc, 19), E[19]);
        QCOMPARE(contentStartYAt(doc, 20), E[20]);
        QCOMPARE(contentStartYAt(doc, 21), E[21]);
        QCOMPARE(contentStartYAt(doc, 22), E[22]);
        QCOMPARE(contentStartYAt(doc, 23), E[23]);
        QCOMPARE(contentStartYAt(doc, 24), E[24]);
        QCOMPARE(contentStartYAt(doc, 25), E[25]);
        QCOMPARE(contentStartYAt(doc, 26), E[26]);
        QCOMPARE(contentStartYAt(doc, 27), E[27]);
        QCOMPARE(contentStartYAt(doc, 28), E[28]);
        QCOMPARE(contentStartYAt(doc, 29), E[29]);
    }
};

QTEST_MAIN(PageViewTests)
#include "pageview_enforcebreaks_test.moc"
