# Tar Header 格式详解

本文档详细说明 POSIX USTAR 格式 tar 文件头的各个字段含义、格式和使用方法。

## 数据结构定义

```c
struct posix_header {   /* byte offset */
    char name[100];     /*   0-99: 文件名（以/结尾表示目录） */
    char mode[8];       /* 100-107: 八进制权限位，前导零填充 */
    char uid[8];        /* 108-115: 所有者的数字用户ID */
    char gid[8];        /* 116-123: 所有者的数字组ID */
    char size[12];      /* 124-135: 文件大小（八进制，对目录为0） */
    char mtime[12];     /* 136-147: 最后修改时间（Unix时间戳，八进制） */
    char chksum[8];     /* 148-155: 校验和（特殊计算方式） */
    char typeflag[1];   /* 156: 文件类型标志 */
    char linkname[100]; /* 157-256: 符号链接目标路径 */
    char magic[6];      /* 257-262: 魔术字符串 "ustar\0" */
    char version[2];    /* 263-264: 版本 "00" */
    char uname[32];     /* 265-296: 所有者用户名 */
    char gname[32];     /* 297-328: 所有者组名 */
    char devmajor[8];   /* 329-336: 主设备号（字符/块设备） */
    char devminor[8];   /* 337-344: 从设备号（字符/块设备） */
    char prefix[155];   /* 345-499: 文件名前缀（处理长路径） */
    char padding[12];   /* 500-511: 填充到512字节 */
};
```

---

## 1. `name[100]` - 文件名

### 格式说明

- **类型**: ASCII 字符串
- **长度**: 最多 100 字符（包含 null 终止符）
- **编码**: ASCII 或 Latin-1（不支持多字节编码，需通过 PAX 扩展处理 UTF-8）
- **终止符**: 应包含 null 终止符，剩余部分用 null 填充

### 特殊规则

1. **目录**: 以 `/` 字符结尾，例如 `mydir/`
2. **根目录**: 特殊名称 `.` 或 `./`
3. **当前目录相对路径**: `./filename`
4. **上级目录**: `../filename`
5. **GNU 长文件名**: 使用 `././@LongLink` 作为特殊标记（GNU 扩展）

### 使用示例

```c
// 普通文件
strncpy(header.name, "file.txt", sizeof(header.name));

// 目录
strncpy(header.name, "mydir/", sizeof(header.name));

// 当前目录下的文件
strncpy(header.name, "./config.ini", sizeof(header.name));

// GNU 长文件名标记
strncpy(header.name, "././@LongLink", sizeof(header.name));
```

### 注意事项

- 超过 100 字符的路径需要使用 `prefix` 字段或扩展机制
- 路径分隔符为 `/`（即使在 Windows 系统中）
- 应避免使用非 ASCII 字符（使用 PAX 扩展处理国际化）

---

## 2. `mode[8]` - 文件权限位

### 格式说明

- **类型**: 八进制数字字符串
- **长度**: 8 字符（包含 null 终止符）
- **格式**: `%07o\0`（7 位八进制数 + null 终止符）
- **前导零**: 必须用零填充到 7 位

### 权限位对应关系

```
位映射（八进制）：
0    : 特殊位（setuid/setgid/sticky）
1-3  : 用户权限
4-6  : 组权限
7-9  : 其他权限

示例：
0644 -> rw-r--r--
0755 -> rwxr-xr-x
04755 -> rwsr-xr-x（带 setuid）
```

### 使用示例

```c
// 普通文件 rw-r--r-- (0644)
format_octal(header.mode, 8, 0644);  // 结果为 "0000644\0"

// 可执行文件 rwxr-xr-x (0755)
format_octal(header.mode, 8, 0755);  // 结果为 "0000755\0"

// 目录 rwxr-xr-x (0755)
format_octal(header.mode, 8, 0755);

// 符号链接 lrwxrwxrwx (0777)
format_octal(header.mode, 8, 0777);
```

### 格式化函数

```c
void format_octal(char* dest, size_t width, uint64_t value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%llo", (unsigned long long)value);

    size_t len = strlen(buffer);
    if (len >= width) {
        strncpy(dest, buffer, width);
    } else {
        memset(dest, '0', width - len - 1);
        strncpy(dest + width - len - 1, buffer, len);
        dest[width - 1] = '\0';
    }
}
```

---

## 3. `uid[8]` / `gid[8]` - 用户/组 ID

### 格式说明

