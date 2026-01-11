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
    
    bool saveToFile(const QString &filePath);
    bool loadFromFile(const QString &filePath);
    bool exportToPdf(const QString &filePath);
    void loadSampleText(); // Extracted sample text loading

public slots:
    void scrollToCursor();

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
    int pageYOffset(int pageIndex) const; // Y offset for page N with gaps
    void enforcePageBreaks(); // Insert spacing at page boundaries
    int printableHeightPerPage() const;

    QRect m_pageRect;       // Single page size (8.5x11")
    QRect m_printRect;      // Printable area inside one page
    int m_pageCount = 1;    // Number of pages based on content
    static constexpr int m_pageGap = 30; // Vertical gap between pages
    bool m_enforcingBreaks = false; // Guard against recursive calls
    ScriptEditor* m_editor; // Editor placed inside printable area
};
