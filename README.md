# LimdC

LimdC 当前提供一个最小 `.lc` 编译器原型：读取 `.lc` 示例，识别 `#include [C]` / `#include [C++]`，将 `window` 和 `button` UI 语句转换为 Win32 C++，再调用 `gcc` / `g++` 生成 exe。

## 依赖

- Windows
- CMake 3.16+
- MinGW-w64 或其他可在命令行使用的 `gcc` / `g++`

## 构建编译器

```powershell
cmake -S . -B build
cmake --build build
```

也可以直接使用 g++ 构建：

```powershell
g++ src/main.cpp -std=c++17 -o limdc.exe
```

## 编译示例

```powershell
build\Debug\limdc.exe Example\newUI\main.lc -o Example\newUI\main.exe
```

如果生成器位于其他目录，请替换为实际 `limdc.exe` 路径。

编译成功后会保留生成的 C++ 文件，例如：

```text
Example\newUI\main.generated.cpp
```

## 当前支持的 `.lc` 语法

- `#include [C]`
- `#include [C++]`
- `#include [C]<...>`
- `#include [C++]<...>`
- `fn main(){ ... }`
- `new name=window{ ... }`
- `new button of parent{ ... }`
- `.key=value`
- `.color of button=#RRGGBB`
- `.color of text=#RRGGBB`
- `return number;`
- `//` 和 `\\` 行注释

UI 语句由 LimdC 编译器转换；普通 C/C++ 语句需要先导入 `[C]` 或 `[C++]`。