- **类型**: 八进制数字字符串
- **长度**: 8 字符（包含 null 终止符）
- **格式**: `%07o\0`（7 位八进制数 + null 终止符）
- **范围**: 0-2097151（八进制 7777777）

### 特殊值

- **root**: `0`（通常显示为 `"0000000\0"`）
- **nobody**: 65534 或 99（取决于系统）
- **未知用户**: 通常使用 `0` 或最大有效值

### 使用示例

```c
// root 用户
format_octal(header.uid, 8, 0);     // "0000000\0"

// 普通用户 ID 1000
format_octal(header.uid, 8, 1000);  // "0001750\0" (1000 的八进制)

// 使用有效最大值
format_octal(header.uid, 8, 2097151);  // "7777777\0"
```

### 数值转换

```c
uint64_t parse_octal(const char* str, size_t len) {
    uint64_t value = 0;
    for (size_t i = 0; i < len; i++) {
        if (str[i] == 0 || str[i] == ' ') break;
        if (str[i] < '0' || str[i] > '7') continue;
        value = value * 8 + (str[i] - '0');
    }
    return value;
}
```

### 扩展支持

由于 7 位八进制数限制最大值为 2097151（约 200 万），现代系统的大 ID 需要使用 PAX 扩展：

```
// 在 PAX 扩展头中
"21 uid=4294967294\n"  // 大 UID
"21 gid=4294967294\n"  // 大 GID
```

---

## 4. `size[12]` - 文件大小

### 格式说明

- **类型**: 八进制数字字符串
- **长度**: 12 字符（包含 null 终止符）
- **格式**: `%011o\0`（11 位八进制数 + null 终止符）
- **单位**: 字节数

### 特殊规则

1. **常规文件**: 文件的实际大小
2. **目录**: 必须为 `0`
3. **符号链接**: 必须为 `0`（链接目标长度存储在 `linkname` 中）
4. **设备文件**: 必须为 `0`
5. **FIFO/管道**: 必须为 `0`

### 大小限制

- **USTAR 限制**: 最大 `077777777777`（八进制） = 8,589,934,591 字节（8 GB）
- **超过限制**: 需要使用 PAX 扩展存储 64 位大小
- **GNU 格式**: 使用特殊扩展处理大于 8GB 的文件

### 使用示例

```c
// 1KB 文件
format_octal(header.size, 12, 1024);  // "00000002000\0"

// 空文件
format_octal(header.size, 12, 0);     // "00000000000\0"

// 目录（必须为 0）
format_octal(header.size, 12, 0);

// 超过 8GB 需要使用 PAX 扩展
if (file_size > 077777777777LL) {
    // 在 size 字段存储 0
    format_octal(header.size, 12, 0);
    // 在 PAX 头中存储实际大小
    // "22 size=1099511627776\n"
}
```

---

## 5. `mtime[12]` - 修改时间

### 格式说明

- **类型**: 八进制数字字符串
- **长度**: 12 字符（包含 null 终止符）
- **格式**: `%011o\0`（11 位八进制数 + null 终止符）
- **含义**: Unix 时间戳（自 1970-01-01 00:00:00 UTC 的秒数）

### 特殊值

- **0**: 1970-01-01 00:00:00 UTC
- **当前时间**: 使用 `time(NULL)`
- **无效时间**: 通常使用 0 或最大值

### 时间限制

- **USTAR 限制**: 最大 `077777777777` = 8589934591（约到 2242 年）
- **精度**: 只能存储秒级精度
- **纳秒精度**: 需要通过 PAX 扩展存储

### 使用示例

```c
#include <time.h>

time_t current_time = time(NULL);
format_octal(header.mtime, 12, (uint64_t)current_time);

// 解析示例
uint64_t timestamp = parse_octal(header.mtime, 12);
time_t mtime = (time_t)timestamp;
```

### PAX 扩展支持

对于更高精度的时间戳，使用 PAX 扩展：

```
// 纳秒级时间戳
"33 mtime=1617234567.123456789\n"

// 访问时间
"33 atime=1617234567.123456789\n"

// 创建时间
"33 ctime=1617234567.123456789\n"
```

---

## 6. `chksum[8]` - 校验和

### 格式说明

- **类型**: 八进制数字字符串
- **长度**: 8 字符
- **格式**: `%06o\0 `（6 位八进制数 + null 终止符 + 空格）
- **位置**: 7 字节，最后一位必须是空格

