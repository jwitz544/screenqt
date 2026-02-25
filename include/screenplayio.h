#pragma once

class QString;
class ScriptEditor;

namespace ScreenplayIO {

bool saveDocument(ScriptEditor *editor, const QString &filePath);
bool loadDocument(ScriptEditor *editor, const QString &filePath, int &lineCount);

} // namespace ScreenplayIO
