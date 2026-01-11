#include "pageview.h"
#include "scripteditor.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextBlockFormat>
#include <QTextLayout>
#include <QFrame>
#include <QScrollArea>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QPdfWriter>
#include <cmath>

PageView::PageView(QWidget *parent)
    : QWidget(parent), m_editor(new ScriptEditor(this))
{
    qDebug() << "[PageView] Constructor starting";
    // Compute fixed letter page size in pixels
    int pageW = static_cast<int>(inchToPxX(8.5));
    int pageH = static_cast<int>(inchToPxY(11.0));
    qDebug() << "[PageView] Page size:" << pageW << "x" << pageH;
    m_pageRect = QRect(0, 0, pageW, pageH);

    // Margins: Left 1.5", Right 1", Top 1", Bottom 1"
    int left = static_cast<int>(inchToPxX(1.5));
    int right = static_cast<int>(inchToPxX(1.0));
    int top = static_cast<int>(inchToPxY(1.0));
    int bottom = static_cast<int>(inchToPxY(1.0));
    m_printRect = QRect(left, top, pageW - left - right, pageH - top - bottom);
    qDebug() << "[PageView] Print rect:" << m_printRect;

    // Editor inside printable area; disable scrollbars so outer view handles scrolling
    m_editor->setFrameStyle(QFrame::NoFrame);
    m_editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // Make editor background transparent so page gaps show through
    QPalette pal = m_editor->palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    m_editor->setPalette(pal);
    m_editor->setAutoFillBackground(false);
    m_editor->viewport()->setAutoFillBackground(false);

    // React to document size changes to paginate
    connect(m_editor->document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
            this, &PageView::updatePagination);
    connect(m_editor, &QTextEdit::textChanged, this, &PageView::enforcePageBreaks);
        connect(m_editor, &QTextEdit::cursorPositionChanged, this, &PageView::scrollToCursor);

    layoutPages();
    qDebug() << "[PageView] Constructor complete, page count:" << m_pageCount;
}

void PageView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    // Gray background
    p.fillRect(rect(), QColor(230, 230, 230));
    
    // Draw stacked pages with gaps - these provide the white background for text
    int x = (width() - m_pageRect.width()) / 2;
    if (x < 20) x = 20;
    for (int i = 0; i < m_pageCount; ++i) {
        int yOff = pageYOffset(i);
        QRect pageRect(x, yOff, m_pageRect.width(), m_pageRect.height());
        p.fillRect(pageRect, Qt::white);
        p.setPen(QPen(QColor(200, 200, 200)));
        p.drawRect(pageRect.adjusted(0, 0, -1, -1));
        
        // Draw white rectangle for printable area to ensure text has white background
        QRect printableRect = m_printRect.translated(pageRect.topLeft());
        p.fillRect(printableRect, Qt::white);
    }
}

void PageView::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    layoutPages();
}

void PageView::layoutPages()
{
    qDebug() << "[PageView] === layoutPages START ===";
    qDebug() << "[PageView] pageCount:" << m_pageCount;
    qDebug() << "[PageView] pageRect:" << m_pageRect;
    qDebug() << "[PageView] printRect (relative):" << m_printRect;
    qDebug() << "[PageView] pageGap:" << m_pageGap;
    
    // Base page position centered horizontally, start near top
    int x = (width() - m_pageRect.width()) / 2;
    if (x < 20) x = 20;
    int startY = 20;
    m_pageRect.moveTopLeft(QPoint(x, startY));

    // Printable width/height
    const int printableW = m_printRect.width();
    const int printableH = m_printRect.height();
    qDebug() << "[PageView] printableW:" << printableW << "printableH:" << printableH;

    // Compute first page printable rect (absolute position)
    QRect firstPrint = m_printRect.translated(x, startY);
    qDebug() << "[PageView] firstPrint (absolute):" << firstPrint;

    // Editor height spans all printable areas PLUS gaps between pages
    // Each page has printableH of content, with gap pixels between pages
    int totalEditorHeight = printableH * m_pageCount + m_pageGap * (m_pageCount - 1);
    qDebug() << "[PageView] totalEditorHeight:" << totalEditorHeight;
    
    m_editor->setGeometry(QRect(firstPrint.left(), firstPrint.top(), printableW, totalEditorHeight));
    qDebug() << "[PageView] editor geometry:" << m_editor->geometry();
    
    // Ensure editor respects line wrap width
    m_editor->setLineWrapColumnOrWidth(printableW);

    // Set fixed size for this widget
    int totalPageHeight = m_pageRect.height() * m_pageCount + m_pageGap * (m_pageCount - 1);
    int fixedWidth = m_pageRect.width() + 40;
    setMinimumWidth(fixedWidth);
    setMaximumWidth(QWIDGETSIZE_MAX);
    setMinimumHeight(totalPageHeight + 40);
    setMaximumHeight(totalPageHeight + 40);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    qDebug() << "[PageView] widget size:" << size();
    qDebug() << "[PageView] === layoutPages END ===";
}

