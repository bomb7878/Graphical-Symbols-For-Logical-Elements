#include <iostream>
#include <windows.h>
#include <stack>
#include <vector>
#include <map>
#include <memory>
#include <locale>
#include <algorithm>
#include <cctype>
#include <string>

using namespace std;

// Типы токенов
enum MyTokenType {
    NULLID = 0,   // Для обозначения токена по умолчанию
    IDENTIFIER,   // Оператор (A, B, C...)
    OPERATOR,     // &, |, ^
    NOT,          // !
    LPAREN,       // (
    RPAREN,       // )
    END           // Конец выражения
};

// Структура токена
struct Token {
    MyTokenType type;
    string value;

    Token() : type(MyTokenType::NULLID), value("") {};
    Token(MyTokenType t, string v = "") : type(t), value(v) {}
};

// Класс лексического анализатора
class Lexer {
private:
    string expr;
    size_t pos;

public:
    Lexer(const string& expression) : expr(expression), pos(0) {}

    Token getNextToken() {
        while (pos < expr.length() && isspace(expr[pos])) {
            pos++;
        }

        if (pos >= expr.length()) {
            return Token(MyTokenType::END);
        }

        char current = expr[pos];

        // Проверяем операторы и скобки
        if (current == '&' || current == '|' || current == '^') {
            pos++;
            return Token(MyTokenType::OPERATOR, string(1, current));
        }
        else if (current == '!') {
            pos++;
            return Token(MyTokenType::NOT, "!");
        }
        else if (current == '(') {
            pos++;
            return Token(MyTokenType::LPAREN);
        }
        else if (current == ')') {
            pos++;
            return Token(MyTokenType::RPAREN);
        }
        // Идентификатор (последовательность букв и цифр)
        else if (isalpha(current) || current == '_') {
            string identifier;
            while (pos < expr.length() && (isalnum(expr[pos]) || expr[pos] == '_')) {
                identifier += expr[pos];
                pos++;
            }
            return Token(MyTokenType::IDENTIFIER, identifier);
        }

        // Если неизвестный символ, пропускаем
        pos++;
        return getNextToken();
    }
};

// Узел AST
struct ASTNode {
    string value;
    MyTokenType type;
    vector<shared_ptr<ASTNode>> children;
    bool hasNot = false; // Есть ли отрицание перед узлом

    ASTNode(MyTokenType t, string v = "") : type(t), value(v) {}
};

// Класс парсера
class Parser {
private:
    Lexer lexer;
    Token currentToken;

    void eat(MyTokenType type) {
        if (currentToken.type == type) {
            currentToken = lexer.getNextToken();
        }
    }

    // primary = IDENTIFIER | '(' expression ')'
    shared_ptr<ASTNode> primary() {
        Token token = currentToken;

        if (token.type == MyTokenType::IDENTIFIER) {
            eat(MyTokenType::IDENTIFIER);
            auto node = make_shared<ASTNode>(MyTokenType::IDENTIFIER, token.value);
            return node;
        }
        else if (token.type == MyTokenType::LPAREN) {
            eat(MyTokenType::LPAREN);
            auto node = expression();
            eat(MyTokenType::RPAREN);
            return node;
        }
        else if (token.type == MyTokenType::NOT) {
            eat(MyTokenType::NOT);
            auto node = primary();
            node->hasNot = true;
            return node;
        }

        return nullptr;
    }

    // factor = primary { ('&' primary)* }
    shared_ptr<ASTNode> factor() {
        auto node = primary();

        while (currentToken.type == MyTokenType::OPERATOR &&
            (currentToken.value == "&" || currentToken.value == "^")) {
            Token op = currentToken;
            eat(MyTokenType::OPERATOR);

            auto right = primary();
            auto newNode = make_shared<ASTNode>(MyTokenType::OPERATOR, op.value);
            newNode->children.push_back(node);
            newNode->children.push_back(right);
            node = newNode;
        }

        return node;
    }

    // term = factor { ('|' factor)* }
    shared_ptr<ASTNode> term() {
        auto node = factor();

        while (currentToken.type == MyTokenType::OPERATOR && currentToken.value == "|") {
            Token op = currentToken;
            eat(MyTokenType::OPERATOR);

            auto right = factor();
            auto newNode = make_shared<ASTNode>(MyTokenType::OPERATOR, op.value);
            newNode->children.push_back(node);
            newNode->children.push_back(right);
            node = newNode;
        }

        return node;
    }

public:
    Parser(const string& expr) : lexer(expr) {
        currentToken = lexer.getNextToken();
    }

    // expression = term
    shared_ptr<ASTNode> expression() {
        return term();
    }

    // Основной метод парсинга
    shared_ptr<ASTNode> parse() {
        return expression();
    }
};

