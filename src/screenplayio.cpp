#include "screenplayio.h"

#include "documentsettings.h"
#include "scripteditor.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace {

QString fdxParagraphTypeForElementState(int state)
{
    switch (state) {
    case ScriptEditor::SceneHeading:
        return "Scene Heading";
    case ScriptEditor::Action:
        return "Action";
    case ScriptEditor::CharacterName:
        return "Character";
    case ScriptEditor::Dialogue:
        return "Dialogue";
    case ScriptEditor::Parenthetical:
        return "Parenthetical";
    case ScriptEditor::Shot:
        return "Shot";
    case ScriptEditor::Transition:
        return "Transition";
    default:
        return "Action";
    }
}

int elementStateForFdxParagraphType(const QString &type)
{
    const QString normalized = type.trimmed().toLower();

    if (normalized == "scene heading") return ScriptEditor::SceneHeading;
    if (normalized == "action") return ScriptEditor::Action;
    if (normalized == "character") return ScriptEditor::CharacterName;
    if (normalized == "dialogue") return ScriptEditor::Dialogue;
    if (normalized == "parenthetical") return ScriptEditor::Parenthetical;
    if (normalized == "shot") return ScriptEditor::Shot;
    if (normalized == "transition") return ScriptEditor::Transition;

    return ScriptEditor::Action;
}

bool saveAsSqtFile(ScriptEditor *editor, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonArray lines;
    QTextDocument *doc = editor->document();
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
    return true;
}

QJsonObject titlePageToJson(const TitlePageData &titlePage)
{
    QJsonObject obj;
    obj["title"]     = titlePage.title;
    obj["author"]    = titlePage.author;
    obj["credit"]    = titlePage.credit;
    obj["contact"]   = titlePage.contact;
    obj["draftDate"] = titlePage.draftDate;
    obj["wgaNumber"] = titlePage.wgaNumber;
    return obj;
}

TitlePageData titlePageFromJson(const QJsonObject &obj)
{
    TitlePageData titlePage;
    titlePage.title     = obj["title"].toString();
    titlePage.author    = obj["author"].toString();
    titlePage.credit    = obj["credit"].toString();
    titlePage.contact   = obj["contact"].toString();
    titlePage.draftDate = obj["draftDate"].toString();
    titlePage.wgaNumber = obj["wgaNumber"].toString();
    return titlePage;
}

QJsonObject documentSettingsToJson(const DocumentSettings &settings)
{
    QJsonObject obj;
    obj["hasTitlePage"] = settings.hasTitlePage;
    obj["titlePage"] = titlePageToJson(settings.titlePage);

    QJsonObject pageNumbering;
    pageNumbering["enabled"] = settings.pageNumbering.enabled;
    pageNumbering["startNumber"] = settings.pageNumbering.startNumber;
    pageNumbering["numberTitlePage"] = settings.pageNumbering.numberTitlePage;
    obj["pageNumbering"] = pageNumbering;
    return obj;
}

DocumentSettings documentSettingsFromJson(const QJsonObject &obj)
{
    DocumentSettings settings;
    settings.hasTitlePage = obj["hasTitlePage"].toBool(false);
    settings.titlePage = titlePageFromJson(obj["titlePage"].toObject());

    const QJsonObject pageNumbering = obj["pageNumbering"].toObject();
    settings.pageNumbering.enabled = pageNumbering["enabled"].toBool(true);
    settings.pageNumbering.startNumber = qMax(1, pageNumbering["startNumber"].toInt(1));
    settings.pageNumbering.numberTitlePage = pageNumbering["numberTitlePage"].toBool(false);
    return settings;
}

bool saveAsSqtFile(ScriptEditor *editor, const QString &filePath, const DocumentSettings *settings)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonArray lines;
    QTextDocument *doc = editor->document();
    QTextBlock block = doc->begin();

    while (block.isValid()) {
        QJsonObject line;
        line["text"] = block.text();
        line["type"] = block.userState();
        lines.append(line);
        block = block.next();
    }

    QJsonObject root;
    root["version"] = 2;
    root["lines"] = lines;
    if (settings) {
        root["meta"] = documentSettingsToJson(*settings);
    }

    QJsonDocument jsonDoc(root);
    file.write(jsonDoc.toJson());
    file.close();
    return true;
}

