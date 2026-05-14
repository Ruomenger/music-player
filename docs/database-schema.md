# 数据库表设计 (SQLite3)

> **状态：Phase 2 已落地（include `play_history` 在 Phase 4 加进来）。**
> 实际迁移脚本在 `src/infra/sqlite_database.cpp` 的 `kSchemaStatements[]`。
> 所有表、索引、触发器、PRAGMA 已经按本文设计落库；本文是设计意图说明，
> 真实 SQL 应以代码为准。FTS5（§全文搜索）仍待 Phase 9 做性能优化时启用。

## ER 关系概览

```
playlists ──< playlist_songs >── songs ──< play_history
                                  │
                                  └─< songs_fts (FTS5 virtual table)

settings (key-value)        player_state (single-row)
```

> "最近播放" **不是**一张实体歌单，而是 UI 层基于 `play_history` 的派生视图（避免歌单/历史双重簿记不一致）。

## 表定义

### 1. songs - 歌曲主表

存储本地音乐文件的元信息。

```sql
CREATE TABLE songs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    file_path TEXT NOT NULL UNIQUE,
    title TEXT NOT NULL,
    artist TEXT NOT NULL DEFAULT '',
    album TEXT NOT NULL DEFAULT '',
    album_artist TEXT NOT NULL DEFAULT '',
    composer TEXT NOT NULL DEFAULT '',
    genre TEXT NOT NULL DEFAULT '',
    year INTEGER NOT NULL DEFAULT 0,
    track_number INTEGER NOT NULL DEFAULT 0,
    disc_number INTEGER NOT NULL DEFAULT 0,
    duration REAL NOT NULL DEFAULT 0,
    file_size INTEGER NOT NULL DEFAULT 0,
    file_mtime INTEGER NOT NULL DEFAULT 0,    -- mtime epoch, 用于跳过未变文件的重扫描
    fingerprint TEXT NOT NULL DEFAULT '',     -- xxhash64(头16KB+尾16KB), 比 md5 全文快几十倍
    format TEXT NOT NULL DEFAULT '',
    bitrate INTEGER NOT NULL DEFAULT 0,
    sample_rate INTEGER NOT NULL DEFAULT 0,
    channels INTEGER NOT NULL DEFAULT 2,
    cover_path TEXT NOT NULL DEFAULT '',
    lyric_path TEXT NOT NULL DEFAULT '',
    lyric_source TEXT NOT NULL DEFAULT 'none'
        CHECK (lyric_source IN ('none', 'embedded', 'lrc_file', 'manual')),
    added_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE INDEX idx_songs_artist ON songs(artist);
CREATE INDEX idx_songs_album ON songs(album, disc_number, track_number);
CREATE INDEX idx_songs_year ON songs(year);
CREATE INDEX idx_songs_fingerprint ON songs(fingerprint) WHERE fingerprint != '';

-- 自动维护 updated_at (SQLite 不会自动更新)
CREATE TRIGGER trg_songs_updated_at
AFTER UPDATE ON songs
FOR EACH ROW WHEN NEW.updated_at = OLD.updated_at  -- 防止递归
BEGIN
    UPDATE songs SET updated_at = datetime('now') WHERE id = NEW.id;
END;
```

**字段说明**:
- `file_path`: 歌曲文件的绝对路径, UNIQUE 约束防同路径重复添加
- `file_mtime` + `file_size`: 用于扫描时快速判断文件是否变化（变了再重读元数据，否则跳过）
- `fingerprint`: 内容指纹（xxhash64 算头 16KB + 尾 16KB）。**比对内容用，不再用 md5 全文哈希**——10k 文件全 md5 要扫几分钟，而 32KB 抽样 hash 几乎瞬时，且对"同文件不同路径"这种罕见场景的去重已足够。**懒计算**：仅在用户主动触发"查找重复"或导入时才算，普通扫描不算。
- `cover_path`: 封面图片缓存路径 (从音频元数据提取或同名图片复制到 cache 目录)
- `lyric_source`: 加 CHECK 约束限制取值
- `lyric_path`: 歌词文件路径 (当 `lyric_source` != `embedded` 时有效)

### 2. playlists - 歌单表

```sql
CREATE TABLE playlists (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT NOT NULL DEFAULT '',
    cover_path TEXT NOT NULL DEFAULT '',
    is_system INTEGER NOT NULL DEFAULT 0,
    sort_order INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);
```

**系统歌单** (`is_system = 1`): 当前仅 "我喜欢" 一个，代码中创建，不可删除。

> **"最近播放" 不入此表**——它是 `play_history` 的派生视图（UI 层 `SELECT ... FROM play_history JOIN songs ...`）。如果建成实体 playlist，会和 history 双重簿记，状态容易脱节（比如清空 history 后这个 playlist 也得跟着清，多一个失败点）。

### 3. playlist_songs - 歌单歌曲关联表

```sql
CREATE TABLE playlist_songs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    playlist_id INTEGER NOT NULL,
    song_id INTEGER NOT NULL,
    sort_index INTEGER NOT NULL DEFAULT 0,
    added_at TEXT NOT NULL DEFAULT (datetime('now')),
    FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE,
    FOREIGN KEY (song_id) REFERENCES songs(id) ON DELETE CASCADE,
    UNIQUE(playlist_id, song_id)
);

CREATE INDEX idx_ps_playlist ON playlist_songs(playlist_id, sort_index);
```

- `sort_index`: 歌曲在歌单中的顺序, 支持拖拽排序
- `ON DELETE CASCADE`: 删除歌单或歌曲时自动清理关联

### 4. play_history - 播放历史表

