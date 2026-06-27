#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct CleanLine {
    int number = 0;
    std::string text;
};

struct IncludeInfo {
    bool enableC = false;
    bool enableCpp = false;
    std::vector<std::string> cHeaders;
    std::vector<std::string> cppHeaders;
};

struct WindowSpec {
    std::string name;
    std::string title = "LimdC";
    int width = 800;
    int height = 600;
};

struct ButtonSpec {
    std::string parent;
    std::string name;
    std::string text = "Button";
    int x = 0;
    int y = 0;
    int width = 80;
    int height = 25;
    std::string buttonColor = "#F0F0F0";
    std::string textColor = "#000000";
    std::string corner = "square";
    std::string clickHandler;
};

struct UiAction {
    std::string objectName;
    std::string property;
    std::string value;
};

struct FunctionSpec {
    std::string name;
    std::vector<UiAction> actions;
};

struct ProgramSpec {
    IncludeInfo includes;
    std::optional<WindowSpec> window;
    std::vector<ButtonSpec> buttons;
    std::vector<FunctionSpec> functions;
    std::vector<std::string> nativeStatements;
    int returnCode = 0;
};

struct Args {
    fs::path input;
    fs::path output;
};

static std::string ltrim(std::string value) {
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    return value;
}

static std::string rtrim(std::string value) {
    value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), value.end());
    return value;
}

static std::string trim(std::string value) {
    return rtrim(ltrim(std::move(value)));
}

static bool startsWith(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

static std::string stripLineComment(const std::string& line) {
    bool inString = false;
    char quote = '\0';

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (inString) {
            if (ch == '\\') {
                ++i;
                continue;
            }
            if (ch == quote) {
                inString = false;
            }
            continue;
        }

        if (ch == '"' || ch == '\'') {
            inString = true;
            quote = ch;
            continue;
        }

        if (ch == '/' && i + 1 < line.size() && line[i + 1] == '/') {
            return line.substr(0, i);
        }
        if (ch == '\\' && i + 1 < line.size() && line[i + 1] == '\\') {
            return line.substr(0, i);
        }
    }

    return line;
}

static std::string normalizeLine(const std::string& rawLine) {
    std::string line = trim(stripLineComment(rawLine));
    while (!line.empty() && line.back() == '\\') {
        line.pop_back();
        line = rtrim(std::move(line));
    }
    return line;
}

static std::string readFile(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("无法读取输入文件: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

static void writeFile(const fs::path& path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("无法写入文件: " + path.string());
    }
    output << content;
}

static std::vector<CleanLine> preprocess(const std::string& source) {
    std::vector<CleanLine> lines;
    std::istringstream stream(source);
    std::string rawLine;
    int lineNumber = 1;

    while (std::getline(stream, rawLine)) {
        std::string line = normalizeLine(rawLine);
        if (!line.empty()) {
            lines.push_back({lineNumber, line});
        }
        ++lineNumber;
    }

    return lines;
}

static std::runtime_error lineError(const CleanLine& line, const std::string& message) {
    return std::runtime_error("第 " + std::to_string(line.number) + " 行: " + message);
}

static bool parseInclude(const CleanLine& line, IncludeInfo& includes) {
    static const std::regex pattern(R"(^#include\s*\[(C\+\+|C)\]\s*(?:<([^>]+)>)?\s*$)");
    std::smatch match;
    if (!std::regex_match(line.text, match, pattern)) {
        return false;
    }

    const std::string kind = match[1].str();
    const std::string header = match[2].matched ? trim(match[2].str()) : "";

    if (kind == "C") {
        includes.enableC = true;
        if (!header.empty()) {
            includes.cHeaders.push_back(header);
        }
    } else {
        includes.enableCpp = true;
        if (!header.empty()) {
            includes.cppHeaders.push_back(header);
        }
    }

    return true;
}

static std::string unquote(std::string value) {
    value = trim(std::move(value));
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) {
        std::string result;
        for (std::size_t i = 1; i + 1 < value.size(); ++i) {
            if (value[i] == '\\' && i + 2 < value.size()) {
                ++i;
            }
            result.push_back(value[i]);
        }
        return result;
    }
    return value;
}