bool saveAsFdxFile(ScriptEditor *editor, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument(QStringLiteral("1.0"), true);
    xml.writeDTD(QStringLiteral("<!DOCTYPE FinalDraft SYSTEM \"Final Draft Document Type Definition\">"));

    xml.writeStartElement(QStringLiteral("FinalDraft"));
    xml.writeAttribute(QStringLiteral("DocumentType"), QStringLiteral("Script"));
    xml.writeAttribute(QStringLiteral("Template"), QStringLiteral("No"));
    xml.writeAttribute(QStringLiteral("Version"), QStringLiteral("1"));

    xml.writeStartElement(QStringLiteral("Content"));

    QTextDocument *doc = editor->document();
    QTextBlock block = doc->begin();
    while (block.isValid()) {
        xml.writeStartElement(QStringLiteral("Paragraph"));
        xml.writeAttribute(QStringLiteral("Type"), fdxParagraphTypeForElementState(block.userState()));

        xml.writeStartElement(QStringLiteral("Text"));
        xml.writeCharacters(block.text());
        xml.writeEndElement();

        xml.writeEndElement();
        block = block.next();
    }

    xml.writeEndElement();
    xml.writeEndElement();
    xml.writeEndDocument();
    file.close();

    return !xml.hasError();
}

bool loadSqtFile(ScriptEditor *editor, const QString &filePath, int &lineCount, DocumentSettings *settings)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        return false;
    }

    QJsonObject root = jsonDoc.object();
    QJsonArray lines = root["lines"].toArray();

    if (settings) {
        if (root.contains("meta") && root["meta"].isObject()) {
            *settings = documentSettingsFromJson(root["meta"].toObject());
        } else {
            *settings = DocumentSettings();
        }
    }

    editor->clear();
    QTextCursor cursor(editor->document());
    cursor.beginEditBlock();

    lineCount = 0;
    for (const QJsonValue &val : lines) {
        QJsonObject line = val.toObject();
        QString text = line["text"].toString();
        int type = line["type"].toInt();

        cursor.insertText(text);
        QTextBlock block = cursor.block();
        block.setUserState(type);
        cursor.insertText("\n");
        cursor.movePosition(QTextCursor::NextBlock);
        ++lineCount;
    }

    cursor.endEditBlock();
    return true;
}

bool loadFdxFile(ScriptEditor *editor, const QString &filePath, int &lineCount)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QXmlStreamReader xml(&file);
    QVector<QPair<QString, int>> paragraphs;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QLatin1String("Paragraph")) {
            const QString type = xml.attributes().value(QStringLiteral("Type")).toString();
            const int elementState = elementStateForFdxParagraphType(type);

            QString paragraphText;
            while (!(xml.isEndElement() && xml.name() == QLatin1String("Paragraph")) && !xml.atEnd()) {
                xml.readNext();
                if (xml.isStartElement() && xml.name() == QLatin1String("Text")) {
                    paragraphText += xml.readElementText(QXmlStreamReader::IncludeChildElements);
                }
            }

            paragraphs.append(qMakePair(paragraphText, elementState));
        }
    }

    file.close();
    if (xml.hasError()) {
        return false;
    }

    editor->clear();
    QTextCursor cursor(editor->document());
    cursor.beginEditBlock();

    lineCount = 0;
    for (const auto &paragraph : paragraphs) {
        cursor.insertText(paragraph.first);
        QTextBlock block = cursor.block();
        block.setUserState(paragraph.second);
        cursor.insertText("\n");
        cursor.movePosition(QTextCursor::NextBlock);
        ++lineCount;
    }

    cursor.endEditBlock();
    return true;
}

} // namespace

namespace ScreenplayIO {

bool saveDocument(ScriptEditor *editor, const QString &filePath, const DocumentSettings *settings)
{
    const QString extension = QFileInfo(filePath).suffix().toLower();
    if (extension == QStringLiteral("fdx")) {
        return saveAsFdxFile(editor, filePath);
    }
    return saveAsSqtFile(editor, filePath, settings);
}

bool loadDocument(ScriptEditor *editor, const QString &filePath, int &lineCount, DocumentSettings *settings)
{
    const QString extension = QFileInfo(filePath).suffix().toLower();
    if (extension == QStringLiteral("fdx")) {
        if (settings) {
            *settings = DocumentSettings();
        }
        return loadFdxFile(editor, filePath, lineCount);
    }
    return loadSqtFile(editor, filePath, lineCount, settings);
}

} // namespace ScreenplayIO
