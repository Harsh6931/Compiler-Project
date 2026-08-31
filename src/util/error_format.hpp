#ifndef ERROR_FORMAT_HPP
#define ERROR_FORMAT_HPP

#include <sstream>
#include <string>

// Build a multi-line error with source line + caret under the column.
inline std::string formatCaretError(const std::string& source,
                                    int line,
                                    int column,
                                    const std::string& prefix,
                                    const std::string& message) {
    std::ostringstream out;
    out << prefix;
    if (line > 0) {
        out << " at line " << line;
        if (column > 0) {
            out << ":" << column;
        }
    }
    out << ": " << message << "\n";

    if (source.empty() || line <= 0) {
        return out.str();
    }

    int currentLine = 1;
    size_t lineStart = 0;
    for (size_t i = 0; i < source.size(); ++i) {
        if (currentLine == line) {
            lineStart = i;
            break;
        }
        if (source[i] == '\n') {
            currentLine++;
            lineStart = i + 1;
        }
    }

    if (currentLine != line) {
        return out.str();
    }

    size_t lineEnd = lineStart;
    while (lineEnd < source.size() && source[lineEnd] != '\n') {
        lineEnd++;
    }

    std::string text = source.substr(lineStart, lineEnd - lineStart);
    out << text << "\n";

    int caretCol = column > 0 ? column : 1;
    if (caretCol > static_cast<int>(text.size()) + 1) {
        caretCol = static_cast<int>(text.size()) + 1;
    }
    for (int i = 1; i < caretCol; ++i) {
        out << (i - 1 < static_cast<int>(text.size()) && text[i - 1] == '\t'
                    ? '\t'
                    : ' ');
    }
    out << "^";
    return out.str();
}

#endif