static int parseInteger(const CleanLine& line, const std::string& value, const std::string& field) {
    try {
        std::size_t consumed = 0;
        int parsed = std::stoi(trim(value), &consumed, 10);
        if (consumed != trim(value).size()) {
            throw std::invalid_argument("extra characters");
        }
        return parsed;
    } catch (const std::exception&) {
        throw lineError(line, field + " 需要整数值");
    }
}

static std::string parseColor(const CleanLine& line, std::string value) {
    value = trim(std::move(value));
    static const std::regex colorPattern(R"(^#[0-9a-fA-F]{6}$)");
    if (!std::regex_match(value, colorPattern)) {
        throw lineError(line, "颜色需要使用 #RRGGBB 格式");
    }
    return value;
}

static std::string parseCorner(const CleanLine& line, std::string value) {
    value = trim(std::move(value));
    if (value != "circle" && value != "square") {
        throw lineError(line, ".corner 只能是 circle 或 square");
    }
    return value;
}

static std::string parseCallName(const CleanLine& line, std::string value) {
    value = trim(std::move(value));
    static const std::regex callPattern(R"(^([A-Za-z_]\w*)\s*\(\s*\)\s*;?\s*$)");
    std::smatch match;
    if (!std::regex_match(value, match, callPattern)) {
        throw lineError(line, ".click 需要使用 functionName() 格式");
    }
    return match[1].str();
}

static std::vector<std::pair<CleanLine, std::smatch>> parsePropertyBlock(
    const std::vector<CleanLine>& lines,
    std::size_t& index
) {
    static const std::regex propertyPattern(R"(^\.([A-Za-z_]\w*)(?:\s+of\s+([A-Za-z_]\w*))?\s*=\s*(.+)$)");
    std::vector<std::pair<CleanLine, std::smatch>> properties;

    for (++index; index < lines.size(); ++index) {
        const CleanLine& line = lines[index];
        if (line.text == "}") {
            return properties;
        }

        std::smatch match;
        if (!std::regex_match(line.text, match, propertyPattern)) {
            throw lineError(line, "无法解析属性语句: " + line.text);
        }
        properties.push_back({line, match});
    }

    throw std::runtime_error("对象块缺少结束的 }");
}

static WindowSpec makeWindow(const CleanLine& line, const std::string& name, const std::vector<std::pair<CleanLine, std::smatch>>& properties) {
    WindowSpec window;
    window.name = name;

    for (const auto& item : properties) {
        const CleanLine& propertyLine = item.first;
        const std::smatch& match = item.second;
        const std::string key = match[1].str();
        const std::string target = match[2].matched ? match[2].str() : "";
        const std::string value = trim(match[3].str());

        if (!target.empty()) {
            throw lineError(propertyLine, "window 属性不支持 of 目标");
        }
        if (key == "title") {
            window.title = unquote(value);
        } else if (key == "w") {
            window.width = parseInteger(propertyLine, value, ".w");
        } else if (key == "h") {
            window.height = parseInteger(propertyLine, value, ".h");
        } else {
            throw lineError(propertyLine, "未知 window 属性: " + key);
        }
    }

    if (window.width <= 0 || window.height <= 0) {
        throw lineError(line, "window 的 .w 和 .h 必须大于 0");
    }

    return window;
}

