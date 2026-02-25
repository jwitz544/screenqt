#pragma once

#include <QString>

class QTextDocument;

namespace PdfExporter {

struct Settings {
    int pageCount = 1;
    int pageGapPx = 30;
    int printableWidthPx = 0;
    int printableHeightPx = 0;
    int topMarginPx = 0;

    double marginLeftInches = 1.5;
    double marginRightInches = 1.0;
    double marginTopInches = 1.0;
    double marginBottomInches = 1.0;

    int resolutionDpi = 300;
    int pageNumberFontSize = 12;
    int pageNumberWidth = 80;
    int pageNumberHeight = 30;
    int pageNumberRightOffset = 100;
    int pageNumberTopOffset = 20;
};

bool exportDocumentToPdf(QTextDocument *document, const QString &filePath, const Settings &settings);

} // namespace PdfExporter
