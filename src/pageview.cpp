#include "pageview.h"
#include "pdfexporter.h"
#include "screenplayio.h"
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
#include <QDebug>
#include <QTimer>
#include <cmath>

namespace {
int blockHeightPx(QTextDocument *doc, const QTextBlock &block) {
    return static_cast<int>(std::ceil(doc->documentLayout()->blockBoundingRect(block).height()));
}
}

PageView::PageView(QWidget *parent)
    : QWidget(parent), m_editor(new ScriptEditor(this))
{
    qDebug() << "[PageView] Constructor starting";
    // Remove default document margin to align layout with page calculations
    m_editor->document()->setDocumentMargin(0);
    recalculatePageMetrics();

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
    
    const int pageTopMarginPx = m_printRect.top();
    const int printableH = m_printRect.height();
    QTextDocument *doc = m_editor->document();
    const int editorLeft = m_editor->geometry().left();
    const int editorTop = m_editor->geometry().top();

    if (m_debugMode) {
        // Draw page break reference lines
        int calcPageStartY = 0;
        int calcNaturalY = pageTopMarginPx;

        // Draw page 1 printable area start reference
        int page1PaintableStart = pagePrintableStartY(0);

        p.setPen(QPen(Qt::yellow, 2, Qt::SolidLine));
        p.drawLine(x + m_printRect.left(), page1PaintableStart, x + m_printRect.left() + m_printRect.width(), page1PaintableStart);
        p.setFont(QFont("Courier New", 9, QFont::Bold));
        p.setPen(Qt::yellow);
        p.drawText(QRect(x + m_printRect.left() + 5, page1PaintableStart - 20, 300, 20),
                  Qt::AlignLeft | Qt::AlignTop, "PAGE1_PRINTABLE_START");

        // Draw first block actual position
        QTextBlock firstBlock = doc->begin();
        if (firstBlock.isValid()) {
            qreal firstBlockQtY = doc->documentLayout()->blockBoundingRect(firstBlock).top();
            int firstBlockScreenY = static_cast<int>(firstBlockQtY) + editorTop;
            p.setPen(QPen(Qt::magenta, 2, Qt::DashLine));
            p.drawLine(x + m_printRect.left(), firstBlockScreenY, x + m_printRect.left() + m_printRect.width(), firstBlockScreenY);
            p.setPen(Qt::magenta);
            int offsetDiff1 = page1PaintableStart - firstBlockScreenY;
            p.drawText(QRect(x + m_printRect.left() + 5, firstBlockScreenY - 20, 300, 20),
                      Qt::AlignLeft | Qt::AlignTop, QString("FIRST_BLOCK_ACTUAL | offset:%1px").arg(offsetDiff1));
        }

        QTextBlock block = doc->begin();
        int blockIdx = 0;
        while (block.isValid()) {
            QRectF blockRect = doc->documentLayout()->blockBoundingRect(block);
            int blockHeight = blockHeightPx(doc, block);

            int baseMargin = static_cast<int>(block.blockFormat().topMargin());
            int existingPageBreakMargin = block.blockFormat().property(QTextFormat::UserProperty + 1).toInt();
            int marginForCalc = baseMargin - existingPageBreakMargin;

            int calcPrintableStartY = calcPageStartY + pageTopMarginPx;
            int calcPosInPage = (calcNaturalY - calcPrintableStartY) + marginForCalc;

            // Check if this block overflows
            if (calcPosInPage + blockHeight > printableH && calcPosInPage > 0) {
                // This block overflows - draw reference lines and info
                int nextPageStartY = calcPageStartY + m_pageRect.height() + PAGE_GAP_PX;
                int nextPagePrintableStart = nextPageStartY + pageTopMarginPx;
                int pageBreakMargin = nextPagePrintableStart - (calcNaturalY + marginForCalc);

                int screenY = pageYOffset(0) + nextPagePrintableStart + editorTop - m_editor->geometry().top();

                // Draw cyan line showing calculated page 2 printable start
                p.setPen(QPen(Qt::cyan, 3, Qt::SolidLine));
                p.drawLine(x + m_printRect.left(), screenY, x + m_printRect.left() + m_printRect.width(), screenY);

                // Draw info box with detailed calculations
                p.setFont(QFont("Courier New", 9, QFont::Bold));
                p.setPen(Qt::cyan);
                QString calcInfo = QString("CALC_PAGE2_START:%1 | nextPageStartY:%2 | calcMargin:%3 | pageHeight:%4 | gap:%5 | calcPageStart:%6")
                    .arg(nextPagePrintableStart).arg(nextPageStartY).arg(pageBreakMargin)
                    .arg(m_pageRect.height()).arg(PAGE_GAP_PX).arg(calcPageStartY);
                p.drawText(QRect(x + m_printRect.left() + 5, screenY - 50, m_printRect.width() - 10, 50),
                          Qt::AlignLeft | Qt::AlignBottom, calcInfo);

                // Draw the actual block position for comparison
                int actualBlockY = static_cast<int>(blockRect.top()) + editorTop;
                p.setPen(QPen(Qt::magenta, 2, Qt::DashLine));
                p.drawLine(x + m_printRect.left(), actualBlockY, x + m_printRect.left() + m_printRect.width(), actualBlockY);
                p.setPen(Qt::magenta);
                p.setFont(QFont("Courier New", 8));
                int offsetDiff = screenY - actualBlockY;
                p.drawText(QRect(x + m_printRect.left() + 5, actualBlockY - 20, 300, 20),
                          Qt::AlignLeft | Qt::AlignTop,
                          QString("ACTUAL_BLOCK | offset_diff:%1px").arg(offsetDiff));

                blockIdx++;
                break;  // Just show the first page break for clarity
            }

            if (calcPosInPage + blockHeight > printableH && calcPosInPage > 0) {
                calcPageStartY += m_pageRect.height() + PAGE_GAP_PX;
                calcNaturalY = calcPageStartY + pageTopMarginPx + blockHeight;
            } else {
                calcNaturalY += marginForCalc + blockHeight;
            }

            blockIdx++;
            block = block.next();
        }
    }
    
    // Debug mode: Draw colored boxes for each text block showing their heights
    if (m_debugMode) {
        // Color palette for debug boxes
        QVector<QColor> colors = {
            QColor(255, 100, 100, 100),  // Red
            QColor(100, 255, 100, 100),  // Green
            QColor(100, 100, 255, 100),  // Blue
            QColor(255, 255, 100, 100),  // Yellow
            QColor(255, 100, 255, 100),  // Magenta
            QColor(100, 255, 255, 100),  // Cyan
        };
        
        int colorIdx = 0;
        int currentPage = 0;
        int pageStartY = 0;
        
        // Get the first page's absolute Y position
        int firstPageAbsY = pageYOffset(0) + m_printRect.top();
        
        // Also draw page break calculation info (reuse variables declared above)
        int debugCalcPageStartY = 0;
        int debugCalcNaturalY = pageTopMarginPx;
        const int debugPageTopMarginPx = m_printRect.top();
        int debugCalcNaturalYTracker = pageTopMarginPx;
        
        QTextBlock block = doc->begin();
        int blockIdx = 0;
        while (block.isValid()) {
            // Get block dimensions
            QRectF blockRect = doc->documentLayout()->blockBoundingRect(block);
            int blockHeight = blockHeightPx(doc, block);
            int blockY = static_cast<int>(blockRect.top()) + editorTop;
            int blockX = editorLeft;
            int blockWidth = m_printRect.width();
            
            // Determine which page this block is on
            int blockPage = 0;
            int blockPageOffset = blockY - firstPageAbsY;
            if (blockPageOffset < 0) blockPageOffset = 0;
            
            const int pageAdvance = printableH + PAGE_GAP_PX;
            blockPage = blockPageOffset / pageAdvance;
            if (blockPage >= m_pageCount) blockPage = m_pageCount - 1;
            
            // Draw colored box for this block
            QColor blockColor = colors[colorIdx % colors.size()];
            p.fillRect(QRect(blockX, blockY, blockWidth, blockHeight), blockColor);
            p.setPen(QPen(Qt::black, 1));
            p.drawRect(QRect(blockX, blockY, blockWidth, blockHeight));
            
            // Draw height label
            QString heightLabel = QString::number(blockHeight) + "px";
            QFont labelFont("Courier New", 8);
            p.setFont(labelFont);
            p.setPen(Qt::black);
            p.drawText(QRect(blockX + 2, blockY + 2, blockWidth - 4, 20), Qt::AlignLeft | Qt::AlignTop, heightLabel);
            
            // Get top margin
            int topMargin = static_cast<int>(block.blockFormat().topMargin());
            if (topMargin > 0) {
                QString marginLabel = "M:" + QString::number(topMargin) + "px";
                p.drawText(QRect(blockX + 2, blockY + 18, blockWidth - 4, 20), Qt::AlignLeft | Qt::AlignTop, marginLabel);
            }
            
            // Track calculated positions for page breaks
            int baseMargin = topMargin;
            int existingPageBreakMargin = block.blockFormat().property(QTextFormat::UserProperty + 1).toInt();
            int marginForCalc = baseMargin - existingPageBreakMargin;
            int calcPrintableStartY = debugCalcPageStartY + debugPageTopMarginPx;
            int calcPosInPage = (debugCalcNaturalYTracker - calcPrintableStartY) + marginForCalc;
            
            // Draw calculated position line if on page 2+
            if (blockPage > 0) {
                // Draw a horizontal line showing where we calculated this block should be
                int calcAbsY = pageYOffset(blockPage) + m_printRect.top() + (debugCalcNaturalYTracker - debugPageTopMarginPx);
                p.setPen(QPen(Qt::magenta, 2, Qt::DashLine));
                p.drawLine(blockX, calcAbsY, blockX + blockWidth, calcAbsY);
                p.setPen(Qt::magenta);
                p.setFont(QFont("Courier New", 7));
                QString calcLabel = "CALC:" + QString::number(debugCalcNaturalYTracker);
                p.drawText(QRect(blockX + 2, calcAbsY - 15, blockWidth - 4, 12), Qt::AlignLeft | Qt::AlignTop, calcLabel);
            }
            
            // Update calculated tracking
            if (calcPosInPage + blockHeight > printableH && calcPosInPage > 0) {
                debugCalcPageStartY += m_pageRect.height() + PAGE_GAP_PX;
                debugCalcNaturalYTracker = debugCalcPageStartY + debugPageTopMarginPx + blockHeight;
            } else {
                debugCalcNaturalYTracker += marginForCalc + blockHeight;
            }
            
            colorIdx++;
            blockIdx++;
            block = block.next();
        }
        
        // Draw margin boxes and page gaps for each page
        for (int i = 0; i < m_pageCount; ++i) {
            int pageAbsY = pageYOffset(i);
            QRect pageAbsRect(x, pageAbsY, m_pageRect.width(), m_pageRect.height());
            QRect printableAbsRect = m_printRect.translated(x, pageAbsY);
            
            // Draw top margin
            QRect topMarginRect(pageAbsRect.left(), pageAbsRect.top(), pageAbsRect.width(), m_printRect.top());
            p.fillRect(topMarginRect, QColor(200, 100, 100, 80));  // Red-ish
            p.setPen(QPen(Qt::darkRed, 1));
            p.drawRect(topMarginRect);
            p.setFont(QFont("Courier New", 7));
            p.setPen(Qt::darkRed);
            p.drawText(topMarginRect, Qt::AlignCenter, "TOP");
            
            // Draw bottom margin
            int bottomMarginHeight = m_pageRect.height() - (m_printRect.top() + m_printRect.height());
            QRect bottomMarginRect(pageAbsRect.left(), printableAbsRect.bottom(), pageAbsRect.width(), bottomMarginHeight);
            p.fillRect(bottomMarginRect, QColor(100, 200, 100, 80));  // Green-ish
            p.setPen(QPen(Qt::darkGreen, 1));
            p.drawRect(bottomMarginRect);
            p.setFont(QFont("Courier New", 7));
            p.setPen(Qt::darkGreen);
            p.drawText(bottomMarginRect, Qt::AlignCenter, "BOTTOM");
            
            // Draw left margin
            QRect leftMarginRect(pageAbsRect.left(), printableAbsRect.top(), m_printRect.left(), m_printRect.height());
            p.fillRect(leftMarginRect, QColor(100, 100, 200, 80));  // Blue-ish
            p.setPen(QPen(Qt::darkBlue, 1));
            p.drawRect(leftMarginRect);
            p.setFont(QFont("Courier New", 7));
            p.setPen(Qt::darkBlue);
            p.drawText(leftMarginRect, Qt::AlignCenter, "L");
            
            // Draw right margin
            int rightMarginLeft = printableAbsRect.right();
            int rightMarginWidth = pageAbsRect.right() - rightMarginLeft;
            QRect rightMarginRect(rightMarginLeft, printableAbsRect.top(), rightMarginWidth, m_printRect.height());
            p.fillRect(rightMarginRect, QColor(200, 200, 100, 80));  // Yellow-ish
            p.setPen(QPen(Qt::darkYellow, 1));
            p.drawRect(rightMarginRect);
            p.setFont(QFont("Courier New", 7));
            p.setPen(Qt::darkYellow);
            p.drawText(rightMarginRect, Qt::AlignCenter, "R");
            
            // Draw printable area border
            p.setPen(QPen(Qt::darkMagenta, 2, Qt::DashLine));
            p.setBrush(Qt::NoBrush);
            p.drawRect(printableAbsRect);
            
            // Label the printable area
            QString pageLabel = "Page " + QString::number(i + 1) + " Printable";
            QFont labelFont("Courier New", 8);
            p.setFont(labelFont);
            p.setPen(Qt::darkMagenta);
            p.drawText(printableAbsRect.adjusted(2, 2, -2, -2), Qt::AlignTop | Qt::AlignLeft, pageLabel);
            
            // Draw page gap (if not the last page)
            if (i < m_pageCount - 1) {
                int gapY = pageAbsY + m_pageRect.height();
                QRect gapRect(x, gapY, m_pageRect.width(), PAGE_GAP_PX);
                p.fillRect(gapRect, QColor(150, 150, 150, 100));  // Gray
                p.setPen(QPen(Qt::gray, 1, Qt::DotLine));
                p.drawRect(gapRect);
                p.setFont(QFont("Courier New", 7));
                p.setPen(Qt::gray);
                p.drawText(gapRect, Qt::AlignCenter, "GAP");
            }
        }
    }
}