static ButtonSpec makeButton(const std::string& parent, const std::vector<std::pair<CleanLine, std::smatch>>& properties) {
    ButtonSpec button;
    button.parent = parent;

    for (const auto& item : properties) {
        const CleanLine& propertyLine = item.first;
        const std::smatch& match = item.second;
        const std::string key = match[1].str();
        const std::string target = match[2].matched ? match[2].str() : "";
        const std::string value = trim(match[3].str());

        if (key == "text" && target.empty()) {
            button.text = unquote(value);
        } else if (key == "x" && target.empty()) {
            button.x = parseInteger(propertyLine, value, ".x");
        } else if (key == "y" && target.empty()) {
            button.y = parseInteger(propertyLine, value, ".y");
        } else if (key == "w" && target.empty()) {
            button.width = parseInteger(propertyLine, value, ".w");
        } else if (key == "h" && target.empty()) {
            button.height = parseInteger(propertyLine, value, ".h");
        } else if (key == "color" && target == "button") {
            button.buttonColor = parseColor(propertyLine, value);
        } else if (key == "color" && target == "text") {
            button.textColor = parseColor(propertyLine, value);
        } else if (key == "corner" && target.empty()) {
            button.corner = parseCorner(propertyLine, value);
        } else if (key == "click" && target.empty()) {
            button.clickHandler = parseCallName(propertyLine, value);
        } else {
            throw lineError(propertyLine, "未知 button 属性: " + propertyLine.text);
        }
    }

    if (button.width <= 0 || button.height <= 0) {
        throw std::runtime_error("button 的 .w 和 .h 必须大于 0");
    }

    return button;
}

static void applyUiAction(ButtonSpec& button, const UiAction& action) {
    if (action.property == "color of button") {
        button.buttonColor = action.value;
    } else if (action.property == "color of text") {
        button.textColor = action.value;
    } else if (action.property == "text") {
        button.text = unquote(action.value);
    } else {
        throw std::runtime_error("暂不支持修改按钮属性: " + action.property);
    }
}

static FunctionSpec parseFunctionBlock(const std::vector<CleanLine>& lines, std::size_t& index) {
    static const std::regex functionPattern(R"(^fn\s+([A-Za-z_]\w*)\s*\(\s*\)\s*\{\s*$)");
    static const std::regex actionPattern(R"(^([A-Za-z_]\w*)\.(?:'([^']+)'|([A-Za-z_]\w*))\s*=\s*(.+?)\s*;?\s*$)");

    std::smatch functionMatch;
    if (!std::regex_match(lines[index].text, functionMatch, functionPattern)) {
        throw lineError(lines[index], "无法解析函数定义");
    }

    FunctionSpec function;
    function.name = functionMatch[1].str();

    for (++index; index < lines.size(); ++index) {
        const CleanLine& line = lines[index];
        if (line.text == "}") {
            return function;
        }

        std::smatch actionMatch;
        if (!std::regex_match(line.text, actionMatch, actionPattern)) {
            throw lineError(line, "无法解析函数语句: " + line.text);
        }

        const std::string property = actionMatch[2].matched ? actionMatch[2].str() : actionMatch[3].str();
        std::string value = trim(actionMatch[4].str());
        if (property == "color of button" || property == "color of text") {
            value = parseColor(line, value);
        }
        function.actions.push_back({actionMatch[1].str(), property, value});
    }

    throw std::runtime_error("函数 " + function.name + " 缺少结束的 }");
}