### 计算规则

校验和是头部 512 字节所有字节的**无符号和**，但在计算时，校验和字段本身被视为 **8 个空格**（ASCII 32）。

### 计算算法

```c
unsigned int calculate_checksum(const struct posix_header* hdr) {
    unsigned int sum = 0;
    const unsigned char* p = (const unsigned char*)hdr;

    for (int i = 0; i < 512; i++) {
        // 校验和字段（148-155字节）视为空格
        if (i >= 148 && i < 156) {
            sum += ' ';  // ASCII 32
        } else {
            sum += p[i];
        }
    }

    return sum;
}
```

### 使用示例

```c
// 1. 先填充其他字段
strncpy(header.name, "file.txt", 100);
// ... 设置其他字段

// 2. 将校验和字段填充为空格
memset(header.chksum, ' ', 8);

// 3. 计算校验和并格式化
unsigned int checksum = calculate_checksum(&header);
format_octal(header.chksum, 8, checksum);
// 注意：format_octal 会在第 7 字节放 null，需要修正
header.chksum[6] = '\0';
header.chksum[7] = ' ';  // 最后一位必须是空格
```

### 验证算法

```c
bool verify_checksum(const struct posix_header* hdr) {
    unsigned int stored = parse_octal(hdr->chksum, 8);
    unsigned int calculated = calculate_checksum(hdr);
    return stored == calculated;
}
```

---

## 7. `typeflag[1]` - 文件类型标志

### 类型定义

| 值          | ASCII             | 描述                 | 示例                     |
| ----------- | ----------------- | -------------------- | ------------------------ |
| `'0'`       | 普通文件（推荐）  | 常规文件数据         |
| `'\0'`      | NUL               | 普通文件（旧格式）   | 早期 tar 使用            |
| `'1'`       | 硬链接            | 指向归档中另一个文件 | 节省空间，但可移植性差   |
| `'2'`       | 符号链接          | 指向任意路径         | 软链接                   |
| `'3'`       | 字符设备          | 字符设备文件         | `/dev/null`, `/dev/tty`  |
| `'4'`       | 块设备            | 块设备文件           | `/dev/sda`, `/dev/loop0` |
| `'5'`       | 目录              | 目录条目             | 必须有 `/` 结尾          |
| `'6'`       | FIFO              | 命名管道             |
| `'7'`       | 保留              | GNU/扩展使用         |
| `'D'`       | 目录条目（GNU）   | GNU 特定目录         |
| `'K'`       | 长链接名（GNU）   | 长符号链接目标       |
| `'L'`       | 长路径名（GNU）   | 长文件名             |
| `'M'`       | 连续文件（GNU）   | 多卷归档继续         |
| `'N'`       | 长名称（旧 GNU）  | 旧长名               |
| `'S'`       | 稀疏文件（GNU）   | 稀疏文件             |
| `'V'`       | 卷头（GNU）       | 多卷归档头           |
| `'g'`       | 全局扩展头（PAX） | 全局 PAX 记录        |
| `'x'`       | 扩展头（PAX）     | 文件 PAX 记录        |
| `'A'`~`'Z'` | 保留              | 自定义扩展           |

### 使用示例

```c
// 普通文件
header.typeflag = '0';

// 目录
header.typeflag = '5';
strncat(header.name, "/", sizeof(header.name) - strlen(header.name) - 1);

// 符号链接
header.typeflag = '2';
strncpy(header.linkname, "/usr/bin/bash", sizeof(header.linkname));
header.size[0] = '0';  // 大小必须为0

// GNU 长文件名
header.typeflag = 'L';
strncpy(header.name, "././@LongLink", sizeof(header.name));
// 数据区包含完整路径名

// PAX 扩展头
header.typeflag = 'x';
strncpy(header.name, "PaxHeader", sizeof(header.name));
// 数据区包含 pax 记录
```

### 重要规则

1. **硬链接（`'1'`）**：

   - `linkname` 指向归档中另一个文件的完整路径
   - 不包含数据内容，`size` 为 0
   - 可移植性差，通常不推荐使用

2. **符号链接（`'2'`）**：

   - `linkname` 是链接目标的完整路径
   - `size` 必须为 0
   - 可以是绝对或相对路径

3. **目录（`'5'`）**：

   - `name` 必须以 `/` 结尾
   - `size` 必须为 0
   - 应包含 `.` 和 `..` 条目（可选）