void PageView::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    layoutPages();
}

void PageView::setDebugMode(bool enabled)
{
    if (m_debugMode != enabled) {
        m_debugMode = enabled;
        update();  // Redraw with or without debug visualization
    }
}


void PageView::layoutPages()
{
    // qDebug() << "[PageView] === layoutPages START ===\";
    // qDebug() << "[PageView] pageCount:" << m_pageCount;
    // qDebug() << "[PageView] pageRect:" << m_pageRect;
    // qDebug() << "[PageView] printRect (relative):" << m_printRect;
    // qDebug() << "[PageView] pageGap:" << PAGE_GAP_PX;
    
    // Base page position centered horizontally, start near top
    int x = (width() - m_pageRect.width()) / 2;
    if (x < PAGE_HORIZONTAL_PADDING) x = PAGE_HORIZONTAL_PADDING;
    int startY = PAGE_HORIZONTAL_PADDING;
    m_pageRect.moveTopLeft(QPoint(x, startY));

    // Printable width/height
    const int printableW = m_printRect.width();
    const int printableH = m_printRect.height();
    // qDebug() << "[PageView] printableW:" << printableW << "printableH:" << printableH;

    // Compute first page printable rect (absolute position)
    QRect firstPrint = m_printRect.translated(x, startY);
    // qDebug() << "[PageView] firstPrint (absolute):" << firstPrint;

    // Editor height spans all pages (full page height including margins) PLUS gaps between pages
    // This ensures editor Y coordinates align with painted page positions
    int totalEditorHeight = m_pageRect.height() * m_pageCount + PAGE_GAP_PX * (m_pageCount - 1);
    // qDebug() << "[PageView] totalEditorHeight:" << totalEditorHeight;
    
    m_editor->setGeometry(QRect(firstPrint.left(), firstPrint.top(), printableW, totalEditorHeight));
    // qDebug() << "[PageView] editor geometry:" << m_editor->geometry();
    
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
    // qDebug() << "[PageView] widget size:" << size();
    // qDebug() << "[PageView] === layoutPages END ===";
}