static ProgramSpec parseProgram(const std::string& source) {
    ProgramSpec program;
    std::vector<CleanLine> bodyLines;
    const std::vector<CleanLine> lines = preprocess(source);

    for (const CleanLine& line : lines) {
        if (!parseInclude(line, program.includes)) {
            bodyLines.push_back(line);
        }
    }

    std::size_t mainIndex = bodyLines.size();
    for (std::size_t i = 0; i < bodyLines.size(); ++i) {
        if (std::regex_match(bodyLines[i].text, std::regex(R"(^fn\s+main\s*\(\s*\)\s*\{\s*$)"))) {
            mainIndex = i;
            break;
        }
    }

    if (mainIndex == bodyLines.size()) {
        throw std::runtime_error("没有找到 fn main(){ ... } 主函数");
    }

    static const std::regex windowPattern(R"(^new\s+([A-Za-z_]\w*)\s*=\s*window\s*\{\s*$)");
    static const std::regex buttonPattern(R"(^new\s+button\s+of\s+([A-Za-z_]\w*)\s*(?:=\s*([A-Za-z_]\w*))?\s*\{\s*$)");
    static const std::regex returnPattern(R"(^return\s+(-?\d+)\s*;?\s*$)");

    bool foundMainEnd = false;
    std::size_t afterMainIndex = mainIndex + 1;
    for (std::size_t i = mainIndex + 1; i < bodyLines.size(); ++i) {
        const CleanLine& line = bodyLines[i];
        if (line.text == "}") {
            foundMainEnd = true;
            afterMainIndex = i + 1;
            break;
        }

        std::smatch match;
        if (std::regex_match(line.text, match, windowPattern)) {
            if (program.window.has_value()) {
                throw lineError(line, "第一版只支持一个 window");
            }
            const std::string name = match[1].str();
            const auto properties = parsePropertyBlock(bodyLines, i);
            program.window = makeWindow(line, name, properties);
            continue;
        }

        if (std::regex_match(line.text, match, buttonPattern)) {
            const std::string parent = match[1].str();
            const auto properties = parsePropertyBlock(bodyLines, i);
            ButtonSpec button = makeButton(parent, properties);
            button.name = match[2].matched ? match[2].str() : "button" + std::to_string(program.buttons.size());
            program.buttons.push_back(button);
            continue;
        }

        if (std::regex_match(line.text, match, returnPattern)) {
            program.returnCode = parseInteger(line, match[1].str(), "return");
            continue;
        }

        if (program.includes.enableC || program.includes.enableCpp) {
            program.nativeStatements.push_back(line.text);
            continue;
        }

        throw lineError(line, "无法识别语句，若要使用 C/C++ 语句请先 #include [C] 或 #include [C++]");
    }

    if (!foundMainEnd) {
        throw std::runtime_error("fn main 缺少结束的 }");
    }

    for (std::size_t i = afterMainIndex; i < bodyLines.size(); ++i) {
        if (std::regex_match(bodyLines[i].text, std::regex(R"(^fn\s+[A-Za-z_]\w*\s*\(\s*\)\s*\{\s*$)"))) {
            program.functions.push_back(parseFunctionBlock(bodyLines, i));
            continue;
        }

        throw lineError(bodyLines[i], "主函数外只支持 fn name(){ ... } 函数定义");
    }

    if (!program.buttons.empty() && !program.window.has_value()) {
        throw std::runtime_error("button 需要先定义父 window");
    }

    if (program.window.has_value()) {
        for (const ButtonSpec& button : program.buttons) {
            if (button.parent != program.window->name) {
                throw std::runtime_error("button 的父窗口未定义: " + button.parent);
            }
        }
    }

    for (const FunctionSpec& function : program.functions) {
        for (const UiAction& action : function.actions) {
            const bool foundTarget = std::any_of(program.buttons.begin(), program.buttons.end(), [&](const ButtonSpec& button) {
                return button.name == action.objectName;
            });
            if (!foundTarget) {
                throw std::runtime_error("函数 " + function.name + " 修改了未定义对象: " + action.objectName);
            }
            if (action.property != "color of button" && action.property != "color of text") {
                throw std::runtime_error("暂不支持修改按钮属性: " + action.property);
            }
        }
    }

    for (const ButtonSpec& button : program.buttons) {
        if (button.clickHandler.empty()) {
            continue;
        }
        const bool foundHandler = std::any_of(program.functions.begin(), program.functions.end(), [&](const FunctionSpec& function) {
            return function.name == button.clickHandler;
        });
        if (!foundHandler) {
            throw std::runtime_error("button " + button.name + " 绑定了未定义 click 函数: " + button.clickHandler);
        }
    }

    return program;
}

static std::string cppStringLiteral(const std::string& value) {
    std::string result = "\"";
    for (const char ch : value) {
        switch (ch) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result.push_back(ch); break;
        }
    }
    result += "\"";
    return result;
}

static std::string colorToRgbCall(const std::string& color) {
    const int r = std::stoi(color.substr(1, 2), nullptr, 16);
    const int g = std::stoi(color.substr(3, 2), nullptr, 16);
    const int b = std::stoi(color.substr(5, 2), nullptr, 16);
    return "RGB(" + std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ")";
}

