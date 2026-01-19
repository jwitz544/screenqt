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
#include <QTimer>
#include <cmath>

PageView::PageView(QWidget *parent)
    : QWidget(parent), m_editor(new ScriptEditor(this))
{
    qDebug() << "[PageView] Constructor starting";
    // Compute fixed letter page size in pixels
    int pageW = static_cast<int>(inchToPxX(PAGE_WIDTH_INCHES));
    int pageH = static_cast<int>(inchToPxY(PAGE_HEIGHT_INCHES));
    qDebug() << "[PageView] Page size:" << pageW << "x" << pageH;
    m_pageRect = QRect(0, 0, pageW, pageH);

    // Margins: Left 1.5", Right 1", Top 1", Bottom 1"
    int left = static_cast<int>(inchToPxX(MARGIN_LEFT_INCHES));
    int right = static_cast<int>(inchToPxX(MARGIN_RIGHT_INCHES));
    int top = static_cast<int>(inchToPxY(MARGIN_TOP_INCHES));
    int bottom = static_cast<int>(inchToPxY(MARGIN_BOTTOM_INCHES));
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
    p.fillRect(rect(), QColor(BG_GRAY_VALUE, BG_GRAY_VALUE, BG_GRAY_VALUE));
    
    // Draw stacked pages with gaps - these provide the white background for text
    int x = (width() - m_pageRect.width()) / 2;
    if (x < PAGE_HORIZONTAL_PADDING) x = PAGE_HORIZONTAL_PADDING;
    for (int i = 0; i < m_pageCount; ++i) {
        int yOff = pageYOffset(i);
        QRect pageRect(x, yOff, m_pageRect.width(), m_pageRect.height());
        p.fillRect(pageRect, Qt::white);
        p.setPen(QPen(QColor(BORDER_GRAY_VALUE, BORDER_GRAY_VALUE, BORDER_GRAY_VALUE)));
        p.drawRect(pageRect.adjusted(0, 0, -1, -1));
        
        // Draw white rectangle for printable area to ensure text has white background
        QRect printableRect = m_printRect.translated(pageRect.topLeft());
        p.fillRect(printableRect, Qt::white);
        
        // Draw page number in top right corner
        QString pageNumStr = QString::number(i + 1) + ".";
        QFont pageNumFont("Courier New", PAGE_NUM_FONT_SIZE);
        p.setFont(pageNumFont);
        p.setPen(Qt::black);
        QRect pageNumRect = pageRect.adjusted(0, PAGE_NUM_TOP_OFFSET, -PAGE_NUM_RIGHT_MARGIN, 0);
        p.drawText(pageNumRect, Qt::AlignTop | Qt::AlignRight, pageNumStr);
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
    qDebug() << "[PageView] pageGap:" << PAGE_GAP_PX;
    
    // Base page position centered horizontally, start near top
    int x = (width() - m_pageRect.width()) / 2;
    if (x < PAGE_HORIZONTAL_PADDING) x = PAGE_HORIZONTAL_PADDING;
    int startY = PAGE_HORIZONTAL_PADDING;
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
    int totalEditorHeight = printableH * m_pageCount + PAGE_GAP_PX * (m_pageCount - 1);
    qDebug() << "[PageView] totalEditorHeight:" << totalEditorHeight;
    
    m_editor->setGeometry(QRect(firstPrint.left(), firstPrint.top(), printableW, totalEditorHeight));
    qDebug() << "[PageView] editor geometry:" << m_editor->geometry();
    
    // Ensure editor respects line wrap width
    m_editor->setLineWrapColumnOrWidth(printableW);

    // Set fixed size for this widget
    int totalPageHeight = m_pageRect.height() * m_pageCount + PAGE_GAP_PX * (m_pageCount - 1);
    int fixedWidth = m_pageRect.width() + WIDGET_HORIZONTAL_PADDING;
    setMinimumWidth(fixedWidth);
    setMaximumWidth(QWIDGETSIZE_MAX);
    setMinimumHeight(totalPageHeight + WIDGET_VERTICAL_PADDING);
    setMaximumHeight(totalPageHeight + WIDGET_VERTICAL_PADDING);
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
            pageStartY += printableH + PAGE_GAP_PX;
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
    return screen ? screen->logicalDotsPerInchX() : DEFAULT_DPI;
}

double PageView::dpiY() const
{
    QScreen *screen = QGuiApplication::primaryScreen();
    return screen ? screen->logicalDotsPerInchY() : DEFAULT_DPI;
}

double PageView::inchToPxX(double inches) const { return inches * dpiX(); }
double PageView::inchToPxY(double inches) const { return inches * dpiY(); }

int PageView::pageYOffset(int pageIndex) const
{
    // Y offset for page N: startY + (pageHeight + gap) * N
    return PAGE_HORIZONTAL_PADDING + (m_pageRect.height() + PAGE_GAP_PX) * pageIndex;
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
    m_loading = true; // Prevent enforcePageBreaks during load
    
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
    qDebug() << "[PageView::loadFromFile] Before formatDocument, isUndoAvailable:" << m_editor->document()->isUndoAvailable();
    m_editor->formatDocument();
    qDebug() << "[PageView::loadFromFile] After formatDocument, isUndoAvailable:" << m_editor->document()->isUndoAvailable();
    
    // Clear loading flag, disconnect textChanged, run enforcePageBreaks once, clear undo, reconnect
    QTimer::singleShot(0, this, [this, lines]() {
        m_loading = false;
        qDebug() << "[PageView::loadFromFile] Disconnecting textChanged signal";
        disconnect(m_editor, &QTextEdit::textChanged, this, &PageView::enforcePageBreaks);
        
        enforcePageBreaks(); // Run once with undo disabled
        
        m_editor->document()->clearUndoRedoStacks();
        qDebug() << "[PageView::loadFromFile] Loaded" << lines.size() << "lines, loading complete, undo stack cleared, isUndoAvailable:" << m_editor->document()->isUndoAvailable();
        
        qDebug() << "[PageView::loadFromFile] Reconnecting textChanged signal";
        connect(m_editor, &QTextEdit::textChanged, this, &PageView::enforcePageBreaks);
    });
    
    return true;
}

bool PageView::exportToPdf(const QString &filePath)
{
    qDebug() << "[PageView] Exporting to PDF:" << filePath;
    
    // Create PDF writer with letter page size (8.5" x 11")
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize::Letter);
    writer.setResolution(PDF_RESOLUTION_DPI); // High quality output
    
    // Set margins: left 1.5", others 1"
    QMarginsF margins(MARGIN_LEFT_INCHES, MARGIN_TOP_INCHES, MARGIN_RIGHT_INCHES, MARGIN_BOTTOM_INCHES); // in inches
    writer.setPageMargins(margins, QPageLayout::Inch);
    
    // Start painting
    QPainter painter(&writer);
    
    // Get the paintable area (already accounts for margins)
    QRect paintRect = writer.pageLayout().paintRectPixels(writer.resolution());
    qDebug() << "[PDF] PDF paintable area:" << paintRect;
    
    // Get document
    QTextDocument *doc = m_editor->document();
    const int printableH = printableHeightPerPage();
    const int topMargin = m_printRect.top();
    
    qDebug() << "[PDF] Screen printable:" << m_printRect.width() << "x" << printableH;
    qDebug() << "[PDF] Top margin:" << topMargin;
    
    // Calculate scale to fit screen printable area into PDF paintable area
    qreal scaleX = static_cast<qreal>(paintRect.width()) / m_printRect.width();
    qreal scaleY = static_cast<qreal>(paintRect.height()) / printableH;
    qreal scale = qMin(scaleX, scaleY);
    
    qDebug() << "[PDF] Scale X:" << scaleX << "Y:" << scaleY << "-> Using:" << scale;
    
    // Render each page by clipping the document
    int totalPages = m_pageCount;
    qDebug() << "[PDF] Total pages to export:" << totalPages;
    
    for (int pageNum = 0; pageNum < totalPages; ++pageNum) {
        if (pageNum > 0) {
            qDebug() << "[PDF] Creating new page" << (pageNum + 1);
            writer.newPage();
        }
        
        qDebug() << "[PDF] ===== DRAWING PAGE" << (pageNum + 1) << "=====";
        
        painter.save();
        painter.scale(scale, scale);
        
        // Find where content actually starts on this page
        // For pages after the first, enforcePageBreaks adds large margins to push blocks to new pages
        // We need to find the first block that's visible on this page
        int pageContentStart = 0;
        int pageContentEnd = printableH;
        
        if (pageNum > 0) {
            // Find the first block whose content appears on this page
            int currentY = 0;
            QTextBlock block = doc->begin();
            while (block.isValid()) {
                QTextLayout *layout = block.layout();
                int blockHeight = static_cast<int>(std::ceil(layout->boundingRect().height()));
                int blockTopMargin = static_cast<int>(block.blockFormat().topMargin());
                int contentStartY = currentY + blockTopMargin;
                
                // If this block's content starts on or after our target page area, use it
                int expectedPageStart = (printableH + PAGE_GAP_PX) * pageNum;
                if (contentStartY >= expectedPageStart) {
                    pageContentStart = contentStartY;
                    pageContentEnd = contentStartY + printableH;
                    qDebug() << "[PDF] Page" << (pageNum + 1) << "first block content at Y=" << contentStartY
                             << "text:" << block.text().left(40);
                    break;
                }
                
                currentY += blockTopMargin + blockHeight;
                block = block.next();
            }
        }
        
        qDebug() << "[PDF] Page" << (pageNum + 1) << "content range:" << pageContentStart << "-" << pageContentEnd;
        
        // Translate to position this page's content at Y=0
        painter.translate(0, -pageContentStart);
        
        // Clip to show only this page's printable content
        QRectF clipRect(0, pageContentStart, m_printRect.width(), printableH);
        painter.setClipRect(clipRect);
        
        qDebug() << "[PDF] Translate:" << -pageContentStart << "Clip rect:" << clipRect;
        
        // Draw the entire document (clipped to current page)
        QAbstractTextDocumentLayout::PaintContext context;
        context.clip = clipRect;
        context.palette.setColor(QPalette::Text, Qt::black);
        doc->documentLayout()->draw(&painter, context);
        
        painter.restore();
        
        // Add page number in top right (after restoring from scaled context)
        painter.save();
        QFont pageNumFont("Courier New", PDF_PAGE_NUM_FONT_SIZE);
        pageNumFont.setPointSizeF(PDF_PAGE_NUM_FONT_SIZE);
        painter.setFont(pageNumFont);
        painter.setPen(Qt::black);
        
        // Position in top right of paintable area
        QString pageNumStr = QString::number(pageNum + 1) + ".";
        QRectF pageNumRect(paintRect.width() - PDF_PAGE_NUM_RIGHT_OFFSET, PDF_PAGE_NUM_TOP_OFFSET, PDF_PAGE_NUM_WIDTH, PDF_PAGE_NUM_HEIGHT);
        painter.drawText(pageNumRect, Qt::AlignRight | Qt::AlignTop, pageNumStr);
        painter.restore();
        
        qDebug() << "[PDF] Page" << (pageNum + 1) << "rendering complete";
    }
    
    painter.end();
    
    qDebug() << "[PageView] PDF export completed successfully";
    return true;
}

