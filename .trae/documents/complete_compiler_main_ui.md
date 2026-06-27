# 完成编译器编译主函数、窗口和按钮功能计划

## Summary

当前仓库只有项目名、许可证和一个 `.lc` 示例，尚无编译器源码、构建配置或 UI 运行时。本计划以 `Example/newUI/main.lc` 为第一版验收样例，从零补齐一个最小可运行编译器：读取 `.lc`，识别 `#include [C]` / `#include [C++]`，将 UI DSL 自行转换为 C++/Win32 代码，保留/注入用户导入的 C/C++ 头文件，并调用 `gcc` / `g++` 生成 Windows `.exe`。

用户已确认：

- 编译器需要检测是否导入 C 或 C++ 语法。
- 导入后，如果检测到 C 或 C++ 语句，应调用 `gcc` 或 `g++` 编译。
- UI 语句由编译器自己转换。
- 第一版产物为 `exe`。

## Current State Analysis

### 已确认存在的文件

- `d:\LimdC\README.md`
  - 当前仅包含 `# LimdC`。
- `d:\LimdC\LICENSE`
  - Apache License 2.0。
- `d:\LimdC\Example\newUI\main.lc`
  - 当前唯一示例，包含 include、`fn main`、`window`、`button`、属性赋值和 `return 0;`。

### 示例语法现状

`Example/newUI/main.lc` 当前包含：

- `#include [C++]`：声明启用 C++ 语法。
- `#include [C]`：声明启用 C 语法。
- `#include [C]<stdio.h>`：导入 C 头文件。
- `#include [C++]<bits/stdc++.h>`：导入 C++ 头文件。
- `fn main(){ ... }`：LimdC 主函数。
- `new mainwindow=window{ ... }`：创建窗口。
- `new button of mainwindow{ ... }`：在窗口内创建按钮。
- `.title`、`.h`、`.w`、`.x`、`.y`、`.text`、`.color of button`、`.color of text`：UI 属性。
- `return 0;`：主函数返回。

### 缺口

当前没有：

- 编译器入口程序。
- 词法/语法解析代码。
- AST 或中间表示。
- UI DSL 到目标代码的转换逻辑。
- C/C++ include 和代码片段处理逻辑。
- 调用 `gcc` / `g++` 的编译流程。
- 构建配置或运行说明。

因此本次实现不是补齐已有函数，而是建立最小编译器闭环。

## Proposed Changes

### 1. 新增编译器目录与入口

新增：

- `d:\LimdC\src\main.cpp`

用途：

- 作为编译器主程序入口。
- 命令格式建议：
  - `limdc <input.lc> -o <output.exe>`
  - 若未指定 `-o`，默认输出到输入文件同目录下的同名 `.exe`。

主流程：

1. 解析命令行参数。
2. 读取 `.lc` 文件。
3. 预处理注释和无意义空白。
4. 解析 include、`fn main`、UI 创建块、返回语句和可能的 C/C++ 原生语句。
5. 生成临时 C++ 源码。
6. 根据检测结果选择 `gcc` 或 `g++` 编译。
7. 输出 `.exe`。

选择 C++ 作为编译器实现语言的原因：

- 用户要求检测 C/C++ 语法并调用 `gcc` / `g++`。
- 当前示例直接包含 `[C]` / `[C++]`。
- UI 目标可以直接生成 Win32 C++ 代码，形成最小 Windows 可执行窗口。

### 2. 新增编译器构建配置

新增：

- `d:\LimdC\CMakeLists.txt`

用途：

- 构建编译器可执行文件 `limdc`。
- 使用 C++17 即可满足字符串处理、文件读写和进程调用。

构建方式：

- `cmake -S . -B build`
- `cmake --build build`

如果当前环境没有 CMake，后续可用直接 `g++ src/main.cpp -std=c++17 -o limdc.exe` 作为替代验证方式，但计划中优先提供 CMake。

### 3. 实现最小解析模型

在 `d:\LimdC\src\main.cpp` 内先实现最小结构，不拆分多个文件，避免过度设计。建议内部结构：

- `IncludeInfo`
  - `bool enableC`
  - `bool enableCpp`
  - `vector<string> cHeaders`
  - `vector<string> cppHeaders`
- `Property`
  - `string name`
  - `string target`
  - `string value`
- `WindowSpec`
  - `string name`
  - `string title`
  - `int width`
  - `int height`
- `ButtonSpec`
  - `string parent`
  - `string text`
  - `int x`
  - `int y`
  - `int width`
  - `int height`
  - `string buttonColor`
  - `string textColor`
- `ProgramSpec`
  - include 信息
  - 一个主窗口
  - 多个按钮
  - `returnCode`
  - 原生 C/C++ 代码片段列表

第一版只支持当前示例需要的语法：

- `#include [C]`
- `#include [C++]`
- `#include [C]<...>`
- `#include [C++]<...>`
- `fn main(){ ... }`
- `new name=window{ ... }`
- `new button of parent{ ... }`
- `.key=value`
- `.key of target=value`
- `return number;`
- 注释：`//...` 和 `\\...`

特别处理：

