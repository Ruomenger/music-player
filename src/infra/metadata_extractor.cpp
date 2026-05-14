#include "metadata_extractor.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

#include <charconv>
#include <cstring>
#include <memory>
#include <string_view>
#include <system_error>

namespace musicplayer {

namespace {

struct FormatCtxDeleter {
    void operator()(AVFormatContext* p) const {
        if (p)
            avformat_close_input(&p);
    }
};
using FormatCtxPtr = std::unique_ptr<AVFormatContext, FormatCtxDeleter>;

// av_dict_get is case-insensitive when AV_DICT_MATCH_CASE is *not* set, which
// is what we want — different containers use different cases (TITLE vs title).
std::string readTag(AVDictionary* dict, const char* key) {
    if (!dict)
        return {};
    if (const AVDictionaryEntry* entry = av_dict_get(dict, key, nullptr, 0))
        return entry->value ? entry->value : std::string{};
    return {};
}

// Tag may be either stream-level (FLAC, OGG) or format-level (MP3 ID3). Prefer
// stream-level (more specific) when both exist.
std::string readTagBoth(AVDictionary* streamDict, AVDictionary* formatDict, const char* key) {
    auto v = readTag(streamDict, key);
    if (!v.empty())
        return v;
    return readTag(formatDict, key);
}

// "12/24" → 12 (skip total); plain "12" also handled.
int parseLeadingInt(std::string_view s) {
    int value = 0;
    const auto* begin = s.data();
    const auto* end = s.data() + s.size();
    // Skip leading whitespace; from_chars stops at the first non-digit.
    while (begin != end && (*begin == ' ' || *begin == '\t'))
        ++begin;
    auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{})
        return 0;
    return value;
}

// FFmpeg's attached_pic AVPacket carries the raw image bytes (already JPEG /
// PNG-encoded — not the decoded pixels). Sniff the magic to pick a mime type.
std::string mimeFromMagic(const std::byte* data, std::size_t size) {
    if (size >= 3 && static_cast<unsigned char>(data[0]) == 0xFF &&
        static_cast<unsigned char>(data[1]) == 0xD8 && static_cast<unsigned char>(data[2]) == 0xFF)
        return "image/jpeg";
    if (size >= 8 && static_cast<unsigned char>(data[0]) == 0x89 &&
        static_cast<unsigned char>(data[1]) == 0x50 &&
        static_cast<unsigned char>(data[2]) == 0x4E && static_cast<unsigned char>(data[3]) == 0x47)
        return "image/png";
    return {};
}

}  // namespace

std::optional<ExtractedMetadata> MetadataExtractor::extract(const std::string& filePath) {
    AVFormatContext* raw = nullptr;
    if (avformat_open_input(&raw, filePath.c_str(), nullptr, nullptr) < 0)
        return std::nullopt;
    FormatCtxPtr ctx(raw);

    if (avformat_find_stream_info(ctx.get(), nullptr) < 0)
        return std::nullopt;

    int audioIdx = -1;
    int coverIdx = -1;
    for (unsigned i = 0; i < ctx->nb_streams; ++i) {
        AVStream* stream = ctx->streams[i];
        const auto type = stream->codecpar->codec_type;
        if (audioIdx < 0 && type == AVMEDIA_TYPE_AUDIO)
            audioIdx = static_cast<int>(i);
        if (coverIdx < 0 && type == AVMEDIA_TYPE_VIDEO &&
            (stream->disposition & AV_DISPOSITION_ATTACHED_PIC))
            coverIdx = static_cast<int>(i);
    }
    if (audioIdx < 0)
        return std::nullopt;

    ExtractedMetadata md;
    AVStream* audio = ctx->streams[audioIdx];
    AVDictionary* sd = audio->metadata;
    AVDictionary* fd = ctx->metadata;

    md.title = readTagBoth(sd, fd, "title");
    md.artist = readTagBoth(sd, fd, "artist");
    md.album = readTagBoth(sd, fd, "album");
    md.albumArtist = readTagBoth(sd, fd, "album_artist");
    md.composer = readTagBoth(sd, fd, "composer");
    md.genre = readTagBoth(sd, fd, "genre");

    if (const auto date = readTagBoth(sd, fd, "date"); !date.empty())
        md.year = parseLeadingInt(date);
    if (md.year == 0) {
        if (const auto year = readTagBoth(sd, fd, "year"); !year.empty())
            md.year = parseLeadingInt(year);
    }
    md.trackNumber = parseLeadingInt(readTagBoth(sd, fd, "track"));
    md.discNumber = parseLeadingInt(readTagBoth(sd, fd, "disc"));

    if (audio->duration > 0)
        md.duration = static_cast<double>(audio->duration) * av_q2d(audio->time_base);
    else if (ctx->duration > 0)
        md.duration = static_cast<double>(ctx->duration) / AV_TIME_BASE;
    md.format = ctx->iformat && ctx->iformat->name ? ctx->iformat->name : "";
    md.bitrate = static_cast<int>(audio->codecpar->bit_rate / 1000);
    if (md.bitrate == 0 && ctx->bit_rate > 0)
        md.bitrate = static_cast<int>(ctx->bit_rate / 1000);
    md.sampleRate = audio->codecpar->sample_rate;
    md.channels = audio->codecpar->ch_layout.nb_channels;

    if (coverIdx >= 0) {
        const AVPacket& pic = ctx->streams[coverIdx]->attached_pic;
        if (pic.size > 0 && pic.data != nullptr) {
            md.coverData.resize(static_cast<std::size_t>(pic.size));
            std::memcpy(md.coverData.data(), pic.data, md.coverData.size());
            md.coverMime = mimeFromMagic(md.coverData.data(), md.coverData.size());
        }
    }

    return md;
}

}  // namespace musicplayer
