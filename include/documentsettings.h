#pragma once

#include <QString>

struct TitlePageData {
    QString title;
    QString author;
    QString credit;
    QString contact;
    QString draftDate;
    QString wgaNumber;
};

struct PageNumberingOptions {
    bool enabled = true;
    int startNumber = 1;
    bool numberTitlePage = false;
};

struct DocumentSettings {
    bool hasTitlePage = false;
    TitlePageData titlePage;
    PageNumberingOptions pageNumbering;
};
