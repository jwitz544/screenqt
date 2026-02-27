#pragma once

#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

struct Misspelling {
    int start = 0;
    int length = 0;
    QString word;
    QStringList suggestions;
};

class ISpellChecker {
public:
    virtual ~ISpellChecker() = default;
    virtual bool isAvailable() const = 0;
    virtual QList<Misspelling> checkText(const QString &text) const = 0;
    virtual QStringList suggestionsFor(const QString &word) const = 0;
    virtual void addWord(const QString &word) = 0;
};

class BasicSpellChecker final : public ISpellChecker {
public:
    BasicSpellChecker();

    bool isAvailable() const override { return true; }
    QList<Misspelling> checkText(const QString &text) const override;
    QStringList suggestionsFor(const QString &word) const override;
    void addWord(const QString &word) override;

private:
    bool isLikelyCorrectToken(const QString &word) const;
    int editDistance(const QString &a, const QString &b) const;

    QSet<QString> m_dictionary;
    QSet<QString> m_userDictionary;
};