static int autoButtonFontSize(const ButtonSpec& button) {
    const int heightSize = std::max(5, (button.height * 2) / 5);
    const int textLength = std::max(1, static_cast<int>(button.text.size()));
    const int widthSize = std::max(5, std::max(1, button.width - 8) / textLength);
    return std::max(5, std::min(heightSize, widthSize));
}

static void appendUserIncludes(std::ostringstream& output, const IncludeInfo& includes) {
    for (const std::string& header : includes.cHeaders) {
        output << "#include <" << header << ">\n";
    }
    for (const std::string& header : includes.cppHeaders) {
        output << "#include <" << header << ">\n";
    }
}

static std::string generateConsoleCode(const ProgramSpec& program) {
    std::ostringstream output;
    appendUserIncludes(output, program.includes);
    output << "\nint main() {\n";
    for (const std::string& statement : program.nativeStatements) {
        output << "    " << statement << "\n";
    }
    output << "    return " << program.returnCode << ";\n";
    output << "}\n";
    return output.str();
}

static std::string generateWin32Code(const ProgramSpec& program) {
    const WindowSpec& window = *program.window;
    std::ostringstream output;

    output << "#include <windows.h>\n";
    appendUserIncludes(output, program.includes);
    output << "\n";

    for (std::size_t i = 0; i < program.buttons.size(); ++i) {
        const ButtonSpec& button = program.buttons[i];
        output << "static HWND g_button" << i << " = NULL;\n";
        output << "static HFONT g_button" << i << "Font = NULL;\n";
        output << "static COLORREF g_button" << i << "Bg = " << colorToRgbCall(button.buttonColor) << ";\n";
        output << "static COLORREF g_button" << i << "TextColor = " << colorToRgbCall(button.textColor) << ";\n";
        output << "static const char* g_button" << i << "Text = " << cppStringLiteral(button.text) << ";\n";
        output << "static const int g_button" << i << "Id = " << (1000 + static_cast<int>(i)) << ";\n";
    }

    output << R"(
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT item = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
)";

    for (std::size_t i = 0; i < program.buttons.size(); ++i) {
        const ButtonSpec& button = program.buttons[i];
        output << "            if (item && item->hwndItem == g_button" << i << ") {\n";
        if (button.corner == "circle") {
            output << "                HRGN clipRegion = CreateRoundRectRgn(item->rcItem.left, item->rcItem.top, item->rcItem.right + 1, item->rcItem.bottom + 1, item->rcItem.bottom - item->rcItem.top, item->rcItem.bottom - item->rcItem.top);\n";
            output << "                SelectClipRgn(item->hDC, clipRegion);\n";
        }
        output << "                HBRUSH brush = CreateSolidBrush(g_button" << i << "Bg);\n";
        if (button.corner == "circle") {
            output << "                HBRUSH oldBrush = reinterpret_cast<HBRUSH>(SelectObject(item->hDC, brush));\n";
            output << "                HPEN borderPen = CreatePen(PS_SOLID, 1, g_button" << i << "TextColor);\n";
            output << "                HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(item->hDC, borderPen));\n";
            output << "                RoundRect(item->hDC, item->rcItem.left, item->rcItem.top, item->rcItem.right, item->rcItem.bottom, item->rcItem.bottom - item->rcItem.top, item->rcItem.bottom - item->rcItem.top);\n";
            output << "                SelectObject(item->hDC, oldPen);\n";
            output << "                SelectObject(item->hDC, oldBrush);\n";
            output << "                DeleteObject(borderPen);\n";
        } else {
            output << "                FillRect(item->hDC, &item->rcItem, brush);\n";
        }
        output << "                DeleteObject(brush);\n";
        if (button.corner == "circle") {
            output << "                SelectClipRgn(item->hDC, NULL);\n";
            output << "                DeleteObject(clipRegion);\n";
        }
        output << "                SetBkMode(item->hDC, TRANSPARENT);\n";
        output << "                SetTextColor(item->hDC, g_button" << i << "TextColor);\n";
        output << "                HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(item->hDC, g_button" << i << "Font));\n";
        output << "                SIZE textSize = {};\n";
        output << "                GetTextExtentPoint32A(item->hDC, g_button" << i << "Text, lstrlenA(g_button" << i << "Text), &textSize);\n";
        output << "                const int buttonWidth = item->rcItem.right - item->rcItem.left;\n";
        output << "                const int buttonHeight = item->rcItem.bottom - item->rcItem.top;\n";
        output << "                const int textX = item->rcItem.left + (buttonWidth - textSize.cx) / 2;\n";
        output << "                const int textY = item->rcItem.top + (buttonHeight - textSize.cy) / 2;\n";
        output << "                TextOutA(item->hDC, textX, textY, g_button" << i << "Text, lstrlenA(g_button" << i << "Text));\n";
        output << "                SelectObject(item->hDC, oldFont);\n";
        output << "                if (item->itemState & ODS_FOCUS) { DrawFocusRect(item->hDC, &item->rcItem); }\n";
        output << "                return TRUE;\n";
        output << "            }\n";
    }

    output << R"(            break;
        }
        case WM_COMMAND: {
            const int commandId = LOWORD(wParam);
)";

    for (std::size_t i = 0; i < program.buttons.size(); ++i) {
        const ButtonSpec& button = program.buttons[i];
        if (button.clickHandler.empty()) {
            continue;
        }
        output << "            if (commandId == g_button" << i << "Id) {\n";
        for (const FunctionSpec& function : program.functions) {
            if (function.name != button.clickHandler) {
                continue;
            }
            for (const UiAction& action : function.actions) {
                if (action.objectName != button.name) {
                    continue;
                }
                if (action.property == "color of button") {
                    output << "                g_button" << i << "Bg = " << colorToRgbCall(action.value) << ";\n";
                } else if (action.property == "color of text") {
                    output << "                g_button" << i << "TextColor = " << colorToRgbCall(action.value) << ";\n";
                }
            }
        }
        output << "                InvalidateRect(g_button" << i << ", NULL, TRUE);\n";
        output << "                return 0;\n";
        output << "            }\n";
    }

    output << R"(            break;
        }
        case WM_DESTROY:
)";

    for (std::size_t i = 0; i < program.buttons.size(); ++i) {
        output << "            if (g_button" << i << "Font) { DeleteObject(g_button" << i << "Font); g_button" << i << "Font = NULL; }\n";
    }

    output << R"(            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

