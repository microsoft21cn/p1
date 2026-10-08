# Windows 备忘录

双击 `dist/Memo.exe` 使用，面向 Windows 10/11 x64，无需安装 Node.js、Python 或额外运行库。

- 新增、编辑、删除备忘，搜索标题和正文。
- 编辑停止约 600 毫秒后自动保存，关闭窗口时再次保存。
- 数据位于 `%LOCALAPPDATA%\SimpleMemo\notes.dat`，与 exe 位置无关。
- 删除需要确认；删除后无法撤销。备份数据请先关闭程序，再复制上述文件。
- 保存失败会显示状态提示并阻止关闭；读取失败不会覆盖原数据。
- 数据为本地明文二进制文件，不适合存储密码或机密。

源码：`memo/main.cpp`。使用 MinGW-w64 编译：

```sh
x86_64-w64-mingw32-g++ -std=c++17 -O2 -Wall -Wextra -static -municode -mwindows memo/main.cpp -o dist/Memo.exe -lshell32 -luser32 -lgdi32
```

Windows 验收：新增含中文和换行的备忘 → 编辑 → 搜索正文 → 清空搜索 → 重启确认保存 → 删除并重启确认删除。另检查空标题、空列表及同时启动两次。

构建环境为 Linux；Windows 图形界面交互需在 Windows 实机验证。
