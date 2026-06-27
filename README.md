# LimdC

LimdC 当前提供一个最小 `.lc` 编译器原型：读取 `.lc` 示例，识别 `#include [C]` / `#include [C++]`，将内置 UI DSL 转换为 Win32 C++，再调用 `gcc` / `g++` 生成 Windows exe。

当前 UI DSL 已支持：窗口、按钮、列表、列表标题、列表卡片背景、列表边框、文本组件和按钮点击修改 UI 属性。

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
.\build\limdc.exe .\Example\newUI\main.lc -o .\Example\newUI\main.exe
```

如果输出 exe 正在运行，Windows 会阻止覆盖文件。请先关闭正在运行的 exe，或换一个输出名：

```powershell
.\build\limdc.exe .\Example\newUI\main.lc -o .\Example\newUI\test.exe
```

编译成功后会保留生成的 C++ 文件，例如：

```text
Example\newUI\main.generated.cpp
```

## 当前支持的 `.lc` 语法

### 导入

```lc
#include [C]
#include [C++]
#include [C]<stdio.h>
#include [C++]<bits/stdc++.h>
```

- `#include [C]` / `#include [C++]` 用于启用原生 C/C++ 语句转发。
- 带 `<...>` 的 include 会输出到生成的 C/C++ 文件。

### 主函数

```lc
fn main(){
    return 0;
}
```

### 窗口

```lc
new mainwindow=window{
    .title="LimdC List Test"
    .w=520
    .h=420
}
```

支持属性：

- `.title="标题"`
- `.w=数字`
- `.h=数字`

### 按钮

```lc
new button of mainwindow=firstButton{
    .x=70
    .y=80
    .w=320
    .h=58
    .text="Button"
    .color of button=#0F172A
    .color of text=#E0F2FE
    .corner=circle
    .click=click()
}
```

支持属性：

- `.x=数字`
- `.y=数字`
- `.w=数字`
- `.h=数字`
- `.text="文字"`
- `.color of button=#RRGGBB`
- `.color of text=#RRGGBB`
- `.corner=circle` 或 `.corner=square`
- `.click=函数名()`

按钮字号会根据按钮大小和文字长度自动计算，文字默认居中绘制。

### 列表

```lc
new list of mainwindow=testList{
    .title="List Card"
    .title color=#1E3A8A
    .x=70
    .y=45
    .aw=320
    .ah=58
    .padding=16
    .background color=#F8FAFC
    .frame=true
    .frame color=#CBD5E1

    new button of * = listButtonA{
        .text="List Item A"
        .color of button=#0F172A
        .color of text=#E0F2FE
        .click=click()
        .corner=circle
    }
}

new button of testList=listButtonB{
    .text="List Item B"
    .color of button=#F59E0B
    .color of text=#FFFFFF
    .corner=circle
}
```

支持属性：

- `.title="标题"`
- `.title color=#RRGGBB`
- `.x=数字`
- `.y=数字`
- `.aw=数字`：列表子组件宽度
- `.ah=数字`：列表子组件高度
- `.padding=数字`：列表内边距
- `.background color=#RRGGBB`
- `.frame=true` / `.frame=false`
- `.frame color=#RRGGBB`

列表子组件写法：

- `new button of * = name{ ... }`：在 list 块内部，`*` 表示当前 list。
- `new button of listName=name{ ... }`：在 list 块外向指定 list 追加组件。

列表子按钮会按 `.ah` 自动垂直排列。

### 文本组件

```lc
string txt = "hallo";

new text=text1{
    .x=70
    .y=310
    .w=320
    .h=40
    .text=printf(txt)
    .color=#FFFFFF
    .size=18
    .background color=#111827
    .center=0
}
```

支持属性：

- `.x=数字`
- `.y=数字`
- `.w=数字`
- `.h=数字`
- `.text="字符串"`
- `.text=变量名`
- `.text=printf(变量名)`
- `.text=cout<<变量名`
- `.color=#RRGGBB`
- `.size=数字`
- `.background color=#RRGGBB`
- `.center=0` / `.center=1`

文本默认从左上角开始显示；只有 `.center=1` 时才居中。

### 点击函数和 UI 属性修改

```lc
fn click(){
    listButtonA.'color of text'=#FFEE58;
}
```

当前函数内支持修改按钮属性：

- `buttonName.'color of button'=#RRGGBB;`
- `buttonName.'color of text'=#RRGGBB;`
- `buttonName.text="新文字";`

属性名包含空格时使用单引号，例如 `'color of text'`。

### 注释

```lc
// 行注释
\ 行注释
```

## 示例文件

当前主要示例位于：

```text
Example\newUI\main.lc
```

示例覆盖窗口、列表、列表内按钮、列表外追加按钮、文本组件和点击事件。

## 设计约束

- 第一版面向 Windows，UI 生成 Win32 C++。
- UI 语句由 LimdC 编译器转换，不交给 gcc/g++ 直接解析。
- 非 UI 的 C/C++ 语句仅在启用 `[C]` 或 `[C++]` 后按原生语句转发。
- 当前主要支持一个主窗口、多个按钮、多个列表和多个文本组件。