void PageView::updatePagination()
{
    // Recompute pages by simulating pagination without mutating the document.
    const int printableH = m_printRect.height();
    int pages = 1;
    int pageStartY = 0;   // start of current page printable area
    int currentY = 0;     // cumulative position in doc coords

    QTextDocument *doc = m_editor->document();
    QTextBlock block = doc->begin();
    while (block.isValid()) {
        qreal blockHeightF = doc->documentLayout()->blockBoundingRect(block).height();
        int blockHeight = static_cast<int>(std::ceil(blockHeightF));
        int blockTopMargin = static_cast<int>(block.blockFormat().topMargin());

        int posInPage = currentY - pageStartY + blockTopMargin;

        if (posInPage + blockHeight > printableH && posInPage > 0) {
            // move to next page
            pages++;
            pageStartY += printableH + m_pageGap;
            currentY = pageStartY + blockTopMargin + blockHeight;
        } else {
            currentY += blockTopMargin + blockHeight;
        }

        block = block.next();
    }

    if (pages < 1) pages = 1;
    qDebug() << "[PageView] updatePagination: pages=" << pages << "currentY=" << currentY << "printableH=" << printableH;
    if (pages != m_pageCount) {
        m_pageCount = pages;
        layoutPages();
        update();
    }
}

double PageView::dpiX() const
{
    QScreen *screen = QGuiApplication::primaryScreen();
    return screen ? screen->logicalDotsPerInchX() : 96.0;
}

double PageView::dpiY() const
{
    QScreen *screen = QGuiApplication::primaryScreen();
    return screen ? screen->logicalDotsPerInchY() : 96.0;
}

double PageView::inchToPxX(double inches) const { return inches * dpiX(); }
double PageView::inchToPxY(double inches) const { return inches * dpiY(); }

int PageView::pageYOffset(int pageIndex) const
{
    // Y offset for page N: startY + (pageHeight + gap) * N
    return 20 + (m_pageRect.height() + m_pageGap) * pageIndex;
}

