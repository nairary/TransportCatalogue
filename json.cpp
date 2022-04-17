#include "json.h"
#include <cmath>

using namespace std;

namespace json {

    namespace {

        Node LoadNode(istream& input);

        Node LoadArray(istream& input) {
            Array result;

            for (char c; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }

            return Node(move(result));
        }

        Node LoadString(istream& input) {
            std::string json_line;
            for (char c; input.get(c) && c != '\"';) {
                if (c == '\\') {
                    input.get(c);
                    if (!input.eof()) {
                        if (c == 'n') {
                            json_line += '\n';
                            continue;
                        }
                        else if (c == 'r') {
                            json_line += '\r';
                            continue;
                        }
                        else if (c == '\"') {
                            json_line += '\"';
                            continue;
                        }
                        else if (c == 't') {
                            json_line += '\t';
                            continue;
                        }
                        else if (c == '\\') {
                            json_line += '\\';
                            continue;
                        }
                    }
                }
                else {
                    json_line += c;
                }
            }
            if (input.eof())  throw ParsingError("Failed to load String");
            return Node(move(json_line));
        }

        Node LoadDict(istream& input) {
            Dict result;

            for (char c; input >> c && c != '}';) {
                if (c == ',') {
                    input >> c;
                }

                string key = LoadString(input).AsString();
                input >> c;
                result.insert({ move(key), LoadNode(input) });
            }

            return Node(move(result));
        }

        using Number = std::variant<int, double>;

        Node LoadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;

            // Считывает в parsed_num очередной символ из input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            // Считывает одну или более цифр в parsed_num из input
            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
            };

            if (input.peek() == '-') {
                read_char();
            }
            // Парсим целую часть числа
            if (input.peek() == '0') {
                read_char();
                // После 0 в JSON не могут идти другие цифры
            }
            else {
                read_digits();
            }

            bool is_int = true;
            // Парсим дробную часть числа
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            // Парсим экспоненциальную часть числа
            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    // Сначала пробуем преобразовать строку в int
                    try {
                        return std::stoi(parsed_num);
                    }
                    catch (...) {
                        // В случае неудачи, например, при переполнении,
                        // код ниже попробует преобразовать строку в double
                    }
                }
                return std::stod(parsed_num);
            }
            catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        Node LoadNode(istream& input) {
            char c;
            input >> c;

            if (c == '[') {
                input >> c;
                if (input.eof()) throw ParsingError("Failed to load Array");
                input.putback(c);
                return LoadArray(input);
            }
            else if (c == '{') {
                input >> c;
                if (input.eof()) throw ParsingError("Failed to load Dict");
                input.putback(c);
                return LoadDict(input);
            }
            else if (c == '"') {
                return LoadString(input);
            }
            else if (c == 't' || c == 'f' || c == 'n') {
                string str;
                char c;
                for (int i = 0; i < 3; ++i) {
                    input >> c;
                    str+=c;
                }
                if  (str == "rue")
                    return Node(true);
                if (str == "ull")
                    return Node(nullptr);
                input >> c;
                if (str + c == "alse")
                    return Node(false);
                else {
                    throw ParsingError("Failed to load Bool");
                }
            }
            else {
                input.putback(c);
                return LoadNumber(input);
            }
        }

    }  // namespace
    ///////////////////////////////////////

    bool Node::IsNull() const {
        if (std::holds_alternative<std::nullptr_t>(node_)) {
            return true;
        }
        if (IsString()) {
            if (AsString() == "null"s) return true;
        }
        return false;
    }
    bool Node::IsInt() const {
        if (std::holds_alternative<int>(node_)) {
            return true;
        }
        return false;
    }
    bool Node::IsDouble() const {
        if (std::holds_alternative<double>(node_) || IsInt()) {
            return true;
        }
        return false;
    }
    bool Node::IsPureDouble() const {
        if (std::holds_alternative<double>(node_)) {
            return true;
        }
        return false;
    }
    bool Node::IsString() const {
        if (std::holds_alternative<std::string>(node_)) {
            return true;
        }
        return false;
    }
    bool Node::IsBool() const {
        if (std::holds_alternative<bool>(node_)) {
            return true;
        }
        return false;
    }
    bool Node::IsArray() const {
        if (std::holds_alternative<Array>(node_)) {
            return true;
        }
        return false;
    }
    bool Node::IsMap() const {
        if (std::holds_alternative<Dict>(node_)) {
            return true;
        }
        return false;
    }
    const Array& Node::AsArray() const {
        static Array none;
        if (IsArray()) return std::get<Array>(node_);
        else throw std::logic_error("No matching variable type"s);
        return none;
    }
    bool Node::AsBool() const {
        if (IsBool()) return std::get<bool>(node_);
        else throw std::logic_error("No matching variable type"s);
        return 0;
    }
    double Node::AsDouble() const {
        if (IsPureDouble()) return std::get<double>(node_);
        else if (IsInt()) return double(std::get<int>(node_));
        else throw std::logic_error("No matching variable type"s);
        return 0.0;
    }
    int	Node::AsInt() const {
        if (IsInt()) return std::get<int>(node_);
        else throw std::logic_error("No matching variable type"s);
        return 0;
    }
    const Dict& Node::AsMap() const {
        static Dict none;
        if (IsMap()) return std::get<Dict>(node_);
        else throw std::logic_error("No matching variable type"s);
        return none;
    }
    const std::string& Node::AsString() const {
        static std::string none;
        if (IsString()) return std::get<std::string>(node_);
        else throw std::logic_error("No matching variable type"s);
        return none;
    }

    //////////////////////////////////////
    Document::Document(Node root)
            : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    Document Load(istream& input) {
        return Document{ LoadNode(input) };
    }

    void Print(const Document& doc, std::ostream& output) {
        auto root = doc.GetRoot();
        if (root.IsNull()) {
            output << "null"s;
            return;
        }
        if (root.IsInt()) {
            output << root.AsInt();
            return;
        }
        if (root.IsPureDouble()) {
            output << root.AsDouble();
            return;
        }
        if (root.IsString()) {
            std::string text = root.AsString();
            std::string json_line;
            for (size_t i = 0; i != text.size(); ++i) {
                char c = text[i];
                if (c == '\n') {
                    json_line += "\\n";
                    continue;
                }
                else if (c == '\r') {
                    json_line += "\\r";
                    continue;
                }
                else if (c == '\"') {
                    json_line += "\\\"";
                    continue;
                }
                else if (c == '\t') {
                    json_line += "\\t";
                    continue;
                }
                else if (c == '\\') {
                    json_line += "\\\\";
                    continue;
                }

                else {
                    json_line += c;
                }
            }
            output << "\""s << json_line << "\""s;
            return;
        }
        if (root.IsBool()) {
            if (root.AsBool()) {
                output << "true"s;
                return;
            }
            if (!root.AsBool()) {
                output << "false"s;
                return;
            }
        }
        if (root.IsArray()) {
            Array vector = root.AsArray();
            bool f = 0;
            output << '[';
            for (size_t j = 0; j < vector.size(); ++j) {
                if (f == 1) output << ", "s;
                Print(Document(vector[j]), output);
                f = 1;
            }
            output << ']';
            return;
        }
        if (root.IsMap()) {
            string word;
            output << '{' << endl;
            auto map = root.AsMap();
            bool f = 0;
            for (auto& [key, value] : map) {
                if (f == 1) output << ",\n"s;
                output << "\""s << key << "\""s << ": "s;
                Print(Document(value), output);
                f = 1;
            }
            output << '}' << endl;
            return;
        }
    }

}  // namespace json