4. **设备文件（`'3'`, `'4'`）**：
   - 使用 `devmajor` 和 `devminor` 字段
   - `size` 必须为 0
   - 通常需要 root 权限创建

---

## 8. `linkname[100]` - 链接目标

### 格式说明

- **类型**: ASCII 字符串
- **长度**: 最多 100 字符（包含 null 终止符）
- **使用条件**: 仅当 `typeflag` 为 `'1'`（硬链接）或 `'2'`（符号链接）时有效

### 硬链接 vs 符号链接

| 类型     | `typeflag` | 指向                       | 要求                 |
| -------- | ---------- | -------------------------- | -------------------- |
| 硬链接   | `'1'`      | 归档中另一个文件的完整路径 | 目标文件必须在归档中 |
| 符号链接 | `'2'`      | 任意路径（可能不存在）     | 可以是任意路径       |

### 使用示例

```c
// 符号链接到绝对路径
header.typeflag = '2';
strncpy(header.linkname, "/usr/bin/bash", sizeof(header.linkname));

// 符号链接到相对路径
strncpy(header.linkname, "../config/file.conf", sizeof(header.linkname));

// 硬链接
header.typeflag = '1';
strncpy(header.linkname, "archive/file.txt", sizeof(header.linkname));

// 清空不需要的字段
if (header.typeflag != '1' && header.typeflag != '2') {
    memset(header.linkname, 0, sizeof(header.linkname));
}
```

### 长链接名处理

超过 100 字符的链接目标需要 GNU 扩展：

```c
// 先创建 K 类型头存放长链接名
header.typeflag = 'K';  // GNU 长链接名
strncpy(header.name, "././@LongLink", sizeof(header.name));
// 数据区包含完整链接目标

// 再创建实际的符号链接头
header.typeflag = '2';
strncpy(header.name, "short_name", sizeof(header.name));
strncpy(header.linkname, "dummy", sizeof(header.linkname));
```

---

## 9. `magic[6]` - 魔术字符串

### 格式说明

- **类型**: 固定字符串
- **内容**: `"ustar\0"`（5 字符 + null 终止符）
- **用途**: 标识 USTAR 格式

### 不同格式的魔术值

| 格式         | magic 值    | 说明                |
| ------------ | ----------- | ------------------- |
| POSIX USTAR  | `"ustar\0"` | 标准格式，null 终止 |
| GNU tar      | `"ustar "`  | GNU 扩展，空格终止  |
| POSIX pax    | `"ustar\0"` | 可能包含 PAX 头     |
| 旧格式（v7） | 空或任何值  | 无魔术值            |

### 使用示例

```c
// 标准 USTAR 格式
strncpy(header.magic, "ustar", 6);
header.magic[5] = '\0';  // 确保 null 终止

// GNU 格式
strncpy(header.magic, "ustar ", 6);
header.magic[5] = ' ';   // 空格终止

// 验证魔术值
bool is_ustar = (memcmp(header.magic, "ustar", 5) == 0);
bool is_posix = (header.magic[5] == '\0');
bool is_gnu = (header.magic[5] == ' ');
```

### 检测函数

```c
enum TarFormat {
    FORMAT_UNKNOWN,
    FORMAT_V7,
    FORMAT_USTAR,
    FORMAT_GNU,
    FORMAT_PAX
};

enum TarFormat detect_format(const struct posix_header* hdr) {
    if (memcmp(hdr->magic, "ustar", 5) != 0) {
        return FORMAT_V7;
    }

    if (hdr->magic[5] == '\0') {
        return FORMAT_USTAR;
    } else if (hdr->magic[5] == ' ') {
        return FORMAT_GNU;
    }

    return FORMAT_UNKNOWN;
}
```

---

## 10. `version[2]` - 版本号

### 格式说明

- **类型**: 固定字符串
- **内容**: `"00"` 或 `"  "`（两个空格）
- **长度**: 2 字符
- **结合**: 与 `magic` 字段一起使用

### 版本标识

| 魔术值      | version | 格式             |
| ----------- | ------- | ---------------- |
| `"ustar\0"` | `"00"`  | POSIX USTAR 格式 |
| `"ustar "`  | `"  "`  | GNU tar 格式     |
| `"ustar\0"` | `"  "`  | 旧 GNU tar       |
| 其他        | 任意    | v7 格式或其他    |

### 使用示例

