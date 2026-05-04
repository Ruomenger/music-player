# 数据库表设计 (SQLite3)

## ER 关系概览

```
playlists ──< playlist_songs >── songs
                 │
                 │ (optional join)
                 ▼
            play_history      settings
                              player_state
```

## 表定义

### 1. songs - 歌曲主表

存储本地音乐文件的元信息。

```sql
CREATE TABLE songs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    file_path TEXT NOT NULL UNIQUE,
    title TEXT NOT NULL,
    artist TEXT DEFAULT '',
    album TEXT DEFAULT '',
    duration REAL NOT NULL DEFAULT 0,
    file_size INTEGER DEFAULT 0,
    format TEXT DEFAULT '',
    bitrate INTEGER DEFAULT 0,
    sample_rate INTEGER DEFAULT 0,
    channels INTEGER DEFAULT 2,
    track_number INTEGER DEFAULT 0,
    cover_path TEXT DEFAULT '',
    md5_hash TEXT DEFAULT '',
    lyric_path TEXT DEFAULT '',
    lyric_source TEXT DEFAULT 'none',
    added_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE INDEX idx_songs_artist ON songs(artist);
CREATE INDEX idx_songs_album ON songs(album);
CREATE INDEX idx_songs_title ON songs(title);
CREATE INDEX idx_songs_file_path ON songs(file_path);
```

**字段说明**:
- `file_path`: 歌曲文件的绝对路径, UNIQUE 约束防重复添加
- `cover_path`: 封面图片缓存路径 (从音频元数据提取或同名图片复制到缓存目录)
- `md5_hash`: 文件哈希, 用于去重检测 (两个路径指向同一文件)
- `lyric_source`: `embedded` (内嵌歌词) / `lrc_file` (同名 lrc 文件) / `manual` (手动指定) / `none`
- `lyric_path`: 歌词文件路径 (当 `lyric_source` != `embedded` 时有效)

### 2. playlists - 歌单表

```sql
CREATE TABLE playlists (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT DEFAULT '',
    cover_path TEXT DEFAULT '',
    is_system INTEGER NOT NULL DEFAULT 0,
    sort_order INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);
```

**系统歌单** (`is_system = 1`): 如 "我喜欢"、"最近播放", 不可删除, 代码中创建。

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
    display_name TEXT DEFAULT '',
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
    play_mode TEXT NOT NULL DEFAULT 'sequential',
    volume REAL NOT NULL DEFAULT 0.8,
    position REAL NOT NULL DEFAULT 0,
    FOREIGN KEY (current_song_id) REFERENCES songs(id) ON DELETE SET NULL,
    FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE SET NULL
);
```

单行表 (`CHECK (id = 1)`), 退出时保存, 启动时恢复。

## 数据库初始化

```sql
PRAGMA journal_mode = WAL;        -- 写前日志, 支持并发读
PRAGMA foreign_keys = ON;         -- 启用外键约束
PRAGMA synchronous = NORMAL;      -- 平衡性能与安全性
PRAGMA cache_size = -8000;        -- 8MB 缓存
```

## 关键查询

```sql
-- 获取歌单中所有歌曲(按排序)
SELECT s.* FROM songs s
JOIN playlist_songs ps ON s.id = ps.song_id
WHERE ps.playlist_id = ?
ORDER BY ps.sort_index;

-- 搜索歌曲(标题/艺术家/专辑模糊匹配)
SELECT * FROM songs
WHERE title LIKE ? OR artist LIKE ? OR album LIKE ?
ORDER BY title LIMIT 100;

-- 最近播放历史
SELECT s.*, ph.played_at FROM songs s
JOIN play_history ph ON s.id = ph.song_id
ORDER BY ph.played_at DESC LIMIT 50;

-- 清理旧播放历史
DELETE FROM play_history
WHERE played_at < datetime('now', '-' || ? || ' days');
```
