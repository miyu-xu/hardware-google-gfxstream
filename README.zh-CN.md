# gfxstream（BSCP 分支）

[English](README.md) | 简体中文

gfxstream 提供 Android Guest 图形命令的主机端处理，并可结合 ANGLE 在 Linux、macOS 与
Windows 上输出。BSCP 主分支保持通用 gfxstream 基线；产品专用显示选择、遥测和录制接口
只属于 `hd-feature`。

修改渲染、外部内存、交换链或窗口生命周期时，必须分别验证无头路径和原生窗口路径，检查
资源释放、线程关闭与设备丢失处理，并运行相关 C++ 单元测试。构建由根仓库脚本与 manifest
固定的 aemu/ANGLE 版本共同驱动。