void PageView::loadSampleText()
{
    qDebug() << "[PageView] Loading sample text";
    QTextCursor cursor(m_editor->document());
    cursor.beginEditBlock();
    
    // Helper to insert line with element type
    auto insertLine = [&](const QString &text, int elementType) {
        cursor.insertText(text);
        QTextBlock block = cursor.block();
        block.setUserState(elementType);
        cursor.insertText("\n");
        cursor.movePosition(QTextCursor::NextBlock);
    };
    
    insertLine("INT. COFFEE SHOP - DAY", 0);
    insertLine("", 1);
    insertLine("A small, cozy coffee shop. SARAH (30s, tired but determined) sits at a corner table with her laptop. The place is nearly empty except for a BARISTA wiping down the counter.", 1);
    insertLine("", 1);
    insertLine("Sarah's fingers hover over the keyboard. She looks at the screen, then out the window, then back to the screen.", 1);
    insertLine("", 1);
    insertLine("SARAH", 2);
    insertLine("(to herself)", 4);
    insertLine("This is it. Just write something.", 3);
    insertLine("", 1);
    insertLine("She types furiously for a moment, then stops. Deletes everything.", 1);
    insertLine("", 1);
    insertLine("The door chimes. MIKE (40s, casual but put-together) enters, spots Sarah, and walks over.", 1);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("Still working on that screenplay?", 3);
    insertLine("", 1);
    insertLine("SARAH", 2);
    insertLine("Trying to. It's harder than I thought.", 3);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("(sitting down)", 4);
    insertLine("The first page is always the hardest. What's it about?", 3);
    insertLine("", 1);
    insertLine("SARAH", 2);
    insertLine("A screenwriter who can't write.", 3);
    insertLine("", 1);
    insertLine("Mike laughs.", 1);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("Meta. I like it.", 3);
    insertLine("", 1);
    insertLine("SARAH", 2);
    insertLine("It's torture. Every word feels wrong.", 3);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("Then write it wrong. You can fix it later.", 3);
    insertLine("", 1);
    insertLine("Sarah sighs and closes her laptop.", 1);
    insertLine("", 1);
    insertLine("SARAH", 2);
    insertLine("I need a break. Want to walk?", 3);
    insertLine("", 1);
    insertLine("EXT. PARK - LATER", 0);
    insertLine("", 1);
    insertLine("Sarah and Mike walk through a busy park. Kids play on swings. A street musician plays guitar.", 1);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("You know what your problem is?", 3);
    insertLine("", 1);
    insertLine("SARAH", 2);
    insertLine("(defensive)", 4);
    insertLine("I don't have a problem.", 3);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("You're overthinking it. Just let the story flow.", 3);
    insertLine("", 1);
    insertLine("SARAH", 2);
    insertLine("Easy for you to say. You finished three screenplays last year.", 3);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("And they all sucked. But I finished them. That's the point.", 3);
    insertLine("", 1);
    insertLine("They walk in silence for a moment.", 1);
    insertLine("", 1);
    insertLine("SARAH", 2);
    insertLine("Maybe you're right. Maybe I just need to stop being so precious about it.", 3);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("Exactly. Write drunk, edit sober. Hemingway said that.", 3);
    insertLine("", 1);
    insertLine("SARAH", 2);
    insertLine("(smiling)", 4);
    insertLine("I don't think he actually said that.", 3);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("Well, he should have.", 3);
    insertLine("", 1);
    insertLine("INT. SARAH'S APARTMENT - NIGHT", 0);
    insertLine("", 1);
    insertLine("Small, cluttered, but warm. Sarah sits at her desk, laptop open. The cursor blinks on an empty page.", 1);
    insertLine("", 1);
    insertLine("She takes a deep breath and starts typing.", 1);
    insertLine("", 1);
    insertLine("SARAH (V.O.)", 2);
    insertLine("The first step is always the hardest. But once you start, the words just come.", 3);
    insertLine("", 1);
    insertLine("Words fill the screen. Sarah smiles.", 1);
    insertLine("", 1);
    insertLine("INT. COFFEE SHOP - THE NEXT DAY", 0);
    insertLine("", 1);
    insertLine("Sarah sits at the same table, typing away. Mike enters and approaches.", 1);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("How's it going?", 3);
    insertLine("", 1);
    insertLine("SARAH", 2);
    insertLine("(beaming)", 4);
    insertLine("I wrote twenty pages last night.", 3);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("That's amazing! See? I told you.", 3);
    insertLine("", 1);
    insertLine("SARAH", 2);
    insertLine("They're probably terrible, but I don't care. I finished something.", 3);
    insertLine("", 1);
    insertLine("Mike sits down across from her.", 1);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("That's all that matters. What's next?", 3);
    insertLine("", 1);
    insertLine("SARAH", 2);
    insertLine("Rewrite. Then rewrite again. Then maybe show it to someone.", 3);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("You'll show it to me, right?", 3);
    insertLine("", 1);
    insertLine("SARAH", 2);
    insertLine("Maybe. If you promise not to laugh.", 3);
    insertLine("", 1);
    insertLine("MIKE", 2);
    insertLine("I promise.", 3);
    insertLine("", 1);
    insertLine("They share a smile.", 1);
    insertLine("", 1);
    insertLine("FADE OUT.", 6);
    
    cursor.endEditBlock();
    m_editor->moveCursor(QTextCursor::Start);
    m_editor->formatDocument();
    qDebug() << "[PageView] Sample text loaded";
}

