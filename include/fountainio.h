#pragma once
#include <QString>

class ScriptEditor;

namespace FountainIO {

bool saveFountain(ScriptEditor *editor, const QString &filePath);
bool loadFountain(ScriptEditor *editor, const QString &filePath);

}
