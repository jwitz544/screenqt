#include "titlepage_dialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

static QString dialogStyleSheet()
{
    return QStringLiteral(
        "QDialog { background: #1E1E1E; color: #D4D4D4; }"
        "QWidget { color: #D4D4D4; background: #1E1E1E; }"
        "QGroupBox {"
        "  border: 1px solid #3C3C3C;"
        "  border-radius: 4px;"
        "  margin-top: 8px;"
        "  font-size: 11px;"
        "  font-weight: 700;"
        "  color: #858585;"
        "  letter-spacing: 1px;"
        "}"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
        "QLabel { color: #CCCCCC; font-size: 12px; }"
        "QLineEdit {"
        "  background: #3C3C3C;"
        "  border: 1px solid #3C3C3C;"
        "  border-radius: 3px;"
        "  padding: 4px 8px;"
        "  color: #D4D4D4;"
        "  font-size: 12px;"
        "}"
        "QLineEdit:focus { border-color: #007ACC; }"
        "QCheckBox { color: #CCCCCC; font-size: 12px; spacing: 6px; }"
        "QCheckBox::indicator {"
        "  width: 14px; height: 14px;"
        "  border: 1px solid #686868; border-radius: 2px; background: transparent;"
        "}"
        "QCheckBox::indicator:checked { background: #007ACC; border-color: #007ACC; }"
        "QDialogButtonBox QPushButton {"
        "  background: #3C3C3C;"
        "  color: #CCCCCC;"
        "  border: 1px solid #3C3C3C;"
        "  border-radius: 3px;"
        "  padding: 5px 18px;"
        "  font-size: 12px;"
        "  min-width: 72px;"
        "}"
        "QDialogButtonBox QPushButton:hover { background: #4C4C4C; border-color: #686868; }"
        "QDialogButtonBox QPushButton:default { background: #0E639C; border-color: #007ACC; color: #FFFFFF; }"
    );
}

TitlePageDialog::TitlePageDialog(const DocumentSettings &settings, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Title Page");
    setMinimumWidth(420);
    setStyleSheet(dialogStyleSheet());

    auto *root = new QVBoxLayout(this);
    root->setSpacing(12);
    root->setContentsMargins(16, 16, 16, 16);

    // Enable toggle
    m_hasTitlePage = new QCheckBox("Include title page in export", this);
    m_hasTitlePage->setChecked(settings.hasTitlePage);
    root->addWidget(m_hasTitlePage);

    // Fields group
    m_fieldsGroup = new QGroupBox("TITLE PAGE FIELDS", this);
    auto *form = new QFormLayout(m_fieldsGroup);
    form->setSpacing(8);
    form->setContentsMargins(12, 16, 12, 12);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_title     = new QLineEdit(settings.titlePage.title, this);
    m_author    = new QLineEdit(settings.titlePage.author, this);
    m_credit    = new QLineEdit(settings.titlePage.credit, this);
    m_contact   = new QLineEdit(settings.titlePage.contact, this);
    m_draftDate = new QLineEdit(settings.titlePage.draftDate, this);
    m_wgaNumber = new QLineEdit(settings.titlePage.wgaNumber, this);

    m_title->setPlaceholderText("My Screenplay");
    m_author->setPlaceholderText("Jane Smith");
    m_credit->setPlaceholderText("Written by");
    m_contact->setPlaceholderText("jane@example.com");
    m_draftDate->setPlaceholderText("January 2026");
    m_wgaNumber->setPlaceholderText("WGA # 0000000");

    form->addRow("Title:", m_title);
    form->addRow("Author:", m_author);
    form->addRow("Credit:", m_credit);
    form->addRow("Contact:", m_contact);
    form->addRow("Draft Date:", m_draftDate);
    form->addRow("WGA #:", m_wgaNumber);

    root->addWidget(m_fieldsGroup);

    // Page numbering group
    auto *numGroup = new QGroupBox("PAGE NUMBERING", this);
    auto *numLayout = new QVBoxLayout(numGroup);
    numLayout->setSpacing(6);
    numLayout->setContentsMargins(12, 16, 12, 12);

    m_pageNumbering = new QCheckBox("Enable page numbering", this);
    m_pageNumbering->setChecked(settings.pageNumbering.enabled);
    numLayout->addWidget(m_pageNumbering);

    m_numberTitlePg = new QCheckBox("Number the title page", this);
    m_numberTitlePg->setChecked(settings.pageNumbering.numberTitlePage);
    numLayout->addWidget(m_numberTitlePg);

    root->addWidget(numGroup);

    // Enable/disable fields based on checkbox
    auto updateEnabled = [this] {
        const bool on = m_hasTitlePage->isChecked();
        m_fieldsGroup->setEnabled(on);
    };
    updateEnabled();
    connect(m_hasTitlePage, &QCheckBox::toggled, this, updateEnabled);

    // Buttons
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

DocumentSettings TitlePageDialog::result() const
{
    DocumentSettings s;
    s.hasTitlePage              = m_hasTitlePage->isChecked();
    s.titlePage.title           = m_title->text().trimmed();
    s.titlePage.author          = m_author->text().trimmed();
    s.titlePage.credit          = m_credit->text().trimmed();
    s.titlePage.contact         = m_contact->text().trimmed();
    s.titlePage.draftDate       = m_draftDate->text().trimmed();
    s.titlePage.wgaNumber       = m_wgaNumber->text().trimmed();
    s.pageNumbering.enabled     = m_pageNumbering->isChecked();
    s.pageNumbering.numberTitlePage = m_numberTitlePg->isChecked();
    s.pageNumbering.startNumber = 1;
    return s;
}