- `Example/newUI/main.lc` 第 13 行 `new button of mainwindow{\` 的行尾反斜杠按容错处理：如果 `{` 后只有反斜杠，解析时忽略该反斜杠。

### 4. 实现 C/C++ 导入与编译器选择

在 `d:\LimdC\src\main.cpp` 中实现规则：

1. 如果检测到 `#include [C++]`、`#include [C++]<...>` 或生成的 UI 代码需要 C++，则使用 `g++`。
2. 如果只检测到 `#include [C]` / `#include [C]<...>` 且没有 UI，则可使用 `gcc`。
3. 由于窗口和按钮将生成 Win32 C++ 代码，当前示例最终使用 `g++`。
4. 导入头文件时：
   - `[C]<stdio.h>` 生成 `#include <stdio.h>`。
   - `[C++]<bits/stdc++.h>` 生成 `#include <bits/stdc++.h>`。
5. `[C]` / `[C++]` 无头文件版本只作为启用语法标记，不直接生成 include。

### 5. 实现 UI DSL 到 Win32 C++ 的转换

在 `d:\LimdC\src\main.cpp` 中生成临时 C++ 文件，目标是 Windows Win32 API。

窗口映射：

- `new mainwindow=window{ ... }`
  - `.title` → 窗口标题。
  - `.w` → 窗口宽度。
  - `.h` → 窗口高度。

按钮映射：

- `new button of mainwindow{ ... }`
  - `.text` → 按钮文字。
  - `.x` / `.y` → 控件位置。
  - `.w` / `.h` → 控件大小。
  - `.color of button` → 第一版可解析并保留为颜色值；如实现复杂度允许，在 `WM_CTLCOLORBTN` 或自绘按钮中使用。若 Win32 原生按钮背景色受主题限制，第一版至少应不报错，并可在生成代码中准备颜色常量。
  - `.color of text` → 在控件颜色处理里用于文本颜色；如果按钮背景色限制导致无法完整应用，仍保留解析结果并尽量用于绘制。

生成代码核心结构：

- `#include <windows.h>`
- 用户导入的 C/C++ 头文件。
- 全局按钮句柄和颜色变量。
- `WndProc`。
- `WinMain`。
- `RegisterClassEx`。
- `CreateWindowEx` 创建主窗口。
- `CreateWindowEx` 创建按钮控件。
- 消息循环。

### 6. 原生 C/C++ 语句处理边界

第一版只做有限支持，避免实现完整 C/C++ 解析器：

- 在 `fn main` 内，无法识别为 LimdC UI 语句或 `return` 的普通语句，如果已启用 `[C]` 或 `[C++]`，按原生代码片段保存。
- 生成 Win32 程序时，原生代码片段插入到窗口创建前或 `WinMain` 中的安全位置。
- 如果未启用 `[C]` / `[C++]` 却出现无法识别语句，编译器报错并提示需要导入对应语法。

选择规则：

- 含 `[C++]` 或 UI → `g++`。
- 仅 `[C]` 且无 UI → `gcc`。
- 示例包含 UI，所以固定走 `g++` + Win32 链接。

### 7. 生成文件位置

建议生成：

- 临时 C++ 文件：输入文件同目录下 `main.generated.cpp`，或输出目录下 `<output>.generated.cpp`。
- 最终 exe：用户通过 `-o` 指定；未指定时为输入文件同目录下 `main.exe`。

为便于调试，第一版保留 generated cpp，不自动删除。

### 8. 更新 README 使用说明

修改：

- `d:\LimdC\README.md`

补充：

- 项目简介。
- 编译器构建命令。
- 示例编译命令。
- 依赖：Windows、CMake 或 g++、MinGW-w64 的 `g++`。
- 示例：
  - `limdc Example\newUI\main.lc -o Example\newUI\main.exe`

### 9. 项目上下文文件处理

当前未发现：

- `d:\LimdC\CLAUDE.md`
- `d:\LimdC\LESSONS_LEARNED.md`

由于当前处于 Plan Mode，除本计划文件外不应修改其他文件。执行阶段可按项目上下文约定新增这两个文件，但只有在不影响用户目标时简洁创建：

- `CLAUDE.md`：记录项目结构、构建命令、编译器设计约束。
- `LESSONS_LEARNED.md`：记录本次实现经验。

若执行时严格控制改动范围，也可以先不创建，优先完成编译器功能。

## Assumptions & Decisions

1. 第一版面向 Windows，因为用户当前环境是 Windows，且窗口/按钮最小实现使用 Win32 API。
2. 编译器本身用 C++ 实现，目标代码也生成 C++。
3. 当前示例是第一优先级验收标准。
4. `#include [C]` / `#include [C++]` 是语法启用声明；带 `<...>` 的版本同时生成真实 include。
5. UI DSL 不交给 gcc/g++ 解析，而是在编译器内转成 Win32 C++。
6. 第一版不实现完整 C/C++ 解析器，只转发无法识别但用户已显式启用 C/C++ 的语句片段。
7. 第 13 行行尾 `\` 作为容错忽略，不作为必须保留的语言特性。
8. 如果系统没有 `g++` / MinGW-w64，编译器应给出清晰错误，而不是静默失败。

## Verification Steps

执行阶段完成后，按以下步骤验证：

1. 构建编译器：
   - `cmake -S . -B build`
   - `cmake --build build`
2. 使用示例编译 exe：
   - `build\Debug\limdc.exe Example\newUI\main.lc -o Example\newUI\main.exe`
   - 或根据实际生成路径使用 `build\limdc.exe`。
3. 确认生成文件：
   - `Example\newUI\main.generated.cpp`
   - `Example\newUI\main.exe`
4. 若环境支持 GUI，运行：
   - `Example\newUI\main.exe`
5. 验收窗口：
   - 窗口标题为 `title`。
   - 窗口尺寸近似使用 `.w=300`、`.h=500`。
   - 窗口内有一个按钮。
   - 按钮文字为 `A button`。
   - 按钮位置近似 `.x=10`、`.y=5`。
   - 按钮尺寸近似 `.w=25`、`.h=10`。
6. 验证 include 处理：
   - generated cpp 包含 `<stdio.h>` 和 `<bits/stdc++.h>`。
7. 验证错误路径：
   - 输入文件不存在时报错。
   - 没有 `fn main` 时报错。
   - 未定义父窗口时报错。
   - 系统缺少 `g++` 时报错并显示编译命令。
