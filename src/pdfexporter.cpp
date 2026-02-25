#include "pdfexporter.h"

#include <QAbstractTextDocumentLayout>
#include <QMarginsF>
#include <QPainter>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QtMath>

namespace PdfExporter {

bool exportDocumentToPdf(QTextDocument *document, const QString &filePath, const Settings &settings)
{
    if (!document || settings.printableWidthPx <= 0 || settings.printableHeightPx <= 0) {
        return false;
    }

    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize::Letter);
    writer.setResolution(settings.resolutionDpi);

    QMarginsF margins(settings.marginLeftInches,
                      settings.marginTopInches,
                      settings.marginRightInches,
                      settings.marginBottomInches);
    writer.setPageMargins(margins, QPageLayout::Inch);

    QPainter painter(&writer);
    QRect paintRect = writer.pageLayout().paintRectPixels(writer.resolution());

    qreal scaleX = static_cast<qreal>(paintRect.width()) / settings.printableWidthPx;
    qreal scaleY = static_cast<qreal>(paintRect.height()) / settings.printableHeightPx;
    qreal scale = qMin(scaleX, scaleY);

    const int totalPages = qMax(1, settings.pageCount);

    for (int pageNum = 0; pageNum < totalPages; ++pageNum) {
        if (pageNum > 0) {
            writer.newPage();
        }

        painter.save();
        painter.scale(scale, scale);

        int pageContentStart = 0;
        if (pageNum > 0) {
            int currentY = 0;
            QTextBlock block = document->begin();
            while (block.isValid()) {
                QTextLayout *layout = block.layout();
                int blockHeight = static_cast<int>(std::ceil(layout->boundingRect().height()));
                int blockTopMargin = static_cast<int>(block.blockFormat().topMargin());
                int contentStartY = currentY + blockTopMargin;

                int expectedPageStart = (settings.printableHeightPx + settings.pageGapPx) * pageNum;
                if (contentStartY >= expectedPageStart) {
                    pageContentStart = contentStartY;
                    break;
                }

                currentY += blockTopMargin + blockHeight;
                block = block.next();
            }
        }

        painter.translate(0, -pageContentStart);
        QRectF clipRect(0, pageContentStart, settings.printableWidthPx, settings.printableHeightPx);
        painter.setClipRect(clipRect);

        QAbstractTextDocumentLayout::PaintContext context;
        context.clip = clipRect;
        context.palette.setColor(QPalette::Text, Qt::black);
        document->documentLayout()->draw(&painter, context);

        painter.restore();

        painter.save();
        QFont pageNumFont("Courier New", settings.pageNumberFontSize);
        pageNumFont.setPointSizeF(settings.pageNumberFontSize);
        painter.setFont(pageNumFont);
        painter.setPen(Qt::black);

        QString pageNumStr = QString::number(pageNum + 1) + ".";
        QRectF pageNumRect(paintRect.width() - settings.pageNumberRightOffset,
                           settings.pageNumberTopOffset,
                           settings.pageNumberWidth,
                           settings.pageNumberHeight);
        painter.drawText(pageNumRect, Qt::AlignRight | Qt::AlignTop, pageNumStr);
        painter.restore();
    }

    painter.end();
    return true;
}

} // namespace PdfExporter