int main() {
)";

    for (const std::string& statement : program.nativeStatements) {
        output << "    " << statement << "\n";
    }

    output << "    HINSTANCE hInstance = GetModuleHandleA(NULL);\n";
    output << "    const char* className = \"LimdCWindowClass\";\n";
    output << "    WNDCLASSEXA wc = {};\n";
    output << "    wc.cbSize = sizeof(wc);\n";
    output << "    wc.lpfnWndProc = WndProc;\n";
    output << "    wc.hInstance = hInstance;\n";
    output << "    wc.hCursor = LoadCursor(NULL, IDC_ARROW);\n";
    output << "    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);\n";
    output << "    wc.lpszClassName = className;\n";
    output << "    if (!RegisterClassExA(&wc)) {\n";
    output << "        MessageBoxA(NULL, \"RegisterClassExA failed\", \"LimdC\", MB_ICONERROR);\n";
    output << "        return 1;\n";
    output << "    }\n";
    output << "\n";
    output << "    HWND " << window.name << " = CreateWindowExA(0, className, " << cppStringLiteral(window.title)
           << ", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, " << window.width << ", " << window.height
           << ", NULL, NULL, hInstance, NULL);\n";
    output << "    if (!" << window.name << ") {\n";
    output << "        MessageBoxA(NULL, \"CreateWindowExA failed\", \"LimdC\", MB_ICONERROR);\n";
    output << "        return 1;\n";
    output << "    }\n";
    output << "\n";

    for (std::size_t i = 0; i < program.buttons.size(); ++i) {
        const ButtonSpec& button = program.buttons[i];
        const int fontSize = autoButtonFontSize(button);
        output << "    g_button" << i << "Font = CreateFontA(-" << fontSize
               << ", 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, "
               << "CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, \"Microsoft YaHei UI\");\n";
        output << "    g_button" << i << " = CreateWindowExA(0, \"BUTTON\", g_button" << i << "Text, "
               << "WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, " << button.x << ", " << button.y << ", "
               << button.width << ", " << button.height << ", " << button.parent << ", reinterpret_cast<HMENU>(g_button" << i
               << "Id), hInstance, NULL);\n";
    }

    output << "\n";
    output << "    ShowWindow(" << window.name << ", SW_SHOWDEFAULT);\n";
    output << "    UpdateWindow(" << window.name << ");\n";
    output << "\n";
    output << "    MSG message = {};\n";
    output << "    while (GetMessageA(&message, NULL, 0, 0) > 0) {\n";
    output << "        TranslateMessage(&message);\n";
    output << "        DispatchMessageA(&message);\n";
    output << "    }\n";
    output << "    return static_cast<int>(message.wParam);\n";
    output << "}\n";

    return output.str();
}

