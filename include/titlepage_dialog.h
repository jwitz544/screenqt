#pragma once
#include <QDialog>
#include "documentsettings.h"

class QCheckBox;
class QGroupBox;
class QLineEdit;

class TitlePageDialog : public QDialog {
    Q_OBJECT
public:
    explicit TitlePageDialog(const DocumentSettings &settings, QWidget *parent = nullptr);
    DocumentSettings result() const;

private:
    QCheckBox  *m_hasTitlePage   = nullptr;
    QGroupBox  *m_fieldsGroup    = nullptr;
    QLineEdit  *m_title          = nullptr;
    QLineEdit  *m_author         = nullptr;
    QLineEdit  *m_credit         = nullptr;
    QLineEdit  *m_contact        = nullptr;
    QLineEdit  *m_draftDate      = nullptr;
    QLineEdit  *m_wgaNumber      = nullptr;
    QCheckBox  *m_pageNumbering  = nullptr;
    QCheckBox  *m_numberTitlePg  = nullptr;
};
