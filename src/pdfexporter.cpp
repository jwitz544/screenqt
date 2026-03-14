#include "pdfexporter.h"
#include "scripteditor.h"

#include <QAbstractTextDocumentLayout>
#include <QMarginsF>
#include <QPainter>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QStringList>
#include <QSet>
#include <QtMath>

namespace PdfExporter {

namespace {
struct PdfContinuationMarker {
    int pageIndex = 0;
    bool isTop = false;
    QString text;
};

QVector<PdfContinuationMarker> buildContinuationMarkers(QTextDocument *document, const Settings &settings)
{
    QVector<PdfContinuationMarker> markers;
    if (!document) {
        return markers;
    }

    QSet<QString> markerKeys;
    const int printableH = settings.printableHeightPx;
    const int pageAdvance = settings.printableHeightPx + settings.pageGapPx;
    int pageStartY = 0;
    int naturalY = 0;

    QTextBlock block = document->begin();
    while (block.isValid()) {
        QTextLayout *layout = block.layout();
        const int blockHeight = static_cast<int>(std::ceil(layout->boundingRect().height()));
        const int blockTopMargin = static_cast<int>(block.blockFormat().topMargin());
        const int posInPage = (naturalY - pageStartY) + blockTopMargin;

        if (posInPage + blockHeight > printableH && posInPage > 0) {
            if (block.userState() == static_cast<int>(ScriptEditor::Dialogue)) {
                const int previousPageIndex = pageStartY / pageAdvance;
                const int nextPageIndex = previousPageIndex + 1;

                const QString lowerKey = QString("%1:%2").arg(previousPageIndex).arg("MORE");
                if (!markerKeys.contains(lowerKey)) {
                    markerKeys.insert(lowerKey);
                    markers.append(PdfContinuationMarker{previousPageIndex, false, "(MORE)"});
                }

                const QString upperKey = QString("%1:%2").arg(nextPageIndex).arg("CONTD");
                if (!markerKeys.contains(upperKey)) {
                    markerKeys.insert(upperKey);
                    markers.append(PdfContinuationMarker{nextPageIndex, true, "(CONT'D)"});
                }
            }

            pageStartY += pageAdvance;
            naturalY = pageStartY + blockTopMargin + blockHeight;
        } else {
            naturalY += blockTopMargin + blockHeight;
        }

        block = block.next();
    }

    return markers;
}
}

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
    const QVector<PdfContinuationMarker> continuationMarkers = buildContinuationMarkers(document, settings);

    const int bodyPages = qMax(1, settings.pageCount);
    const bool hasTitlePage = settings.documentSettings.hasTitlePage;
    const int totalPages = bodyPages + (hasTitlePage ? 1 : 0);

