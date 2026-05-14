#include "lyric_parser.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace musicplayer {

namespace {

std::string_view trim(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front())))
        sv.remove_prefix(1);
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back())))
        sv.remove_suffix(1);
    return sv;
}

bool parseInt(std::string_view s, int& out) {
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
    return ec == std::errc{};
}

// Reads "mm:ss[.fff]" from the body of a single bracket. Returns total ms,
// or -1 if the body isn't a numeric timestamp (e.g. it's metadata like
// "ti:Title" or "offset:-100").
std::int64_t parseTimestamp(std::string_view body) {
    const auto colon = body.find(':');
    if (colon == std::string_view::npos)
        return -1;
    const auto dot = body.find('.', colon + 1);
    int minutes = 0;
    int seconds = 0;
    int fraction = 0;
    int fractionDigits = 0;
    if (!parseInt(body.substr(0, colon), minutes))
        return -1;
    const auto secEnd = dot == std::string_view::npos ? body.size() : dot;
    if (!parseInt(body.substr(colon + 1, secEnd - colon - 1), seconds))
        return -1;
    if (dot != std::string_view::npos) {
        const auto fracText = body.substr(dot + 1);
        if (!parseInt(fracText, fraction))
            return -1;
        fractionDigits = static_cast<int>(fracText.size());
    }
    if (minutes < 0 || seconds < 0 || fraction < 0)
        return -1;
    int fractionMs = 0;
    if (fractionDigits == 1) {
        fractionMs = fraction * 100;
    } else if (fractionDigits == 2) {
        fractionMs = fraction * 10;
    } else if (fractionDigits >= 3) {
        // Truncate beyond 3 digits — LRC has no convention past ms.
        while (fractionDigits > 3) {
            fraction /= 10;
            --fractionDigits;
        }
        fractionMs = fraction;
    }
    return (static_cast<std::int64_t>(minutes) * 60 * 1000) +
           (static_cast<std::int64_t>(seconds) * 1000) + fractionMs;
}

void emplaceMetadata(std::unordered_map<std::string, std::string>& meta, std::string_view body) {
    const auto colon = body.find(':');
    if (colon == std::string_view::npos || colon == 0)
        return;
    std::string key(trim(body.substr(0, colon)));
    std::string value(trim(body.substr(colon + 1)));
    for (char& c : key)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    meta.emplace(std::move(key), std::move(value));
}

}  // namespace

ParsedLyrics LyricParser::parse(std::string_view content) {
    ParsedLyrics out;

    auto handleLine = [&](std::string_view line) {
        line = trim(line);
        if (line.empty() || line.front() != '[')
            return;

        // Collect all leading "[...]"; capture timestamps, route the rest to
        // metadata. Stops at the first non-bracket char, which marks the
        // start of the lyric text.
        std::vector<std::int64_t> timestamps;
        std::size_t pos = 0;
        while (pos < line.size() && line[pos] == '[') {
            const auto close = line.find(']', pos);
            if (close == std::string_view::npos)
                return;  // malformed bracket; skip the line
            const std::string_view body = line.substr(pos + 1, close - pos - 1);
            const std::int64_t ms = parseTimestamp(body);
            if (ms >= 0) {
                timestamps.push_back(ms);
            } else {
                emplaceMetadata(out.metadata, body);
            }
            pos = close + 1;
        }
        if (timestamps.empty())
            return;

        const std::string text(trim(line.substr(pos)));
        for (std::int64_t ts : timestamps) {
            LyricLine ll;
            ll.timestampMs = ts;
            ll.text = text;
            out.lines.push_back(std::move(ll));
        }
    };

    // Walk by line, supporting CRLF and bare LF.
    std::size_t start = 0;
    for (std::size_t i = 0; i <= content.size(); ++i) {
        const bool eol = i == content.size() || content[i] == '\n';
        if (!eol)
            continue;
        std::string_view line = content.substr(start, i - start);
        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1);
        handleLine(line);
        start = i + 1;
    }

    // [offset:N] is "shift every timestamp by N ms" (negative ⇒ earlier).
    if (auto it = out.metadata.find("offset"); it != out.metadata.end()) {
        int n = 0;
        if (parseInt(it->second, n)) {
            for (auto& l : out.lines)
                l.timestampMs = std::max<std::int64_t>(0, l.timestampMs + n);
        }
    }

    std::ranges::sort(out.lines, [](const LyricLine& a, const LyricLine& b) {
        return a.timestampMs < b.timestampMs;
    });
    return out;
}

int LyricParser::findCurrentLineIndex(const std::vector<LyricLine>& lines, std::int64_t ms) {
    if (lines.empty())
        return -1;
    // upper_bound returns the first line strictly after ms; the one before
    // it is the line currently "playing".
    auto it =
        std::ranges::upper_bound(lines, ms, {}, [](const LyricLine& l) { return l.timestampMs; });
    if (it == lines.begin())
        return -1;
    return static_cast<int>(std::distance(lines.begin(), it) - 1);
}

}  // namespace musicplayer