```c
// POSIX USTAR 格式
strncpy(header.magic, "ustar", 6);
header.magic[5] = '\0';
strncpy(header.version, "00", 2);

// GNU 格式
strncpy(header.magic, "ustar ", 6);
header.magic[5] = ' ';
strncpy(header.version, "  ", 2);
```

### 验证组合

```c
bool is_valid_ustar(const struct posix_header* hdr) {
    // 检查魔术值
    if (memcmp(hdr->magic, "ustar", 5) != 0) {
        return false;
    }

    // 检查版本
    if (hdr->magic[5] == '\0') {
        // POSIX USTAR: 版本必须是 "00"
        return (hdr->version[0] == '0' && hdr->version[1] == '0');
    } else if (hdr->magic[5] == ' ') {
        // GNU tar: 版本通常为 "  "（空格）
        return (hdr->version[0] == ' ' && hdr->version[1] == ' ');
    }

    return false;
}
```

---

## 11. `uname[32]` / `gname[32]` - 用户名/组名

### 格式说明

- **类型**: ASCII 字符串
- **长度**: 最多 32 字符（包含 null 终止符）
- **编码**: ASCII（不支持多字节）

### 使用规则

1. **用户/组名存储**：

   - 实际用户名/组名（如果长度 ≤ 31）
   - 超过 31 字符时截断或使用数字 ID
   - 可以通过 PAX 扩展存储完整名称

2. **与 UID/GID 的关系**：
   - `uid`/`gid` 是数字标识符
   - `uname`/`gname` 是符号名称
   - 两者都应提供，但解析器可能只使用其中一个

### 使用示例

```c
// 普通用户
strncpy(header.uname, "alice", sizeof(header.uname));
strncpy(header.gname, "users", sizeof(header.gname));

// root 用户
strncpy(header.uname, "root", sizeof(header.uname));
strncpy(header.gname, "root", sizeof(header.gname));

// 长用户名处理
const char* long_username = "very_long_username_exceeds_32_chars";
if (strlen(long_username) <= 31) {
    strncpy(header.uname, long_username, sizeof(header.uname));
} else {
    // 截断或使用 PAX 扩展
    header.uname[0] = '\0';
    // 在 PAX 头中存储完整名称
}
```

### PAX 扩展支持

```c
// 在 PAX 扩展头中存储长用户名
char pax_buffer[512];
snprintf(pax_buffer, sizeof(pax_buffer),
         "%d uname=%s\n%d gname=%s\n",
         (int)(6 + strlen(long_username)), long_username,
         (int)(6 + strlen(long_groupname)), long_groupname);
```

---

## 12. `devmajor[8]` / `devminor[8]` - 设备号

### 格式说明

- **类型**: 八进制数字字符串
- **长度**: 8 字符（包含 null 终止符）
- **格式**: `%07o\0`（7 位八进制数 + null 终止符）
- **使用条件**: 仅当 `typeflag` 为 `'3'`（字符设备）或 `'4'`（块设备）时有效

### 设备号含义

| 类型     | 主设备号     | 次设备号 | 示例             |
| -------- | ------------ | -------- | ---------------- |
| 字符设备 | 驱动程序标识 | 设备实例 | `/dev/tty1`: 4,1 |
| 块设备   | 驱动程序标识 | 分区编号 | `/dev/sda1`: 8,1 |
| 特殊设备 | 系统定义     | 系统定义 | `/dev/null`: 1,3 |

### Linux 常见设备号

| 设备文件       | 类型 | 主设备号 | 次设备号 |
| -------------- | ---- | -------- | -------- |
| `/dev/null`    | 字符 | 1        | 3        |
| `/dev/zero`    | 字符 | 1        | 5        |
| `/dev/full`    | 字符 | 1        | 7        |
| `/dev/random`  | 字符 | 1        | 8        |
| `/dev/urandom` | 字符 | 1        | 9        |
| `/dev/tty`     | 字符 | 5        | 0        |
| `/dev/console` | 字符 | 5        | 1        |
| `/dev/ptmx`    | 字符 | 5        | 2        |
| `/dev/sda`     | 块   | 8        | 0        |
| `/dev/sda1`    | 块   | 8        | 1        |
| `/dev/loop0`   | 块   | 7        | 0        |

### 使用示例