void PageView::updatePagination()
{
    // Recompute pages by simulating pagination without mutating the document.
    // Uses ABSOLUTE coordinates: naturalY starts at pageTopMarginPx, pageStartY at 0
    const int printableH = m_printRect.height();
    const int pageTopMarginPx = m_printRect.top();
    const int pageHeight = m_pageRect.height();
    
    int pages = 1;
    int pageStartY = 0;              // Physical top of current page
    int naturalY = pageTopMarginPx;  // Current position (starts at page 1 printable top)

    QTextDocument *doc = m_editor->document();
    QTextBlock block = doc->begin();
    while (block.isValid()) {
        int blockHeight = blockHeightPx(doc, block);
        int blockTopMargin = static_cast<int>(block.blockFormat().topMargin());

        // Position within current page's printable area
        int printableStartY = pageStartY + pageTopMarginPx;
        int posInPage = (naturalY - printableStartY) + blockTopMargin;

        if (posInPage + blockHeight > printableH && posInPage > 0) {
            // move to next page
            pages++;
            pageStartY += pageHeight + PAGE_GAP_PX;
            naturalY = pageStartY + pageTopMarginPx + blockTopMargin + blockHeight;
        } else {
            naturalY += blockTopMargin + blockHeight;
        }

        block = block.next();
    }

    if (pages < 1) pages = 1;
    // qDebug() << "[PageView] updatePagination: pages=" << pages << "naturalY=" << naturalY << "printableH=" << printableH;
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

double PageView::inchToPxX(double inches) const { return inches * dpiX() * m_zoomFactor; }
double PageView::inchToPxY(double inches) const { return inches * dpiY() * m_zoomFactor; }

void PageView::recalculatePageMetrics()
{
    int pageW = static_cast<int>(inchToPxX(PAGE_WIDTH_INCHES));
    int pageH = static_cast<int>(inchToPxY(PAGE_HEIGHT_INCHES));
    m_pageRect = QRect(0, 0, pageW, pageH);

    int left = static_cast<int>(inchToPxX(MARGIN_LEFT_INCHES));
    int right = static_cast<int>(inchToPxX(MARGIN_RIGHT_INCHES));
    int top = static_cast<int>(inchToPxY(MARGIN_TOP_INCHES));
    int bottom = static_cast<int>(inchToPxY(MARGIN_BOTTOM_INCHES));
    m_printRect = QRect(left, top, pageW - left - right, pageH - top - bottom);
}

void PageView::applyZoom()
{
    m_zoomFactor = std::pow(ZOOM_STEP_MULTIPLIER, m_zoomSteps);

    QFont editorFont = m_editor->font();
    editorFont.setPointSizeF(BASE_FONT_POINT_SIZE * m_zoomFactor);
    m_editor->setFont(editorFont);

    recalculatePageMetrics();
    m_editor->formatDocument();
    enforcePageBreaks();
    updatePagination();
    layoutPages();
    update();
}

void PageView::zoomInView()
{
    if (m_zoomSteps >= MAX_ZOOM_STEPS) {
        return;
    }
    ++m_zoomSteps;
    applyZoom();
}

void PageView::zoomOutView()
{
    if (m_zoomSteps <= MIN_ZOOM_STEPS) {
        return;
    }
    --m_zoomSteps;
    applyZoom();
}

void PageView::resetZoom()
{
    if (m_zoomSteps == 0) {
        return;
    }
    m_zoomSteps = 0;
    applyZoom();
}

void PageView::setZoomSteps(int steps)
{
    const int clamped = qBound(MIN_ZOOM_STEPS, steps, MAX_ZOOM_STEPS);
    if (clamped == m_zoomSteps) {
        return;
    }

    m_zoomSteps = clamped;
    applyZoom();
}

int PageView::pageYOffset(int pageIndex) const
{
    // Y offset for page N: startY + (pageHeight + gap) * N
    return PAGE_HORIZONTAL_PADDING + (m_pageRect.height() + PAGE_GAP_PX) * pageIndex;
}

int PageView::pagePrintableStartY(int pageIndex) const
{
    return pageYOffset(pageIndex) + m_printRect.top();
}

bool PageView::saveToFile(const QString &filePath)
{
    qDebug() << "[PageView] Saving to:" << filePath;
    const bool ok = ScreenplayIO::saveDocument(m_editor, filePath);

    if (!ok) {
        qDebug() << "[PageView] Failed to write file";
        return false;
    }

    int linesSaved = 0;
    for (QTextBlock block = m_editor->document()->begin(); block.isValid(); block = block.next()) {
        ++linesSaved;
    }
    qDebug() << "[PageView] Saved" << linesSaved << "lines";
    return true;
}

bool PageView::loadFromFile(const QString &filePath)
{
    qDebug() << "[PageView] Loading from:" << filePath;
    m_loading = true; // Prevent enforcePageBreaks during load

    int loadedLineCount = 0;
    const bool ok = ScreenplayIO::loadDocument(m_editor, filePath, loadedLineCount);
    if (!ok) {
        qDebug() << "[PageView] Failed to load screenplay";
        m_loading = false;
        return false;
    }

    m_editor->moveCursor(QTextCursor::Start);
    qDebug() << "[PageView::loadFromFile] Before formatDocument, isUndoAvailable:" << m_editor->document()->isUndoAvailable();
    m_editor->formatDocument();
    qDebug() << "[PageView::loadFromFile] After formatDocument, isUndoAvailable:" << m_editor->document()->isUndoAvailable();
    
    // Clear loading flag, disconnect textChanged, run enforcePageBreaks once, clear undo, reconnect
    QTimer::singleShot(0, this, [this, loadedLineCount]() {
        m_loading = false;
        qDebug() << "[PageView::loadFromFile] Disconnecting textChanged signal";
        disconnect(m_editor, &QTextEdit::textChanged, this, &PageView::enforcePageBreaks);
        
        enforcePageBreaks(); // Run once with undo disabled
        
        m_editor->document()->clearUndoRedoStacks();
        qDebug() << "[PageView::loadFromFile] Loaded" << loadedLineCount << "lines, loading complete, undo stack cleared, isUndoAvailable:" << m_editor->document()->isUndoAvailable();
        
        qDebug() << "[PageView::loadFromFile] Reconnecting textChanged signal";
        connect(m_editor, &QTextEdit::textChanged, this, &PageView::enforcePageBreaks);
    });
    
    return true;
}

bool PageView::exportToPdf(const QString &filePath)
{
    PdfExporter::Settings settings;
    settings.pageCount = m_pageCount;
    settings.pageGapPx = PAGE_GAP_PX;
    settings.printableWidthPx = m_printRect.width();
    settings.printableHeightPx = printableHeightPerPage();
    settings.topMarginPx = m_printRect.top();

    settings.marginLeftInches = MARGIN_LEFT_INCHES;
    settings.marginRightInches = MARGIN_RIGHT_INCHES;
    settings.marginTopInches = MARGIN_TOP_INCHES;
    settings.marginBottomInches = MARGIN_BOTTOM_INCHES;

    settings.resolutionDpi = PDF_RESOLUTION_DPI;
    settings.pageNumberFontSize = PDF_PAGE_NUM_FONT_SIZE;
    settings.pageNumberWidth = PDF_PAGE_NUM_WIDTH;
    settings.pageNumberHeight = PDF_PAGE_NUM_HEIGHT;
    settings.pageNumberRightOffset = PDF_PAGE_NUM_RIGHT_OFFSET;
    settings.pageNumberTopOffset = PDF_PAGE_NUM_TOP_OFFSET;

    return PdfExporter::exportDocumentToPdf(m_editor->document(), filePath, settings);
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
    
    // Temporarily disconnect textChanged to prevent re-triggering during modifications
    disconnect(m_editor, &QTextEdit::textChanged, this, &PageView::enforcePageBreaks);
    
    qDebug() << "[PageView] enforcePageBreaks called";
    
    QTextDocument *doc = m_editor->document();
    const int printableH = printableHeightPerPage();
    const int pageTopMarginPx = m_printRect.top();
    const int pageBottomMarginPx = m_pageRect.height() - (m_printRect.top() + m_printRect.height());
    
    // Calculate which blocks need page break margins and what those margins should be
    // Key: block position, Value: required top margin for page break
    QMap<int, int> requiredPageBreakMargins;
    
    // Use ABSOLUTE coordinates throughout:
    // - naturalY starts at pageTopMarginPx (96) - first content position below page 1's top margin
    // - pageStartY starts at 0 - physical top of page 1
    // - Page boundaries: pageHeight (1056) + gap (30)
    // - Printable area on page N starts at: pageStartY + pageTopMarginPx
    
    int naturalY = pageTopMarginPx;  // Start at page 1's printable top (absolute)
    int pageStartY = 0;              // Physical top of current page
    
    int blockIdx = 0;
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat fmt = block.blockFormat();
        int blockHeight = blockHeightPx(doc, block);
        
        // Get the block's base margin (element type margin, not pagination margin)
        int baseMargin = static_cast<int>(fmt.topMargin());
        
        // For this calculation, use only the base margin (ignore any existing page break margin)
        int existingPageBreakMargin = fmt.property(QTextFormat::UserProperty + 1).toInt();
        int marginForCalculation = baseMargin - existingPageBreakMargin;
        
        // Get actual Qt block position for debug
        qreal qtBlockY = doc->documentLayout()->blockBoundingRect(block).top();
        
        // Calculate position within current page's printable area
        // Printable area starts at pageStartY + pageTopMarginPx
        int printableStartY = pageStartY + pageTopMarginPx;
        int posInPage = (naturalY - printableStartY) + marginForCalculation;
        
        qDebug() << "[Block" << blockIdx << "] naturalY=" << naturalY << "qtBlockY=" << qtBlockY
                 << "pageStartY=" << pageStartY << "printableStartY=" << printableStartY
                 << "marginForCalc=" << marginForCalculation << "blockHeight=" << blockHeight
                 << "posInPage=" << posInPage << "overflow?" << (posInPage + blockHeight > printableH);
        
        // Does it overflow the page?
        if (posInPage + blockHeight > printableH && posInPage > 0) {
            // Block needs to be pushed to the next page
            // Next page starts at: pageStartY + pageHeight + gap
            int nextPageStartY = pageStartY + m_pageRect.height() + PAGE_GAP_PX;
            // Next page's printable area starts at: nextPageStartY + pageTopMarginPx
            int nextPagePrintableStart = nextPageStartY + pageTopMarginPx;
            
            // The page break margin pushes the block from its current position to the next page's printable start
            // Block top is at: naturalY
            // We want block top at: nextPagePrintableStart
            // So we need extra margin of: nextPagePrintableStart - naturalY
            int pageBreakMargin = nextPagePrintableStart - naturalY;
            // For pages beyond the first, remove the base margin contribution to prevent drift
            if (pageStartY > 0) {
                pageBreakMargin -= marginForCalculation;
            }
            
            qDebug() << "[enforcePageBreaks] Block" << blockIdx << "OVERFLOWS:";
            qDebug() << "  pageStartY=" << pageStartY << "nextPageStartY=" << nextPageStartY;
            qDebug() << "  nextPagePrintableStart=" << nextPagePrintableStart;
            qDebug() << "  naturalY=" << naturalY << "qtBlockY=" << qtBlockY << "marginForCalculation=" << marginForCalculation;
            qDebug() << "  naturalY + marginForCalculation=" << (naturalY + marginForCalculation);
            qDebug() << "  pageBreakMargin=" << pageBreakMargin;
            
            requiredPageBreakMargins[block.position()] = pageBreakMargin;
            
            // Move to next page tracking
            pageStartY = nextPageStartY;
            // naturalY = where this block ends on the new page (printable start + block height)
            naturalY = nextPagePrintableStart + blockHeight;
        } else {
            naturalY += marginForCalculation + blockHeight;
        }
        blockIdx++;
    }
    
    // Now check current state and only modify if needed
    bool needsChanges = false;
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat fmt = block.blockFormat();
        int currentPageBreakMargin = fmt.property(QTextFormat::UserProperty + 1).toInt();
        int requiredMargin = requiredPageBreakMargins.value(block.position(), 0);
        
        if (currentPageBreakMargin != requiredMargin) {
            needsChanges = true;
            break;
        }
    }
    
    // Only modify document if changes are needed
    if (needsChanges) {
        QTextCursor cursor(doc);
        cursor.beginEditBlock();
        
        for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
            QTextBlockFormat fmt = block.blockFormat();
            int currentPageBreakMargin = fmt.property(QTextFormat::UserProperty + 1).toInt();
            int requiredMargin = requiredPageBreakMargins.value(block.position(), 0);
            
            if (currentPageBreakMargin != requiredMargin) {
                // Need to update this block's margin
                int baseMargin = static_cast<int>(fmt.topMargin()) - currentPageBreakMargin;
                int newTotalMargin = baseMargin + requiredMargin;
                
                cursor.setPosition(block.position());
                fmt.setTopMargin(newTotalMargin);
                fmt.setProperty(QTextFormat::UserProperty + 1, requiredMargin);
                cursor.setBlockFormat(fmt);
            }
        }
        
        cursor.endEditBlock();
    }
    
    // Reconnect textChanged and reset flag
    connect(m_editor, &QTextEdit::textChanged, this, &PageView::enforcePageBreaks);
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
