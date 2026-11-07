# 可选素材格式

原素材、超分结果和预览只保存在用户本地，不提交仓库。默认没有 sidecar 时沿用原画。

RRHDv001 文件：8 字节魔数，随后六个 little-endian uint32：宽、高、参考图字节数、增强图字节数、0、0；接着是 640×480 RGB555 参考帧和指定尺寸 RGB555 增强帧。没有行填充，按从上到下排列。

`python tools/pack_sidecar.py --reference native.png --enhanced enhanced.png --output HD/batch01/MAIN.rrhd`

运行时路径为游戏目录中的 HD/batchXX/KEY.rrhd；完整键名和批次见 src/hd-backgrounds.h。拼接页须先按原布局组成完整 640×480 参考帧，不能把单块图冒充整页。RGB565 实际上传只在转换后的完整帧匹配后替换；未知状态、局部锁、只读锁或带覆盖内容的帧会保留原绘制。

Pillow 只供本地打包脚本使用；模型与外部超分工具由用户另行安装，保留各自许可。打包不能凭空新增纹理细节。
