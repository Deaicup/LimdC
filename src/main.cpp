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

struct ListSpec {
    std::string parent;
    std::string name;
    std::string title;
    std::string titleColor = "#111827";
    bool frame = false;
    std::string frameColor = "#D1D5DB";
    std::string backgroundColor = "#F9FAFB";
    int padding = 12;
    int x = 0;
    int y = 0;
    int itemWidth = 80;
    int itemHeight = 25;
};

struct TextSpec {
    std::string name;
    std::string textExpression = "\"Text\"";
    int x = 70;
    int y = 315;
    int width = 320;
    int height = 32;
    std::string textColor = "#000000";
    std::string backgroundColor = "#FFFFFF";
    int size = 14;
    bool center = false;
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
    std::vector<ListSpec> lists;
    std::vector<ButtonSpec> buttons;
    std::vector<TextSpec> texts;
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

static bool parseBoolean(const CleanLine& line, std::string value, const std::string& field) {
    value = trim(std::move(value));
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw lineError(line, field + " 只能是 true 或 false");
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

static bool isListPropertyLine(const std::string& text) {
    static const std::regex propertyPattern(R"(^\.(title|title\s+color|x|y|aw|ah|frame|frame\s+color|background\s+color|padding)\s*=\s*.+$)");
    return std::regex_match(text, propertyPattern);
}

static std::vector<std::pair<CleanLine, std::smatch>> parsePropertyBlock(
    const std::vector<CleanLine>& lines,
    std::size_t& index
);

static ButtonSpec makeButton(const std::string& parent, const std::vector<std::pair<CleanLine, std::smatch>>& properties);

static std::vector<std::pair<CleanLine, std::smatch>> parseListPropertyBlock(
    const std::vector<CleanLine>& lines,
    std::size_t& index,
    std::vector<ButtonSpec>& buttons,
    const std::string& listName,
    int itemWidth,
    int itemHeight
) {
    static const std::regex propertyPattern(R"(^\.([A-Za-z_]\w*(?:\s+[A-Za-z_]\w*)?)(?:\s+of\s+([A-Za-z_]\w*))?\s*=\s*(.+)$)");
    static const std::regex childButtonPattern(R"(^new\s+button\s+of\s+\*\s*=\s*([A-Za-z_]\w*)\s*\{\s*$)");
    std::vector<std::pair<CleanLine, std::smatch>> properties;

    for (++index; index < lines.size(); ++index) {
        const CleanLine& line = lines[index];
        if (line.text == "}") {
            return properties;
        }

        std::smatch match;
        if (std::regex_match(line.text, match, childButtonPattern)) {
            const auto childProperties = parsePropertyBlock(lines, index);
            ButtonSpec button = makeButton(listName, childProperties);
            button.name = match[1].str();
            button.width = itemWidth;
            button.height = itemHeight;
            button.x = 0;
            button.y = 0;
            buttons.push_back(button);
            continue;
        }

        if (!std::regex_match(line.text, match, propertyPattern)) {
            throw lineError(line, "无法解析 list 属性语句: " + line.text);
        }
        properties.push_back({line, match});
    }

    throw std::runtime_error("list 对象块缺少结束的 }");
}

static std::vector<std::pair<CleanLine, std::smatch>> parsePropertyBlock(
    const std::vector<CleanLine>& lines,
    std::size_t& index
) {
    static const std::regex propertyPattern(R"(^\.([A-Za-z_]\w*(?:\s+[A-Za-z_]\w*)?)(?:\s+of\s+([A-Za-z_]\w*))?\s*=\s*(.+)$)");
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

static ListSpec makeList(
    const CleanLine& line,
    const std::string& parent,
    const std::string& name,
    const std::vector<std::pair<CleanLine, std::smatch>>& properties
) {
    ListSpec list;
    list.parent = parent;
    list.name = name;

    for (const auto& item : properties) {
        const CleanLine& propertyLine = item.first;
        const std::smatch& match = item.second;
        const std::string key = match[1].str();
        const std::string target = match[2].matched ? match[2].str() : "";
        const std::string value = trim(match[3].str());

        if (!target.empty()) {
            throw lineError(propertyLine, "list 属性不支持 of 目标");
        }
        if (key == "title") {
            list.title = unquote(value);
        } else if (key == "title color") {
            list.titleColor = parseColor(propertyLine, value);
        } else if (key == "frame") {
            list.frame = parseBoolean(propertyLine, value, ".frame");
        } else if (key == "frame color") {
            list.frameColor = parseColor(propertyLine, value);
        } else if (key == "background color") {
            list.backgroundColor = parseColor(propertyLine, value);
        } else if (key == "padding") {
            list.padding = parseInteger(propertyLine, value, ".padding");
        } else if (key == "x") {
            list.x = parseInteger(propertyLine, value, ".x");
        } else if (key == "y") {
            list.y = parseInteger(propertyLine, value, ".y");
        } else if (key == "aw") {
            list.itemWidth = parseInteger(propertyLine, value, ".aw");
        } else if (key == "ah") {
            list.itemHeight = parseInteger(propertyLine, value, ".ah");
        } else {
            throw lineError(propertyLine, "未知 list 属性: " + key);
        }
    }

    if (list.itemWidth <= 0 || list.itemHeight <= 0) {
        throw lineError(line, "list 的 .aw 和 .ah 必须大于 0");
    }
    if (list.padding < 0) {
        throw lineError(line, "list 的 .padding 不能小于 0");
    }

    return list;
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

static std::string cppStringLiteral(const std::string& value);

static std::string parseTextExpression(const CleanLine& line, std::string value) {
    value = trim(std::move(value));
    std::smatch match;
    if (std::regex_match(value, match, std::regex(R"(^printf\s*\(\s*([A-Za-z_]\w*)\s*\)\s*;?\s*$)"))) {
        return match[1].str();
    }
    if (std::regex_match(value, match, std::regex(R"(^cout\s*<<\s*([A-Za-z_]\w*)\s*;?\s*$)"))) {
        return match[1].str();
    }
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) {
        return cppStringLiteral(unquote(value));
    }
    if (std::regex_match(value, std::regex(R"(^[A-Za-z_]\w*$)"))) {
        return value;
    }
    throw lineError(line, ".text 需要字符串、变量名、printf(var) 或 cout<<var");
}

static TextSpec makeText(const CleanLine& line, const std::string& name, const std::vector<std::pair<CleanLine, std::smatch>>& properties) {
    TextSpec text;
    text.name = name;

    for (const auto& item : properties) {
        const CleanLine& propertyLine = item.first;
        const std::smatch& match = item.second;
        const std::string key = match[1].str();
        const std::string target = match[2].matched ? match[2].str() : "";
        const std::string value = trim(match[3].str());

        if (!target.empty()) {
            throw lineError(propertyLine, "text 属性不支持 of 目标");
        }
        if (key == "text") {
            text.textExpression = parseTextExpression(propertyLine, value);
        } else if (key == "color") {
            text.textColor = parseColor(propertyLine, value);
        } else if (key == "background color") {
            text.backgroundColor = parseColor(propertyLine, value);
        } else if (key == "size") {
            text.size = parseInteger(propertyLine, value, ".size");
        } else if (key == "center") {
            const int center = parseInteger(propertyLine, value, ".center");
            if (center != 0 && center != 1) {
                throw lineError(propertyLine, ".center 只能是 0 或 1");
            }
            text.center = center == 1;
        } else if (key == "x") {
            text.x = parseInteger(propertyLine, value, ".x");
        } else if (key == "y") {
            text.y = parseInteger(propertyLine, value, ".y");
        } else if (key == "w") {
            text.width = parseInteger(propertyLine, value, ".w");
        } else if (key == "h") {
            text.height = parseInteger(propertyLine, value, ".h");
        } else {
            throw lineError(propertyLine, "未知 text 属性: " + key);
        }
    }

    if (text.size <= 0 || text.width <= 0 || text.height <= 0) {
        throw lineError(line, "text 的 .size/.w/.h 必须大于 0");
    }

    return text;
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
    static const std::regex listPattern(R"(^new\s+list\s+of\s+([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*)\s*\{\s*$)");
    static const std::regex buttonPattern(R"(^new\s+button\s+of\s+([A-Za-z_]\w*)\s*(?:=\s*([A-Za-z_]\w*))?\s*\{\s*$)");
    static const std::regex textPattern(R"(^new\s+text\s*=\s*([A-Za-z_]\w*)\s*\{\s*$)");
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

        if (std::regex_match(line.text, match, listPattern)) {
            const std::string parent = match[1].str();
            const std::string name = match[2].str();
            int listX = 0;
            int listY = 0;
            int itemWidth = 80;
            int itemHeight = 25;
            std::size_t scan = i + 1;
            for (; scan < bodyLines.size(); ++scan) {
                if (bodyLines[scan].text == "}" || startsWith(bodyLines[scan].text, "new ")) {
                    break;
                }
                if (!isListPropertyLine(bodyLines[scan].text)) {
                    break;
                }
                std::smatch propertyMatch;
                std::regex_match(bodyLines[scan].text, propertyMatch, std::regex(R"(^\.([A-Za-z_]\w*(?:\s+[A-Za-z_]\w*)?)\s*=\s*(.+)$)"));
                const std::string key = propertyMatch[1].str();
                const std::string value = propertyMatch[2].str();
                if (key == "x") {
                    listX = parseInteger(bodyLines[scan], value, ".x");
                } else if (key == "y") {
                    listY = parseInteger(bodyLines[scan], value, ".y");
                } else if (key == "aw") {
                    itemWidth = parseInteger(bodyLines[scan], value, ".aw");
                } else if (key == "ah") {
                    itemHeight = parseInteger(bodyLines[scan], value, ".ah");
                }
            }
            const auto properties = parseListPropertyBlock(bodyLines, i, program.buttons, name, itemWidth, itemHeight);
            ListSpec list = makeList(line, parent, name, properties);
            program.lists.push_back(list);
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

        if (std::regex_match(line.text, match, textPattern)) {
            const auto properties = parsePropertyBlock(bodyLines, i);
            program.texts.push_back(makeText(line, match[1].str(), properties));
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
    if (!program.texts.empty() && !program.window.has_value()) {
        throw std::runtime_error("text 需要先定义 window");
    }

    if (program.window.has_value()) {
        for (const ListSpec& list : program.lists) {
            if (list.parent != program.window->name) {
                throw std::runtime_error("list 的父窗口未定义: " + list.parent);
            }
        }

        for (const ListSpec& list : program.lists) {
            int itemIndex = 0;
            for (ButtonSpec& button : program.buttons) {
                if (button.parent == list.name) {
                    button.x = 0;
                    button.y = itemIndex * list.itemHeight;
                    button.width = list.itemWidth;
                    button.height = list.itemHeight;
                    ++itemIndex;
                }
            }
        }

        for (const ButtonSpec& button : program.buttons) {
            const bool parentIsWindow = button.parent == program.window->name;
            const bool parentIsList = std::any_of(program.lists.begin(), program.lists.end(), [&](const ListSpec& list) {
                return button.parent == list.name;
            });
            if (!parentIsWindow && !parentIsList) {
                throw std::runtime_error("button 的父对象未定义: " + button.parent);
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

static std::string parentExpression(const ProgramSpec& program, const ButtonSpec& button) {
    for (const ListSpec& list : program.lists) {
        if (button.parent == list.name) {
            return list.parent;
        }
    }
    return button.parent;
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
    output << "#include <string>\n";
    output << "#include <sstream>\n";
    appendUserIncludes(output, program.includes);
    output << "\n";
    if (program.includes.enableCpp || !program.includes.cppHeaders.empty() || !program.texts.empty()) {
        output << "using namespace std;\n\n";
    }

    for (std::size_t i = 0; i < program.buttons.size(); ++i) {
        const ButtonSpec& button = program.buttons[i];
        output << "static HWND g_button" << i << " = NULL;\n";
        output << "static HFONT g_button" << i << "Font = NULL;\n";
        output << "static COLORREF g_button" << i << "Bg = " << colorToRgbCall(button.buttonColor) << ";\n";
        output << "static COLORREF g_button" << i << "TextColor = " << colorToRgbCall(button.textColor) << ";\n";
        output << "static const char* g_button" << i << "Text = " << cppStringLiteral(button.text) << ";\n";
        output << "static const int g_button" << i << "Id = " << (1000 + static_cast<int>(i)) << ";\n";
    }

    for (std::size_t i = 0; i < program.lists.size(); ++i) {
        output << "static HWND g_list" << i << " = NULL;\n";
        output << "static HWND g_list" << i << "Title = NULL;\n";
        output << "static HFONT g_list" << i << "TitleFont = NULL;\n";
        output << "static const COLORREF g_list" << i << "FrameColor = " << colorToRgbCall(program.lists[i].frameColor) << ";\n";
        output << "static const COLORREF g_list" << i << "BgColor = " << colorToRgbCall(program.lists[i].backgroundColor) << ";\n";
        output << "static const COLORREF g_list" << i << "TitleColor = " << colorToRgbCall(program.lists[i].titleColor) << ";\n";
        output << "static HBRUSH g_list" << i << "BgBrush = NULL;\n";
    }

    for (std::size_t i = 0; i < program.texts.size(); ++i) {
        const TextSpec& text = program.texts[i];
        output << "static HWND g_text" << i << " = NULL;\n";
        output << "static HFONT g_text" << i << "Font = NULL;\n";
        output << "static const COLORREF g_text" << i << "Color = " << colorToRgbCall(text.textColor) << ";\n";
        output << "static const COLORREF g_text" << i << "BgColor = " << colorToRgbCall(text.backgroundColor) << ";\n";
        output << "static HBRUSH g_text" << i << "BgBrush = NULL;\n";
    }

    output << R"(static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_PAINT: {
            LRESULT result = DefWindowProcA(hwnd, message, wParam, lParam);
            HDC hdc = GetDC(hwnd);
)";

    for (std::size_t i = 0; i < program.lists.size(); ++i) {
        const ListSpec& list = program.lists[i];
        if (!list.frame) {
            continue;
        }
        int itemCount = 0;
        for (const ButtonSpec& button : program.buttons) {
            if (button.parent == list.name) {
                ++itemCount;
            }
        }
        const int titleHeight = list.title.empty() ? 0 : 34;
        const int contentWidth = std::max(list.itemWidth, 1);
        const int contentHeight = std::max(list.itemHeight * std::max(itemCount, 1), list.itemHeight) + titleHeight;
        const int frameWidth = contentWidth + list.padding * 2;
        const int frameHeight = contentHeight + list.padding * 2;
        output << "            HBRUSH list" << i << "Brush = CreateSolidBrush(g_list" << i << "BgColor);\n";
        output << "            RECT list" << i << "Rect = {" << list.x << ", " << list.y << ", " << (list.x + frameWidth) << ", " << (list.y + frameHeight) << "};\n";
        output << "            FillRect(hdc, &list" << i << "Rect, list" << i << "Brush);\n";
        output << "            DeleteObject(list" << i << "Brush);\n";
        output << "            HPEN list" << i << "Pen = CreatePen(PS_SOLID, 1, g_list" << i << "FrameColor);\n";
        output << "            HPEN list" << i << "OldPen = reinterpret_cast<HPEN>(SelectObject(hdc, list" << i << "Pen));\n";
        output << "            MoveToEx(hdc, " << list.x << ", " << list.y << ", NULL);\n";
        output << "            LineTo(hdc, " << (list.x + frameWidth) << ", " << list.y << ");\n";
        output << "            LineTo(hdc, " << (list.x + frameWidth) << ", " << (list.y + frameHeight) << ");\n";
        output << "            LineTo(hdc, " << list.x << ", " << (list.y + frameHeight) << ");\n";
        output << "            LineTo(hdc, " << list.x << ", " << list.y << ");\n";
        output << "            SelectObject(hdc, list" << i << "OldPen);\n";
        output << "            DeleteObject(list" << i << "Pen);\n";
    }

    output << R"(            ReleaseDC(hwnd, hdc);
            return result;
        }
        case WM_CTLCOLORSTATIC: {
            HDC controlDc = reinterpret_cast<HDC>(wParam);
            HWND control = reinterpret_cast<HWND>(lParam);
)";

    for (std::size_t i = 0; i < program.lists.size(); ++i) {
        output << "            if (control == g_list" << i << " || control == g_list" << i << "Title) {\n";
        output << "                SetBkColor(controlDc, g_list" << i << "BgColor);\n";
        output << "                SetTextColor(controlDc, g_list" << i << "TitleColor);\n";
        output << "                return reinterpret_cast<LRESULT>(g_list" << i << "BgBrush);\n";
        output << "            }\n";
    }
    for (std::size_t i = 0; i < program.texts.size(); ++i) {
        output << "            if (control == g_text" << i << ") {\n";
        output << "                SetBkColor(controlDc, g_text" << i << "BgColor);\n";
        output << "                SetTextColor(controlDc, g_text" << i << "Color);\n";
        output << "                return reinterpret_cast<LRESULT>(g_text" << i << "BgBrush);\n";
        output << "            }\n";
    }

    output << R"(            break;
        }
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
    for (std::size_t i = 0; i < program.lists.size(); ++i) {
        output << "            if (g_list" << i << "TitleFont) { DeleteObject(g_list" << i << "TitleFont); g_list" << i << "TitleFont = NULL; }\n";
        output << "            if (g_list" << i << "BgBrush) { DeleteObject(g_list" << i << "BgBrush); g_list" << i << "BgBrush = NULL; }\n";
    }
    for (std::size_t i = 0; i < program.texts.size(); ++i) {
        output << "            if (g_text" << i << "Font) { DeleteObject(g_text" << i << "Font); g_text" << i << "Font = NULL; }\n";
        output << "            if (g_text" << i << "BgBrush) { DeleteObject(g_text" << i << "BgBrush); g_text" << i << "BgBrush = NULL; }\n";
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
    output << "    auto LimdCToString = [](const auto& value) { std::ostringstream stream; stream << value; return stream.str(); };\n";
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

    for (std::size_t i = 0; i < program.lists.size(); ++i) {
        const ListSpec& list = program.lists[i];
        int itemCount = 0;
        for (const ButtonSpec& button : program.buttons) {
            if (button.parent == list.name) {
                ++itemCount;
            }
        }
        const int width = std::max(list.itemWidth, 1) + list.padding * 2;
        const int titleHeight = list.title.empty() ? 0 : 34;
        const int height = std::max(list.itemHeight * std::max(itemCount, 1), list.itemHeight) + titleHeight + list.padding * 2;
        output << "    g_list" << i << "BgBrush = CreateSolidBrush(g_list" << i << "BgColor);\n";
        output << "    g_list" << i << " = CreateWindowExA(0, \"STATIC\", \"\", WS_CHILD | WS_VISIBLE, "
               << list.x << ", " << list.y << ", " << width << ", " << height << ", " << list.parent
               << ", NULL, hInstance, NULL);\n";
        output << "    (void)g_list" << i << ";\n";
        if (!list.title.empty()) {
            output << "    g_list" << i << "TitleFont = CreateFontA(-18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, \"Microsoft YaHei UI\");\n";
            output << "    g_list" << i << "Title = CreateWindowExA(0, \"STATIC\", " << cppStringLiteral(list.title)
                   << ", WS_CHILD | WS_VISIBLE | SS_CENTER, " << (list.x + list.padding) << ", " << (list.y + list.padding) << ", " << std::max(list.itemWidth, 1)
                   << ", " << titleHeight << ", " << list.parent << ", NULL, hInstance, NULL);\n";
            output << "    SendMessageA(g_list" << i << "Title, WM_SETFONT, reinterpret_cast<WPARAM>(g_list" << i << "TitleFont), TRUE);\n";
        }
    }
    output << "\n";

    for (std::size_t i = 0; i < program.buttons.size(); ++i) {
        const ButtonSpec& button = program.buttons[i];
        const int fontSize = autoButtonFontSize(button);
        output << "    g_button" << i << "Font = CreateFontA(-" << fontSize
               << ", 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, "
               << "CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, \"Microsoft YaHei UI\");\n";
        int listOffsetX = 0;
        int listOffsetY = 0;
        for (const ListSpec& list : program.lists) {
            if (button.parent == list.name) {
                listOffsetX = list.x + list.padding;
                listOffsetY = list.y + list.padding + (list.title.empty() ? 0 : 34);
                break;
            }
        }
        const int drawX = button.x + listOffsetX;
        const int drawY = button.y + listOffsetY;
        output << "    g_button" << i << " = CreateWindowExA(0, \"BUTTON\", g_button" << i << "Text, "
               << "WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, " << drawX << ", " << drawY << ", "
               << button.width << ", " << button.height << ", " << parentExpression(program, button) << ", reinterpret_cast<HMENU>(g_button" << i
               << "Id), hInstance, NULL);\n";
    }

    for (std::size_t i = 0; i < program.texts.size(); ++i) {
        const TextSpec& text = program.texts[i];
        output << "    std::string g_text" << i << "Value = LimdCToString(" << text.textExpression << ");\n";
        output << "    g_text" << i << "BgBrush = CreateSolidBrush(g_text" << i << "BgColor);\n";
        output << "    g_text" << i << "Font = CreateFontA(-" << text.size
               << ", 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, "
               << "CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, \"Microsoft YaHei UI\");\n";
        const std::string textStyle = text.center ? "SS_CENTER" : "SS_LEFT | SS_LEFTNOWORDWRAP";
        output << "    g_text" << i << " = CreateWindowExA(0, \"STATIC\", g_text" << i << "Value.c_str(), WS_CHILD | WS_VISIBLE | " << textStyle << ", "
               << text.x << ", " << text.y << ", " << text.width << ", " << text.height << ", " << window.name
               << ", NULL, hInstance, NULL);\n";
        output << "    SendMessageA(g_text" << i << ", WM_SETFONT, reinterpret_cast<WPARAM>(g_text" << i << "Font), TRUE);\n";
    }

    output << "\n";
    output << "    ShowWindow(" << window.name << ", SW_SHOWDEFAULT);\n";
    output << "    UpdateWindow(" << window.name << ");\n";
    for (std::size_t i = 0; i < program.lists.size(); ++i) {
        if (program.lists[i].frame) {
            output << "    RedrawWindow(" << window.name << ", NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);\n";
            break;
        }
    }
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
            if (fs::exists(args.output)) {
                throw std::runtime_error(
                    "目标编译失败，可能是输出 exe 正在运行或被其他程序占用，请先关闭: " +
                    fs::absolute(args.output).string() +
                    "，命令: " + command
                );
            }
            throw std::runtime_error("目标编译失败，请确认已安装 gcc/g++，命令: " + command);
        }

        std::cout << "Output: " << fs::absolute(args.output).string() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "limdc: " << error.what() << '\n';
        return 1;
    }
}
