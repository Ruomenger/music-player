#include "cover_cache.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QString>
#include <QtLogging>

#include <filesystem>
#include <system_error>
#include <utility>

namespace musicplayer {

namespace {

// Map mime → file extension. Anything else is rejected at the call site.
std::string extensionFor(std::string_view mime) {
    if (mime == "image/jpeg")
        return ".jpg";
    if (mime == "image/png")
        return ".png";
    return {};
}

}  // namespace

CoverCache::CoverCache(std::string cacheDir) : cacheDir_(std::move(cacheDir)) {}

std::string CoverCache::store(const std::byte* data, std::size_t size,
                              std::string_view mime) const {
    if (size == 0 || data == nullptr) {
        return {};
    }
    const std::string ext = extensionFor(mime);
    if (ext.empty()) {
        qWarning("CoverCache::store: unsupported mime '%s'", std::string(mime).c_str());
        return {};
    }
    if (cacheDir_.empty()) {
        qWarning("CoverCache::store: cache dir not configured");
        return {};
    }

    std::error_code ec;
    std::filesystem::create_directories(cacheDir_, ec);
    if (ec) {
        qWarning("CoverCache::store: mkdir '%s' failed: %s", cacheDir_.c_str(),
                 ec.message().c_str());
        return {};
    }

    const QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    const QString hash = QCryptographicHash::hash(bytes, QCryptographicHash::Md5).toHex();
    const std::filesystem::path target =
        std::filesystem::path(cacheDir_) / (hash.toStdString() + ext);

    // Idempotent: a previous song with the same cover bytes already wrote this
    // file. Skip the I/O and return the existing path.
    if (std::filesystem::exists(target, ec) && !ec) {
        return target.string();
    }

    QFile out(QString::fromStdString(target.string()));
    if (!out.open(QIODevice::WriteOnly)) {
        qWarning("CoverCache::store: open '%s' failed: %s", target.string().c_str(),
                 out.errorString().toUtf8().constData());
        return {};
    }
    if (out.write(bytes) != bytes.size()) {
        qWarning("CoverCache::store: short write to '%s'", target.string().c_str());
        out.remove();
        return {};
    }
    return target.string();
}

}  // namespace musicplayer
