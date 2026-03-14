#pragma once

class QString;
class ScriptEditor;
struct DocumentSettings;

namespace ScreenplayIO {

bool saveDocument(ScriptEditor *editor, const QString &filePath, const DocumentSettings *settings = nullptr);
bool loadDocument(ScriptEditor *editor, const QString &filePath, int &lineCount, DocumentSettings *settings = nullptr);

} // namespace ScreenplayIO