```sql
CREATE TABLE play_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    song_id INTEGER NOT NULL,
    played_at TEXT NOT NULL DEFAULT (datetime('now')),
    progress REAL NOT NULL DEFAULT 0,
    FOREIGN KEY (song_id) REFERENCES songs(id) ON DELETE CASCADE
);

CREATE INDEX idx_history_song ON play_history(song_id);
CREATE INDEX idx_history_time ON play_history(played_at DESC);
```

- `progress`: 播放进度 (秒), 用于续播功能
- 需定期清理: 保留最近 N 条记录或 N 天内记录

### 5. settings - 用户配置表 (key-value)

```sql
CREATE TABLE settings (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL,
    value_type TEXT NOT NULL DEFAULT 'string',
    display_name TEXT NOT NULL DEFAULT '',
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);
```

**预置配置项**:

| key | value_type | 默认值 | 说明 |
|-----|-----------|--------|------|
| `volume` | double | 0.8 | 音量 (0.0-1.0) |
| `play_mode` | string | sequential | sequential/list_loop/single/random |
| `language` | string | zh_CN | 界面语言 |
| `theme` | string | system | 主题 (预留) |
| `output_device` | string | (auto) | 音频输出设备名 |
| `music_dir` | string | (系统音乐目录) | 默认音乐扫描路径 |
| `auto_load_lyric` | bool | true | 自动加载同名歌词 |
| `close_to_tray` | bool | false | 关闭时缩到系统托盘 |
| `history_max_days` | int | 90 | 播放历史保留天数 |

### 6. player_state - 播放器状态持久化

```sql
CREATE TABLE player_state (
    id INTEGER PRIMARY KEY CHECK (id = 1),
    current_song_id INTEGER,
    playlist_id INTEGER,
    position REAL NOT NULL DEFAULT 0,           -- 上次退出时的播放进度 (秒), 用于续播
    FOREIGN KEY (current_song_id) REFERENCES songs(id) ON DELETE SET NULL,
    FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE SET NULL
);
```

单行表 (`CHECK (id = 1)`), 退出时保存, 启动时恢复。

> **职责划分**：`volume` / `play_mode` / `language` 等用户偏好统一存 `settings` 表（跨会话持久），`player_state` 只存当前会话的"运行时"残留（哪首歌、哪个歌单、播到哪里）。早先版本两表都有 volume/play_mode，已合并到 settings 避免双源。

## 全文搜索 (FTS5)

`title` / `artist` / `album` 通过 SQL `LIKE '%xxx%'` 查询会做全表扫描，10k 行已经能感知到延迟。用 SQLite FTS5 虚表加速：

```sql
CREATE VIRTUAL TABLE songs_fts USING fts5(
    title, artist, album,
    content='songs', content_rowid='id',
    tokenize = 'unicode61 remove_diacritics 2'
);

-- 同步触发器
CREATE TRIGGER trg_songs_fts_insert AFTER INSERT ON songs BEGIN
    INSERT INTO songs_fts(rowid, title, artist, album)
    VALUES (NEW.id, NEW.title, NEW.artist, NEW.album);
END;
CREATE TRIGGER trg_songs_fts_delete AFTER DELETE ON songs BEGIN
    INSERT INTO songs_fts(songs_fts, rowid, title, artist, album)
    VALUES ('delete', OLD.id, OLD.title, OLD.artist, OLD.album);
END;
CREATE TRIGGER trg_songs_fts_update AFTER UPDATE ON songs BEGIN
    INSERT INTO songs_fts(songs_fts, rowid, title, artist, album)
    VALUES ('delete', OLD.id, OLD.title, OLD.artist, OLD.album);
    INSERT INTO songs_fts(rowid, title, artist, album)
    VALUES (NEW.id, NEW.title, NEW.artist, NEW.album);
END;
```

查询：
```sql
SELECT s.* FROM songs s
JOIN songs_fts f ON f.rowid = s.id
WHERE songs_fts MATCH ?      -- 例如: 'title:陈粒 OR artist:陈粒'
ORDER BY rank
LIMIT 100;
```

> 中文分词 FTS5 默认按字符切分，对中文搜索"陈粒"已够用；若需更智能的中文分词，可后续接入 ICU tokenizer。Phase 9 性能优化项。

## 数据库初始化

每个 `QSqlDatabase` 连接打开后立即设置（PRAGMA 是 per-connection 的，多连接需各自设置）：

```sql
PRAGMA journal_mode = WAL;        -- 写前日志, 支持并发读
PRAGMA foreign_keys = ON;         -- 启用外键约束 (必须 per-connection)
PRAGMA synchronous = NORMAL;      -- 平衡性能与安全性
PRAGMA cache_size = -8000;        -- 8MB 缓存
PRAGMA temp_store = MEMORY;       -- 临时表存内存
PRAGMA busy_timeout = 5000;       -- 5s 等锁
```

## 关键查询

```sql
-- 获取歌单中所有歌曲(按排序)
SELECT s.* FROM songs s
JOIN playlist_songs ps ON s.id = ps.song_id
WHERE ps.playlist_id = ?
ORDER BY ps.sort_index;

-- 搜索歌曲(优先用 FTS5; LIKE 仅作 fallback)
SELECT s.* FROM songs s
JOIN songs_fts f ON f.rowid = s.id
WHERE songs_fts MATCH ?
ORDER BY rank LIMIT 100;

-- 最近播放历史
SELECT s.*, ph.played_at FROM songs s
JOIN play_history ph ON s.id = ph.song_id
ORDER BY ph.played_at DESC LIMIT 50;

-- 清理旧播放历史
DELETE FROM play_history
WHERE played_at < datetime('now', '-' || ? || ' days');
```