```c
#include <sys/stat.h>
#include <sys/sysmacros.h>

struct stat st;
if (stat("/dev/null", &st) == 0) {
    if (S_ISCHR(st.st_mode)) {
        header.typeflag = '3';  // 字符设备
        format_octal(header.devmajor, 8, major(st.st_rdev));
        format_octal(header.devminor, 8, minor(st.st_rdev));
    } else if (S_ISBLK(st.st_mode)) {
        header.typeflag = '4';  // 块设备
        format_octal(header.devmajor, 8, major(st.st_rdev));
        format_octal(header.devminor, 8, minor(st.st_rdev));
    }
}
```

### 创建时的注意事项

1. **需要特权**: 创建设备文件通常需要 root 权限
2. **可移植性**: 设备号在不同系统间可能不同
3. **安全性**: 避免在归档中包含敏感设备文件
4. **虚拟设备**: 如 `/dev/null`, `/dev/zero` 通常可以安全归档

---

## 13. `prefix[155]` - 路径名前缀

### 格式说明

- **类型**: ASCII 字符串
- **长度**: 最多 155 字符（包含 null 终止符）
- **用途**: 与 `name` 字段结合存储长路径

### 路径拼接规则

1. **如果 `prefix` 为空**：

   - 完整路径 = `name`

2. **如果 `prefix` 非空**：

   - 完整路径 = `prefix` + `/` + `name`
   - `prefix` 不应以 `/` 结尾
   - `name` 不应以 `/` 开头

3. **长度限制**：
   - `name` ≤ 100 字符
   - `prefix` ≤ 155 字符
   - 完整路径 ≤ 256 字符（`prefix` + `/` + `name`）

### 使用示例

```c
// 长路径: /usr/local/share/applications/myapp.desktop
const char* full_path = "/usr/local/share/applications/myapp.desktop";

// 分割为 prefix 和 name
const char* last_slash = strrchr(full_path, '/');
if (last_slash) {
    size_t prefix_len = last_slash - full_path;
    size_t name_len = strlen(last_slash + 1);

    if (prefix_len <= 155 && name_len <= 100) {
        strncpy(header.prefix, full_path, prefix_len);
        header.prefix[prefix_len] = '\0';
        strncpy(header.name, last_slash + 1, name_len);
        header.name[name_len] = '\0';
    } else {
        // 需要 GNU 或 PAX 扩展
        header.prefix[0] = '\0';
        strncpy(header.name, "short_name", sizeof(header.name));
        // 使用扩展头存储完整路径
    }
}
```

### 路径分割算法

```c
bool split_path(const char* path, char* prefix, size_t prefix_size,
                char* name, size_t name_size) {
    const char* last_slash = strrchr(path, '/');
    if (!last_slash) {
        // 没有目录部分
        if (strlen(path) < name_size) {
            strcpy(name, path);
            prefix[0] = '\0';
            return true;
        }
        return false;
    }

    size_t dir_len = last_slash - path;
    size_t file_len = strlen(last_slash + 1);

    if (dir_len < prefix_size && file_len < name_size) {
        strncpy(prefix, path, dir_len);
        prefix[dir_len] = '\0';
        strcpy(name, last_slash + 1);
        return true;
    }

    return false;
}
```

### 限制与扩展

- **USTAR 限制**: 最大路径长度 256 字符
- **GNU 扩展**: 使用 `typeflag='L'` 存储长路径
- **PAX 扩展**: 使用 `path=` 属性存储任意长度路径

---

## 14. `padding[12]` - 填充字段

### 格式说明

- **类型**: 任意字节（通常为 0）
- **长度**: 固定 12 字节
- **目的**: 确保头结构为 512 字节

### 填充规则

1. **USTAR 规范**: 未定义，通常填充为 0
2. **GNU tar**: 可能包含额外信息（如稀疏文件映射）
3. **实际使用**: 建议填充为 0，确保兼容性

### 使用示例

```c
// 标准填充方式
memset(header.padding, 0, sizeof(header.padding));

// 或者填充为空格
memset(header.padding, ' ', sizeof(header.padding));

// GNU tar 可能在特定情况下使用 padding 字段
if (header.typeflag == 'S' || header.typeflag == 'D') {
    // GNU 稀疏文件可能使用 padding
    // 但通常建议保持为 0
}
```

---

## 完整创建示例

