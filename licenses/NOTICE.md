# 第三方组件与许可证

本发布包以动态链接方式使用 Qt 6（LGPLv3）。完整许可证原文见本目录下的
`LGPL-3.0-only.txt`，Qt 的版权声明归 The Qt Company Oy 及其贡献者所有。

- Qt 6 Core / Gui / Widgets / Network：https://www.qt.io/ ，LGPLv3。
- 打包时由 `windeployqt` 复制的插件（platforms、imageformats、tls、styles 等）
  同样来自 Qt，适用同一许可证。

如需替换 Qt 版本，删除包内 Qt6*.dll 与插件目录后放入自行编译的同版本 DLL 即可，
本程序未对 Qt 做静态链接或修改。