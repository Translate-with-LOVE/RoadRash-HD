# Road Rash HD

为 Road Rash: 30th Anniversary Edition（Steam AppID 2448570，build 26105967）提供高分辨率比赛、原生分辨率设置、菜单画布与本地高清素材替换。高清项目与 [汉化项目](https://github.com/Translate-with-LOVE/RoadRash-zh-CN) 独立。

支持 640×480、960×720、1440×1080、1920×1440、2880×2160。首次缺少设置时按屏幕选档，已有选择保持不变，改档后重启生效。包含制作人员裁剪修复和 RGB565 高清替换适配。

原生 `.RRS` 保存可使用游戏所选的文档目录等路径，保留 Windows 权限和文件占用检查；不重定向已有存档，也不改变 Steam Cloud 的同步规则。

## 构建与使用

需要 Windows、Visual Studio 的 x86 C/C++ 工具和 Windows SDK、Python 3。请在 x86 Native Tools 开发者终端运行 `powershell -NoProfile -File build.ps1`，输出在 build。无需运行游戏即可执行离线测试。

使用 `python tools/bootstrap.py --input <自有原版或汉化版EXE> --output <副本中的RoadRash95.exe>` 加入核心导入，再把 build/rr4k-isolation.dll 放到同一副本目录，建立空文件 rr4k-trial.enabled。只处理指定版本，输出不得覆盖输入。可从 Steam 直接启动已应用的副本配置，无需专用启动器。

高清侧载图是用户本地数据。`tools/pack_sidecar.py` 将用户自有的 640×480 参考图和增强图打包为 RRHD；格式和路径见 docs/ASSETS.md。本仓库不含原始素材或高清派生图，单独构建核心不会凭空获得新增纹理细节。

先备份整个游戏目录及存档。若同时使用汉化，请先构建汉化 EXE，再对它加入高清导入；不要用未整合的 EXE 覆盖另一项目的改动。

## 验证范围

自动选档、设置持久化及 RGB555/RGB565 上传/解锁路径有不含游戏的离屏测试。内存表面回放不能代替 DirectDraw 驱动、前台菜单、完整比赛和存档的实机验收。不会自动启动游戏、锁鼠或播放音频。

## 许可

自编代码采用 [MIT](LICENSE)，第三方声明见 [NOTICE](NOTICE.md)。

## 制作说明

程序、翻译：GPT-6 Astra，测试：手柄喵（Handle）
附带字体 未来荧黑 Condensed，使用 OFL 许可进行再分发

转载请保留本段
