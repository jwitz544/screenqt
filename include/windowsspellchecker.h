#pragma once

#include "spellcheckservice.h"

// Windows COM types — fully declared here via spellcheck.h when building on Windows.
// On other platforms this header is never included (CMakeLists guards it).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <spellcheck.h>

class WindowsSpellChecker final : public AbstractSpellChecker {
public:
    WindowsSpellChecker();
    ~WindowsSpellChecker() override;

    bool isAvailable() const override;
    QList<Misspelling> checkText(const QString &text) const override;
    QStringList suggestionsFor(const QString &word) const override;
    void addWord(const QString &word) override;

private:
    ISpellCheckerFactory *m_factory = nullptr;
    ISpellChecker        *m_checker = nullptr;
};