bool PageView::saveToFile(const QString &filePath)
{
    qDebug() << "[PageView] Saving to:" << filePath;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "[PageView] Failed to open file for writing";
        return false;
    }
    
    QJsonArray lines;
    QTextDocument *doc = m_editor->document();
    QTextBlock block = doc->begin();
    
    while (block.isValid()) {
        QJsonObject line;
        line["text"] = block.text();
        line["type"] = block.userState();
        lines.append(line);
        block = block.next();
    }
    
    QJsonObject root;
    root["version"] = 1;
    root["lines"] = lines;
    
    QJsonDocument jsonDoc(root);
    file.write(jsonDoc.toJson());
    file.close();
    
    qDebug() << "[PageView] Saved" << lines.size() << "lines";
    return true;
}

bool PageView::loadFromFile(const QString &filePath)
{
    qDebug() << "[PageView] Loading from:" << filePath;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "[PageView] Failed to open file for reading";
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qDebug() << "[PageView] Invalid JSON";
        return false;
    }
    
    QJsonObject root = jsonDoc.object();
    QJsonArray lines = root["lines"].toArray();
    
    m_editor->clear();
    QTextCursor cursor(m_editor->document());
    cursor.beginEditBlock();
    
    for (const QJsonValue &val : lines) {
        QJsonObject line = val.toObject();
        QString text = line["text"].toString();
        int type = line["type"].toInt();
        
        cursor.insertText(text);
        QTextBlock block = cursor.block();
        block.setUserState(type);
        cursor.insertText("\n");
        cursor.movePosition(QTextCursor::NextBlock);
    }
    
    cursor.endEditBlock();
    m_editor->moveCursor(QTextCursor::Start);
    m_editor->formatDocument();
    
    qDebug() << "[PageView] Loaded" << lines.size() << "lines";
    return true;
}

bool PageView::exportToPdf(const QString &filePath)
{
    qDebug() << "[PageView] Exporting to PDF:" << filePath;
    
    // Create PDF writer with letter page size (8.5" x 11")
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize::Letter);
    writer.setResolution(300); // High quality output
    
    // Paint the document onto the PDF
    QPainter painter(&writer);
    painter.setFont(m_editor->font());
    
    // Draw the pages
    QTextDocument *doc = m_editor->document();
    QAbstractTextDocumentLayout::PaintContext context;
    doc->documentLayout()->draw(&painter, context);
    painter.end();
    
    qDebug() << "[PageView] PDF export completed";
    return true;
}

int PageView::printableHeightPerPage() const
{
    return m_printRect.height();
}

