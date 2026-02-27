#include "spellcheckservice.h"

#include <QRegularExpression>
#include <QVector>

namespace {
QString normalizeWord(const QString &word)
{
    return word.trimmed().toLower();
}

const QSet<QString> &baseDictionary()
{
    static const QSet<QString> words = {
        "a", "about", "above", "after", "again", "all", "also", "am", "an", "and", "any", "are", "as", "at",
        "back", "be", "because", "been", "before", "being", "between", "but", "by", "can", "come", "could",
        "day", "did", "do", "does", "down", "each", "even", "every", "for", "from", "get", "go", "good",
        "had", "has", "have", "he", "her", "here", "him", "his", "how", "i", "if", "in", "into", "is", "it",
        "its", "just", "know", "like", "look", "make", "man", "me", "more", "my", "new", "no", "not", "now",
        "of", "on", "one", "only", "or", "other", "our", "out", "over", "people", "right", "said", "same",
        "say", "scene", "screenplay", "script", "see", "she", "so", "some", "story", "take", "than", "that",
        "the", "their", "them", "then", "there", "these", "they", "this", "time", "to", "two", "up", "use",
        "very", "want", "was", "way", "we", "well", "were", "what", "when", "where", "which", "who", "will",
        "with", "work", "would", "write", "writer", "you", "your",
        "int", "ext", "est", "fade", "cut", "dissolve", "to", "continuously", "later", "night", "day", "interior", "exterior"
    };
    return words;
}
}

BasicSpellChecker::BasicSpellChecker()
    : m_dictionary(baseDictionary())
{
}

QList<Misspelling> BasicSpellChecker::checkText(const QString &text) const
{
    QList<Misspelling> result;
    static const QRegularExpression tokenRegex("[A-Za-z][A-Za-z']*");

    QRegularExpressionMatchIterator it = tokenRegex.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString word = match.captured(0);
        if (isLikelyCorrectToken(word)) {
            continue;
        }

        Misspelling item;
        item.start = match.capturedStart(0);
        item.length = match.capturedLength(0);
        item.word = word;
        item.suggestions = suggestionsFor(word);
        result.append(item);
    }

    return result;
}

QStringList BasicSpellChecker::suggestionsFor(const QString &word) const
{
    const QString normalized = normalizeWord(word);
    if (normalized.isEmpty()) {
        return {};
    }

    struct Candidate {
        QString word;
        int distance = 0;
    };

    QVector<Candidate> candidates;
    candidates.reserve(16);

    for (const QString &entry : m_dictionary) {
        if (qAbs(entry.size() - normalized.size()) > 2) {
            continue;
        }
        if (entry.front() != normalized.front()) {
            continue;
        }

        const int distance = editDistance(entry, normalized);
        if (distance <= 2) {
            candidates.push_back({entry, distance});
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate &a, const Candidate &b) {
        if (a.distance == b.distance) {
            return a.word < b.word;
        }
        return a.distance < b.distance;
    });

    QStringList result;
    for (const Candidate &candidate : candidates) {
        result.append(candidate.word);
        if (result.size() >= 6) {
            break;
        }
    }

    return result;
}

void BasicSpellChecker::addWord(const QString &word)
{
    const QString normalized = normalizeWord(word);
    if (!normalized.isEmpty()) {
        m_userDictionary.insert(normalized);
    }
}

bool BasicSpellChecker::isLikelyCorrectToken(const QString &word) const
{
    if (word.size() <= 2) {
        return true;
    }

    if (word == word.toUpper()) {
        return true;
    }

    const QString normalized = normalizeWord(word);
    if (normalized.isEmpty()) {
        return true;
    }

    return m_dictionary.contains(normalized) || m_userDictionary.contains(normalized);
}

int BasicSpellChecker::editDistance(const QString &a, const QString &b) const
{
    const int n = a.size();
    const int m = b.size();

    QVector<int> prev(m + 1);
    QVector<int> curr(m + 1);

    for (int j = 0; j <= m; ++j) {
        prev[j] = j;
    }

    for (int i = 1; i <= n; ++i) {
        curr[0] = i;
        for (int j = 1; j <= m; ++j) {
            const int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] = std::min({
                prev[j] + 1,
                curr[j - 1] + 1,
                prev[j - 1] + cost
            });
        }
        prev = curr;
    }

    return prev[m];
}
