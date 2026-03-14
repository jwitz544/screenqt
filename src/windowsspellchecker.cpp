#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <spellcheck.h>
#include <ole2.h>

#include "windowsspellchecker.h"

#include <QString>
#include <QList>

WindowsSpellChecker::WindowsSpellChecker()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    HRESULT hr = CoCreateInstance(
        __uuidof(SpellCheckerFactory),
        nullptr,
        CLSCTX_INPROC_SERVER,
        __uuidof(ISpellCheckerFactory),
        reinterpret_cast<void **>(&m_factory)
    );

    if (FAILED(hr) || !m_factory) {
        return;
    }

    BOOL supported = FALSE;
    m_factory->IsSupported(L"en-US", &supported);
    if (!supported) {
        return;
    }

    m_factory->CreateSpellChecker(L"en-US", &m_checker);
}

WindowsSpellChecker::~WindowsSpellChecker()
{
    if (m_checker) {
        m_checker->Release();
        m_checker = nullptr;
    }
    if (m_factory) {
        m_factory->Release();
        m_factory = nullptr;
    }
    CoUninitialize();
}

bool WindowsSpellChecker::isAvailable() const
{
    return m_checker != nullptr;
}

QList<Misspelling> WindowsSpellChecker::checkText(const QString &text) const
{
    QList<Misspelling> result;
    if (!m_checker || text.isEmpty()) {
        return result;
    }

    const std::wstring wtext = text.toStdWString();
    IEnumSpellingError *errors = nullptr;

    if (FAILED(m_checker->Check(wtext.c_str(), &errors)) || !errors) {
        return result;
    }

    ISpellingError *error = nullptr;
    while (errors->Next(&error) == S_OK) {
        ULONG start = 0, length = 0;
        error->get_StartIndex(&start);
        error->get_Length(&length);

        Misspelling m;
        m.start  = static_cast<int>(start);
        m.length = static_cast<int>(length);
        m.word   = text.mid(m.start, m.length);
        result.append(m);

        error->Release();
    }
    errors->Release();

    return result;
}

QStringList WindowsSpellChecker::suggestionsFor(const QString &word) const
{
    QStringList suggestions;
    if (!m_checker || word.isEmpty()) {
        return suggestions;
    }

    const std::wstring wword = word.toStdWString();
    IEnumString *enumStr = nullptr;

    if (FAILED(m_checker->Suggest(wword.c_str(), &enumStr)) || !enumStr) {
        return suggestions;
    }

    LPOLESTR suggestion = nullptr;
    while (suggestions.size() < 8 && enumStr->Next(1, &suggestion, nullptr) == S_OK) {
        suggestions.append(QString::fromWCharArray(suggestion));
        CoTaskMemFree(suggestion);
    }
    enumStr->Release();

    return suggestions;
}

void WindowsSpellChecker::addWord(const QString &word)
{
    if (!m_checker || word.isEmpty()) {
        return;
    }
    m_checker->Add(word.toStdWString().c_str());
}