```c
#include <string.h>
#include <time.h>
#include <sys/stat.h>

void create_tar_header(struct posix_header* hdr,
                      const char* filename,
                      const struct stat* st) {
    // 清空头部
    memset(hdr, 0, sizeof(struct posix_header));

    // 处理文件名
    split_path(filename, hdr->prefix, sizeof(hdr->prefix),
               hdr->name, sizeof(hdr->name));

    // 权限和所有者
    format_octal(hdr->mode, 8, st->st_mode & 07777);
    format_octal(hdr->uid, 8, st->st_uid);
    format_octal(hdr->gid, 8, st->st_gid);

    // 文件大小（目录和特殊文件为 0）
    if (S_ISREG(st->st_mode)) {
        format_octal(hdr->size, 12, st->st_size);
    } else {
        format_octal(hdr->size, 12, 0);
    }

    // 修改时间
    format_octal(hdr->mtime, 12, st->st_mtime);

    // 文件类型
    if (S_ISREG(st->st_mode)) {
        hdr->typeflag = '0';
    } else if (S_ISDIR(st->st_mode)) {
        hdr->typeflag = '5';
        // 确保目录名以 / 结尾
        size_t len = strlen(hdr->name);
        if (len < sizeof(hdr->name) - 1 && hdr->name[len-1] != '/') {
            hdr->name[len] = '/';
            hdr->name[len+1] = '\0';
        }
    } else if (S_ISLNK(st->st_mode)) {
        hdr->typeflag = '2';
        // 读取链接目标到 linkname
        readlink(filename, hdr->linkname, sizeof(hdr->linkname) - 1);
    } else if (S_ISCHR(st->st_mode)) {
        hdr->typeflag = '3';
        format_octal(hdr->devmajor, 8, major(st->st_rdev));
        format_octal(hdr->devminor, 8, minor(st->st_rdev));
    } else if (S_ISBLK(st->st_mode)) {
        hdr->typeflag = '4';
        format_octal(hdr->devmajor, 8, major(st->st_rdev));
        format_octal(hdr->devminor, 8, minor(st->st_rdev));
    } else if (S_ISFIFO(st->st_mode)) {
        hdr->typeflag = '6';
    }

    // 魔术字符串和版本
    strncpy(hdr->magic, "ustar", 6);
    strncpy(hdr->version, "00", 2);

    // 用户名和组名
    struct passwd* pw = getpwuid(st->st_uid);
    if (pw) {
        strncpy(hdr->uname, pw->pw_name, sizeof(hdr->uname) - 1);
    }
    struct group* gr = getgrgid(st->st_gid);
    if (gr) {
        strncpy(hdr->gname, gr->gr_name, sizeof(hdr->gname) - 1);
    }

    // 计算校验和
    memset(hdr->chksum, ' ', 8);
    unsigned int checksum = calculate_checksum(hdr);
    format_octal(hdr->chksum, 8, checksum);
    hdr->chksum[6] = '\0';
    hdr->chksum[7] = ' ';

    // 填充字段
    memset(hdr->padding, 0, sizeof(hdr->padding));
}
```

---

## 验证和调试函数

```c
void print_header_info(const struct posix_header* hdr) {
    printf("=== Tar Header Info ===\n");
    printf("Name:      %s\n", hdr->name);
    printf("Prefix:    %s\n", hdr->prefix);
    printf("Full Path: %s%s%s\n",
           hdr->prefix[0] ? hdr->prefix : "",
           hdr->prefix[0] ? "/" : "",
           hdr->name);
    printf("Type:      %c\n", hdr->typeflag[0]);
    printf("Mode:      %s (octal)\n", hdr->mode);
    printf("Size:      %llu bytes\n",
           (unsigned long long)parse_octal(hdr->size, 12));
    printf("UID/GID:   %s/%s\n", hdr->uid, hdr->gid);
    printf("User/Group:%s/%s\n", hdr->uname, hdr->gname);
    printf("Mod Time:  %s\n", hdr->mtime);
    printf("Checksum:  %s (stored) %u (calc)\n",
           hdr->chksum, calculate_checksum(hdr));
    printf("Magic:     %.*s (0x%02x)\n", 6, hdr->magic, hdr->magic[5]);
    printf("Version:   %c%c\n", hdr->version[0], hdr->version[1]);
    printf("Link Name: %s\n", hdr->linkname);
    if (hdr->typeflag[0] == '3' || hdr->typeflag[0] == '4') {
        printf("Device:    %s,%s\n", hdr->devmajor, hdr->devminor);
    }
    printf("========================\n");
}
```

这个文档详细介绍了每个字段的格式、使用方法和注意事项，应该可以帮助你实现一个完整的 tar 解析器/生成器。