void PageView::enforcePageBreaks()
{
    // Guard against recursive calls
    if (m_enforcingBreaks) {
        return;
    }
    m_enforcingBreaks = true;
    
    qDebug() << "[PageView] === enforcePageBreaks START ===";
    
    QTextDocument *doc = m_editor->document();
    const int printableH = printableHeightPerPage();
    const int topMargin = m_printRect.top();
    const int bottomMargin = m_pageRect.height() - (m_printRect.top() + m_printRect.height());
    const int pageAdvance = printableH + topMargin + bottomMargin + m_pageGap;
    qDebug() << "[PageView] printableHeightPerPage:" << printableH << "gap:" << m_pageGap;
    
    // Track cumulative Y position as we iterate through blocks
    int currentY = 0;          // Absolute Y in document coords
    int currentPage = 0;       // Which page we're on (0-based)
    int pageStartY = 0;        // Absolute Y where current page's printable area starts
    
    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    
    QTextBlock block = doc->begin();
    int blockNum = 0;
    
    while (block.isValid()) {
        blockNum++;
        
        // Use document layout for accurate height (includes line spacing/formatting)
        qreal blockHeightF = doc->documentLayout()->blockBoundingRect(block).height();
        int blockHeight = static_cast<int>(std::ceil(blockHeightF));
        
        // Get the block's existing top margin (spacing BEFORE this block)
        int blockTopMargin = static_cast<int>(block.blockFormat().topMargin());
        
        // Position within current page (including the spacing before this block)
        int posInPage = currentY - pageStartY + blockTopMargin;
        
        qDebug() << "[PageView] Block" << blockNum << ":"
                 << "height=" << blockHeight
                 << "topMargin=" << blockTopMargin
                 << "currentY=" << currentY
                 << "page=" << currentPage
                 << "posInPage=" << posInPage
                 << "remaining=" << (printableH - posInPage)
                 << "text=" << block.text().left(30);
        
        // Check if this block would exceed the current page
        if (posInPage + blockHeight > printableH && posInPage > 0) {
            // Block crosses page boundary
            // Push this block so it begins at the next page's printable top.
            // Distance from current printable top to next printable top includes bottom margin, gap, and top margin.
            int nextPageStart = pageStartY + pageAdvance;
            int blockCurrentStart = currentY;
            int spacingNeeded = nextPageStart - blockCurrentStart;
            
            qDebug() << "[PageView]   -> Block crosses page!"
                     << "blockStart=" << blockCurrentStart
                     << "nextPageStart=" << nextPageStart  
                     << "spacingNeeded=" << spacingNeeded
                     << "topMargin=" << topMargin
                     << "bottomMargin=" << bottomMargin
                     << "gap=" << m_pageGap;
            
            cursor.setPosition(block.position());
            QTextBlockFormat fmt = block.blockFormat();
            fmt.setTopMargin(spacingNeeded);
            cursor.setBlockFormat(fmt);
            
            // Move to next page
            currentPage++;
            currentY = nextPageStart;  // Block now starts at next page
            pageStartY = nextPageStart;  // New page starts here
            currentY += blockHeight;  // Add this block's height
            
            qDebug() << "[PageView]   -> Now on page" << currentPage << "currentY=" << currentY;
        } else {
            // Block fits on current page
            // Only remove top margins that look like page breaks (>= gap size)
            QTextBlockFormat fmt = block.blockFormat();
            int existingMargin = static_cast<int>(fmt.topMargin());
            if (existingMargin >= m_pageGap) {
                qDebug() << "[PageView]   -> Removing page break margin of" << existingMargin;
                fmt.setTopMargin(0);
                cursor.setPosition(block.position());
                cursor.setBlockFormat(fmt);
            }
            
            // Advance currentY by the block's top margin AND its height
            currentY += blockTopMargin + blockHeight;
        }
        
        block = block.next();
    }
    
    cursor.endEditBlock();
    
    qDebug() << "[PageView] Total blocks:" << blockNum << "final Y:" << currentY;
    qDebug() << "[PageView] === enforcePageBreaks END ===";
    
    m_enforcingBreaks = false;
}

void PageView::scrollToCursor()
{
    // Find the owning scroll area
    QWidget *p = parentWidget();
    QScrollArea *sa = nullptr;
    while (p && !sa) {
        sa = qobject_cast<QScrollArea*>(p);
        p = p->parentWidget();
    }
    if (!sa) return;

    // Cursor rect in editor coords
    QRect cr = m_editor->cursorRect();
    // Map to PageView coords
    QPoint center = m_editor->mapTo(this, cr.center());
    int xMargin = 40;
    int yMargin = 120; // give room above/below
    sa->ensureVisible(center.x(), center.y(), xMargin, yMargin);
}
