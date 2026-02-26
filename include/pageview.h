#pragma once
#include <QWidget>
#include <QRect>
#include <QScrollArea>
class ScriptEditor;

class PageView : public QWidget {
    Q_OBJECT
public:
    explicit PageView(QWidget *parent = nullptr);
    ScriptEditor* editor() const { return m_editor; }
    int pageGapPx() const { return PAGE_GAP_PX; }
    int pageCount() const { return m_pageCount; }
    int printableHeight() const; // Height of printable area per page (px)
    int pageHeight() const { return m_pageRect.height(); } // Full page height including margins (px)
    int pageTopMarginPx() const { return m_printRect.top(); } // Top margin from page edge to printable area (px)
    int pageBottomMarginPx() const { return m_pageRect.height() - (m_printRect.top() + m_printRect.height()); } // Bottom margin (px)
    int pagePrintableStartY(int pageIndex) const; // Absolute Y of printable start for page N
    bool debugMode() const { return m_debugMode; }
    void setDebugMode(bool enabled);
    
    bool saveToFile(const QString &filePath);
    bool loadFromFile(const QString &filePath);
    bool exportToPdf(const QString &filePath);
    int zoomSteps() const { return m_zoomSteps; }
    void setZoomSteps(int steps);

public slots:
    void scrollToCursor();
    void zoomInView();
    void zoomOutView();
    void resetZoom();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    double dpiX() const;
    double dpiY() const;
    double inchToPxX(double inches) const;
    double inchToPxY(double inches) const;
    void layoutPages();
    void updatePagination();
    void recalculatePageMetrics();
    void applyZoom();
    int pageYOffset(int pageIndex) const; // Y offset for page N with gaps
    void enforcePageBreaks(); // Insert spacing at page boundaries
    int printableHeightPerPage() const;

    // Page dimensions in inches (US Letter)
    static constexpr double PAGE_WIDTH_INCHES = 8.5;
    static constexpr double PAGE_HEIGHT_INCHES = 11.0;
    
    // Page margins in inches
    static constexpr double MARGIN_LEFT_INCHES = 1.5;
    static constexpr double MARGIN_RIGHT_INCHES = 1.0;
    static constexpr double MARGIN_TOP_INCHES = 1.0;
    static constexpr double MARGIN_BOTTOM_INCHES = 1.0;
    
    // Display constants
    static constexpr int PAGE_GAP_PX = 30;              // Vertical gap between pages
    static constexpr int PAGE_HORIZONTAL_PADDING = 20;  // Minimum horizontal padding
    static constexpr int WIDGET_HORIZONTAL_PADDING = 40;
    static constexpr int WIDGET_VERTICAL_PADDING = 40;
    static constexpr double DEFAULT_DPI = 96.0;         // Fallback DPI if screen unavailable
    
    // Background colors
    static constexpr int BG_GRAY_VALUE = 28;            // Main app dark gray (#1C1C1E)
    static constexpr int BORDER_GRAY_VALUE = 70;        // Soft page border in dark mode
    
    // Page number display
    static constexpr int PAGE_NUM_FONT_SIZE = 10;
    static constexpr int PAGE_NUM_TOP_OFFSET = 10;
    static constexpr int PAGE_NUM_RIGHT_MARGIN = 20;
    
    // PDF export settings
    static constexpr int PDF_RESOLUTION_DPI = 300;
    static constexpr int PDF_PAGE_NUM_FONT_SIZE = 12;
    static constexpr int PDF_PAGE_NUM_WIDTH = 80;
    static constexpr int PDF_PAGE_NUM_HEIGHT = 30;
    static constexpr int PDF_PAGE_NUM_RIGHT_OFFSET = 100;
    static constexpr int PDF_PAGE_NUM_TOP_OFFSET = 20;
    
    // Scroll behavior
    static constexpr int SCROLL_X_MARGIN = 40;
    static constexpr int SCROLL_Y_MARGIN = 120;
    static constexpr double BASE_FONT_POINT_SIZE = 15.0;
    static constexpr int MIN_ZOOM_STEPS = -8;
    static constexpr int MAX_ZOOM_STEPS = 20;
    static constexpr double ZOOM_STEP_MULTIPLIER = 1.1;

    QRect m_pageRect;       // Single page size (8.5x11")
    QRect m_printRect;      // Printable area inside one page
    int m_pageCount = 1;    // Number of pages based on content
    bool m_enforcingBreaks = false; // Guard against recursive calls
    bool m_loading = false; // True during document load/initialization
    ScriptEditor* m_editor; // Editor placed inside printable area
    bool m_debugMode = false;
    int m_zoomSteps = 0;
    double m_zoomFactor = 1.0;
};