static Args parseArgs(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        const std::string value = argv[i];
        if (value == "-o") {
            if (i + 1 >= argc) {
                throw std::runtime_error("-o 后需要输出 exe 路径");
            }
            args.output = argv[++i];
        } else if (args.input.empty()) {
            args.input = value;
        } else {
            throw std::runtime_error("未知参数: " + value);
        }
    }

    if (args.input.empty()) {
        throw std::runtime_error("用法: limdc <input.lc> [-o output.exe]");
    }

    if (args.output.empty()) {
        args.output = args.input.parent_path() / (args.input.stem().string() + ".exe");
    }

    return args;
}

static std::string quoteArg(const fs::path& path) {
    std::string value = fs::absolute(path).string();
    std::string quoted = "\"";
    for (const char ch : value) {
        if (ch == '"') {
            quoted += "\\\"";
        } else {
            quoted.push_back(ch);
        }
    }
    quoted += "\"";
    return quoted;
}

static fs::path generatedPathFor(const fs::path& input, bool useGpp) {
    const std::string extension = useGpp ? ".generated.cpp" : ".generated.c";
    return input.parent_path() / (input.stem().string() + extension);
}

int main(int argc, char** argv) {
    try {
        Args args = parseArgs(argc, argv);
        const std::string source = readFile(args.input);
        const ProgramSpec program = parseProgram(source);
        const bool hasUi = program.window.has_value();
        const bool useGpp = hasUi || program.includes.enableCpp || !program.includes.cppHeaders.empty();
        const fs::path generatedPath = generatedPathFor(args.input, useGpp);
        const std::string generatedCode = hasUi ? generateWin32Code(program) : generateConsoleCode(program);

        writeFile(generatedPath, generatedCode);

        const fs::path tempDir = args.input.parent_path() / ".limdc_tmp";
        fs::create_directories(tempDir);

        std::string compiler = useGpp ? "g++" : "gcc";
        std::string command;
#ifdef _WIN32
        command += "set \"TMP=" + fs::absolute(tempDir).string() + "\" && ";
        command += "set \"TEMP=" + fs::absolute(tempDir).string() + "\" && ";
#else
        command += "TMPDIR=" + quoteArg(tempDir) + " ";
#endif
        command += compiler;
        if (useGpp) {
            command += " -std=c++17";
        }
        command += " " + quoteArg(generatedPath) + " -o " + quoteArg(args.output);
        if (hasUi) {
            command += " -lgdi32 -luser32 -mwindows";
        }

        std::cout << "Generated: " << fs::absolute(generatedPath).string() << '\n';
        std::cout << "Compile: " << command << '\n';

        const int result = std::system(command.c_str());
        if (result != 0) {
            throw std::runtime_error("目标编译失败，请确认已安装 gcc/g++，命令: " + command);
        }

        std::cout << "Output: " << fs::absolute(args.output).string() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "limdc: " << error.what() << '\n';
        return 1;
    }
}