// Класс для отображения УГО
class UGORenderer {
private:
    // Рекурсивная функция для отображения узла
    void renderNode(shared_ptr<ASTNode> node, int depth = 0) {
        if (!node) return;

        string indent(depth * 2, ' ');

        switch (node->type) {
        case MyTokenType::IDENTIFIER:
            cout << indent;
            if (node->hasNot) cout << "○";
            cout << node->value << endl;
            break;

        case MyTokenType::OPERATOR: {
            string opSymbol;
            if (node->value == "&") opSymbol = "AND";
            else if (node->value == "|") opSymbol = "OR";
            else if (node->value == "^") opSymbol = "XOR";
            else opSymbol = node->value;

            cout << indent << "+----------------+" << endl;
            cout << indent << "|      " << opSymbol << "      |" << endl;
            cout << indent << "+----------------+" << endl;

            // Отображаем входы
            for (size_t i = 0; i < node->children.size(); i++) {
                cout << indent << "   вход " << (i + 1) << ": ";
                if (node->children[i]->type == MyTokenType::IDENTIFIER) {
                    if (node->children[i]->hasNot) cout << "○";
                    cout << node->children[i]->value;
                }
                else {
                    cout << endl;
                    renderNode(node->children[i], depth + 3);
                }
                cout << endl;
            }
            break;
        }
        }
    }

public:
    // Отображение всего УГО
    void render(shared_ptr<ASTNode> ast) {
        cout << "Условно-графическое обозначение (УГО):" << endl;
        cout << "======================================" << endl;
        renderNode(ast);
        cout << "  выход: Y" << endl;
        cout << "======================================" << endl;
    }

    // Простая текстовая визуализация AST
    void printAST(shared_ptr<ASTNode> node, int depth = 0) {
        if (!node) return;

        string indent(depth * 2, ' ');
        cout << indent;

        switch (node->type) {
        case MyTokenType::IDENTIFIER:
            cout << "IDENTIFIER: " << node->value;
            if (node->hasNot) cout << " (NOT)";
            cout << endl;
            break;

        case MyTokenType::OPERATOR:
            cout << "OPERATOR: " << node->value << endl;
            for (const auto& child : node->children) {
                printAST(child, depth + 1);
            }
            break;
        }
    }
};

// Функция для извлечения логического выражения из строки
string extractExpression(const string& input) {
    size_t eqPos = input.find('=');
    if (eqPos == string::npos) return input;

    string expr = input.substr(eqPos + 1);

    expr.erase(0, expr.find_first_not_of(" \t"));
    expr.erase(expr.find_last_not_of(" \t") + 1);

    return expr;
}

int main() {
    locale::global(locale("ru_RU.UTF-8"));
    cout.imbue(locale());
    cin.imbue(locale());
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    cout << "=== Генератор УГО для логических элементов ===" << endl;
    cout << "Примеры выражений:" << endl;
    cout << "1. !(((A|B)&!C)&D&E)" << endl;
    cout << "2. ((A&B&(C&D))|((!E|F)&G))" << endl;
    cout << "3. A & B | C ^ D" << endl;
    cout << endl;

    // Пример использования
    vector<string> examples = {
        "Y = !(((A|B)&!C)&D&E)",
        "Y = ((A&B&(C&D))|((!E|F)&G))",
        "Y = A & B | C ^ D"
    };

    for (int i = 0; i < examples.size(); i++) {
        cout << "\nПример " << (i + 1) << ": " << examples[i] << endl;

        string expr = extractExpression(examples[i]);
        cout << "Выражение: " << expr << endl;

        Parser parser(expr);
        try {
            auto ast = parser.parse();

            cout << "\nAST структура:" << endl;
            UGORenderer renderer;
            renderer.printAST(ast);

            cout << "\nУГО представление:" << endl;
            renderer.render(ast);

        }
        catch (const exception& e) {
            cout << "Ошибка парсинга: " << e.what() << endl;
        }

        cout << "\n" << string(50, '-') << endl;
    }

    // Интерактивный режим
    cout << "\n=== Интерактивный режим ===" << endl;
    cout << "Введите логическое выражение (или 'exit' для выхода):" << endl;

    string userInput;
    while (true) {
        cout << "\n> ";
        getline(cin, userInput);

        if (userInput == "exit" || userInput == "выход") {
            break;
        }

        if (userInput.empty()) continue;

        string expr = extractExpression(userInput);
        cout << "Разбор выражения: " << expr << endl;

        Parser parser(expr);
        try {
            auto ast = parser.parse();

            UGORenderer renderer;
            cout << "\nAST структура:" << endl;
            renderer.printAST(ast);

            cout << "\nУГО представление:" << endl;
            renderer.render(ast);

        }
        catch (const exception& e) {
            cout << "Ошибка: " << e.what() << endl;
            cout << "Пожалуйста, проверьте синтаксис выражения." << endl;
        }
    }

    cout << "\nПрограмма завершена." << endl;
    return 0;
}