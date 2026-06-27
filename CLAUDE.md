# LimdC 项目上下文

## 项目概览

LimdC 是一个最小 `.lc` 编译器原型。当前目标是读取 `.lc` 文件，识别 C/C++ 导入声明，将内置 UI DSL 转换为 Win32 C++，并调用 `gcc` / `g++` 生成 Windows exe。

## 当前结构

- `src/main.cpp`：编译器入口、最小解析器、代码生成器和目标编译调用逻辑。
- `Example/newUI/main.lc`：当前主要验收示例，包含窗口和按钮语法。
- `CMakeLists.txt`：构建 `limdc` 编译器。
- `README.md`：使用说明。

## 构建命令

```powershell
cmake -S . -B build
cmake --build build
```

## 示例编译命令

```powershell
.\build\limdc.exe .\Example\newUI\main.lc -o .\Example\newUI\main.exe
```

## 设计约束

- 第一版面向 Windows，UI 生成 Win32 C++。
- `#include [C]` / `#include [C++]` 是语法启用声明。
- 带 `<...>` 的 include 会转成目标 C/C++ include。
- UI 语句由编译器转换，不交给 gcc/g++ 直接解析。
- 非 UI 的 C/C++ 语句仅在启用 `[C]` 或 `[C++]` 后按原生语句转发。
- 当前只支持一个主窗口和多个按钮。