int PageView::printableHeightPerPage() const
{
    return m_printRect.height();
}

int PageView::printableHeight() const
{
    return m_printRect.height();
}

void PageView::enforcePageBreaks()
{
    // Guard against recursive calls
    if (m_enforcingBreaks) {
        return;
    }
    
    // Skip during document loading/initialization
    if (m_loading) {
        qDebug() << "[PageView::enforcePageBreaks] Skipping - document is loading";
        return;
    }
    
    m_enforcingBreaks = true;
    
    qDebug() << "[PageView] === enforcePageBreaks START ===";
    
    QTextDocument *doc = m_editor->document();
    const int printableH = printableHeightPerPage();
    const int topMargin = m_printRect.top();
    const int bottomMargin = m_pageRect.height() - (m_printRect.top() + m_printRect.height());
    const int pageAdvance = printableH + topMargin + bottomMargin + PAGE_GAP_PX;
    qDebug() << "[PageView] printableHeightPerPage:" << printableH << "gap:" << PAGE_GAP_PX;
    
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
                     << "gap=" << PAGE_GAP_PX;
            
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
            if (existingMargin >= PAGE_GAP_PX) {
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
    
    qDebug() << "[PageView] Total blocks:" << blockNum << "final Y:" << currentY;
    qDebug() << "[PageView] === enforcePageBreaks END ===";

    cursor.endEditBlock();

    qDebug() << "[PageView] Undo: " << doc->isUndoAvailable();
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
    int xMargin = SCROLL_X_MARGIN;
    int yMargin = SCROLL_Y_MARGIN; // give room above/below
    sa->ensureVisible(center.x(), center.y(), xMargin, yMargin);
}