    for (int renderPage = 0; renderPage < totalPages; ++renderPage) {
        if (renderPage > 0) {
            writer.newPage();
        }

        if (hasTitlePage && renderPage == 0) {
            painter.save();
            const TitlePageData &titlePage = settings.documentSettings.titlePage;

            QFont titleFont("Courier New", 20, QFont::Bold);
            painter.setFont(titleFont);
            painter.setPen(Qt::black);

            const QString title = titlePage.title.trimmed().isEmpty() ? QStringLiteral("UNTITLED") : titlePage.title.trimmed();
            QRectF titleRect(0, paintRect.height() * 0.28, paintRect.width(), 120);
            painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter, title.toUpper());

            QFont metaFont("Courier New", 12);
            painter.setFont(metaFont);
            const QString credit = titlePage.credit.trimmed().isEmpty() ? QStringLiteral("Written by") : titlePage.credit.trimmed();
            const QString author = titlePage.author.trimmed();
            QRectF creditRect(0, paintRect.height() * 0.42, paintRect.width(), 90);
            painter.drawText(creditRect, Qt::AlignHCenter | Qt::AlignVCenter,
                             author.isEmpty() ? credit : QString("%1\n%2").arg(credit, author));

            const QString contact = titlePage.contact.trimmed();
            const QString draftDate = titlePage.draftDate.trimmed();
            const QString wgaNumber = titlePage.wgaNumber.trimmed();
            QRectF footerRect(paintRect.width() * 0.62, paintRect.height() * 0.78,
                              paintRect.width() * 0.34, paintRect.height() * 0.16);
            QStringList footerLines;
            if (!contact.isEmpty()) {
                footerLines << contact;
            }
            if (!draftDate.isEmpty()) {
                footerLines << draftDate;
            }
            if (!wgaNumber.isEmpty()) {
                footerLines << wgaNumber;
            }
            if (!footerLines.isEmpty()) {
                painter.drawText(footerRect, Qt::AlignLeft | Qt::AlignTop, footerLines.join('\n'));
            }
            painter.restore();

            if (settings.documentSettings.pageNumbering.enabled && settings.documentSettings.pageNumbering.numberTitlePage) {
                painter.save();
                QFont pageNumFont("Courier New", settings.pageNumberFontSize);
                pageNumFont.setPointSizeF(settings.pageNumberFontSize);
                painter.setFont(pageNumFont);
                painter.setPen(Qt::black);

                const int pageNumber = qMax(1, settings.documentSettings.pageNumbering.startNumber);
                QString pageNumStr = QString::number(pageNumber) + ".";
                QRectF pageNumRect(paintRect.width() - settings.pageNumberRightOffset,
                                   settings.pageNumberTopOffset,
                                   settings.pageNumberWidth,
                                   settings.pageNumberHeight);
                painter.drawText(pageNumRect, Qt::AlignRight | Qt::AlignTop, pageNumStr);
                painter.restore();
            }
            continue;
        }

        const int bodyPageIndex = hasTitlePage ? (renderPage - 1) : renderPage;

        painter.save();
        painter.scale(scale, scale);

        int pageContentStart = 0;
        if (bodyPageIndex > 0) {
            int currentY = 0;
            QTextBlock block = document->begin();
            while (block.isValid()) {
                QTextLayout *layout = block.layout();
                int blockHeight = static_cast<int>(std::ceil(layout->boundingRect().height()));
                int blockTopMargin = static_cast<int>(block.blockFormat().topMargin());
                int contentStartY = currentY + blockTopMargin;

                int expectedPageStart = (settings.printableHeightPx + settings.pageGapPx) * bodyPageIndex;
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

        if (settings.documentSettings.pageNumbering.enabled) {
            painter.save();
            QFont pageNumFont("Courier New", settings.pageNumberFontSize);
            pageNumFont.setPointSizeF(settings.pageNumberFontSize);
            painter.setFont(pageNumFont);
            painter.setPen(Qt::black);

            const int startNumber = qMax(1, settings.documentSettings.pageNumbering.startNumber);
            const int pageNumber = startNumber + bodyPageIndex;
            QString pageNumStr = QString::number(pageNumber) + ".";
            QRectF pageNumRect(paintRect.width() - settings.pageNumberRightOffset,
                               settings.pageNumberTopOffset,
                               settings.pageNumberWidth,
                               settings.pageNumberHeight);
            painter.drawText(pageNumRect, Qt::AlignRight | Qt::AlignTop, pageNumStr);
            painter.restore();
        }

        for (const PdfContinuationMarker &marker : continuationMarkers) {
            if (marker.pageIndex != bodyPageIndex) {
                continue;
            }

            painter.save();
            QFont markerFont("Courier New", 10);
            painter.setFont(markerFont);
            painter.setPen(Qt::black);
            const qreal y = marker.isTop ? 6.0 : (paintRect.height() - 24.0);
            QRectF textRect(0, y, paintRect.width(), 18);
            painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, marker.text);
            painter.restore();
        }
    }

    painter.end();
    return true;
}

} // namespace PdfExporter
