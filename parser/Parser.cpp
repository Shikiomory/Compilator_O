#include "Parser.h"
#include "../sourceCodeDriver/Driver.h"
#include "../scanner/Scanner.h"
#include "../error/Error.h"
#include "../table/TableOfName.h"
#include "../table/Item.h"
#include "../virtualMachine/generateCode.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <string>

Parser::Parser(Scanner &scanner, std::shared_ptr<Error> errorPtr) : 
    scanner(scanner), table(), errorPtr(std::move(errorPtr)), generateCode() {
    scanner.nextLex();
} 

void Parser::compile() 
{
    table.openScope(); // Блок стандартных идентификаторов
    table.addItem(table.functionItem("ABS", Item::ItemTypes::Integer));
    table.addItem(table.functionItem("MIN", Item::ItemTypes::Integer));
    table.addItem(table.functionItem("MAX", Item::ItemTypes::Integer));
    table.addItem(table.functionItem("ODD", Item::ItemTypes::Boolean));

    table.addItem(table.procedureItem("HALT"));
    table.addItem(table.procedureItem("INC"));
    table.addItem(table.procedureItem("DEC"));
    table.addItem(table.procedureItem("In.Open"));
    table.addItem(table.procedureItem("In.Int"));
    table.addItem(table.procedureItem("Out.Int"));
    table.addItem(table.procedureItem("Out.Ln"));

    table.addItem(table.typeItem("INTEGER", Item::ItemTypes::Integer));
    
    table.openScope(); // Блок модуля

    modulePrc();

    table.closeScope();
    table.closeScope();

    generateCode.printCode();
    generateCode.runCode();
}

// MODULE Имя ";"
// [Импорт]
// ПослОбъявл
// [BEGIN
//   ПослОператоров]
// END Имя "."
void Parser::modulePrc() 
{
    checkLex(Scanner::Lex::MODULE);
    // checkLex(Scanner::Lex::NAME);

    errorIfNotExpectedLex(Scanner::Lex::NAME);
    std::string moduleName = scanner.nameValue;
    table.newItem(table.moduleItem(moduleName));
    scanner.nextLex();
    
    checkLex(Scanner::Lex::SEMI);

    if (scanner.lex == Scanner::Lex::IMPORT)
    {
        // IMPORT опциональный
        // распознающая процедура для нетерминала IMPORT
        importPrc();
    }

    sequenceDeclarationsPrc();

    if (scanner.lex == Scanner::Lex::BEGIN) 
    {
        scanner.nextLex();
        sequenceStatementsPrc();
    }

    checkLex(Scanner::Lex::END);
    // checkLex(Scannerx::Lex::NAME);

    errorIfNotExpectedLex(Scanner::Lex::NAME);
    Item* item = table.findItem(scanner.nameValue);
    if (item->typeOfItem != "module")
    {
        errorPtr->syntaxError("имя модуля");
    }
    else if (item->name != moduleName)
    {
        errorPtr->syntaxError("имя модуля " + scanner.nameValue);
    }

    scanner.nextLex();
    checkLex(Scanner::Lex::DOT);
    generateCode.genSTOP();
    loccateVariables();
}

void Parser::loccateVariables()
{
    auto vars = table.getVars();
    generateCode.gen(generateCode.getCmdCounter());
    std::reverse(vars.begin(), vars.end());
    for (Item& var : vars)
    {
        if (std::stoi(var.addr) > 0) {
            generateCode.fillGaps(std::stoi(var.addr));
            generateCode.gen(0); // Увеличили cmdCounter на 1
        }
        else
        {
            std::cout << "Переменна `" << var.name << "` объявлена, но не используется" << std::endl;
        }
    }
}

// IMPORT Имя {"," Имя} ";".
void Parser::importPrc()
{
    checkLex(Scanner::Lex::IMPORT);
    contextImportPrc();
    // checkLex(Scanner::Lex::NAME);
    while (scanner.lex == Scanner::Lex::COMMA) {
        checkLex(Scanner::Lex::COMMA);
        contextImportPrc();
        // checkLex(Scanner::Lex::NAME);
    }
    checkLex(Scanner::Lex::SEMI);
}

void Parser::sequenceDeclarationsPrc() 
{
    while (scanner.lex == Scanner::Lex::CONST || scanner.lex == Scanner::Lex::VAR)
    {
        if (scanner.lex == Scanner::Lex::CONST) 
        {
            checkLex(Scanner::Lex::CONST);
            while (scanner.lex == Scanner::Lex::NAME) 
            {
                constDeclarationPrc();
                checkLex(Scanner::Lex::SEMI);
            }
        }
        else 
        {
            checkLex(Scanner::Lex::VAR);
            while (scanner.lex == Scanner::Lex::NAME) 
            {
                varDeclarationPrc();
                checkLex(Scanner::Lex::SEMI);
            }
        }
    }
}

void Parser::constDeclarationPrc()
{
    // checkLex(Scanner::Lex::NAME);
    errorIfNotExpectedLex(Scanner::Lex::NAME);
    std::string constName = scanner.nameValue;
    scanner.nextLex();

    checkLex(Scanner::Lex::EQ);
    int constValue = constExpresionPrc();

    table.newItem(table.constItem(constName, Item::ItemTypes::Integer, std::to_string(constValue)));
}

int Parser::constExpresionPrc() 
{
    int sign = 1;
    
    // не проверяем деление на 0, переполнение и тп
    if (scanner.lex == Scanner::Lex::MINUS)
    {
        checkLex(Scanner::Lex::MINUS);
        sign = -1;

    }
    else if (scanner.lex == Scanner::Lex::PLUS)
    {
        checkLex(Scanner::Lex::PLUS);
    }

    if (scanner.lex == Scanner::Lex::NAME)
    {
        Item* item = table.findItem(scanner.nameValue);
        checkLex(Scanner::Lex::NAME);
        if (item->typeOfItem != "const")
        {
            errorPtr->contextError("имя модуля");
        }
        else
        {
            return std::stoi(item->value) * sign;
        }
    }
    else if (scanner.lex == Scanner::Lex::NUM)
    {
        int constValue = scanner.numValue * sign;
        checkLex(Scanner::Lex::NUM);
        return constValue;
    }
    else
    {
        errorPtr->syntaxError("имя константы или число");
    }
}

void Parser::varDeclarationPrc() 
{
    contextVarPrc();
    // checkLex(Scanner::Lex::NAME);
    while (scanner.lex == Scanner::Lex::COMMA)
    {
        checkLex(Scanner::Lex::COMMA);
        contextVarPrc();
        // checkLex(Scanner::Lex::NAME);
    }

    checkLex(Scanner::Lex::COLON);
    typePrc();
}

void Parser::typePrc() 
{
    errorIfNotExpectedLex(Scanner::Lex::NAME);
    Item* item = table.findItem(scanner.nameValue);

    if (item->typeOfItem != "type")
    {
        errorPtr->contextError("Необьявленное имя типа");
    }

    scanner.nextLex();
}

void Parser::sequenceStatementsPrc() 
{
    statementsPrc();
    while (scanner.lex == Scanner::Lex::SEMI)
    {
        scanner.nextLex();
        statementsPrc();
    }
}

// [
//   Переменная ":=" Выраж
//   | [Имя "."] Имя ["(" Параметр {"," Параметр}] ")"]
//   | IF Выраж THEN
//     ПослОператоров
//   {ELSIF Выраж THEN
//     ПослОператоров}
//   [ELSE
//     ПослОператоров]
//    END
//   | WHILE Выраж DO
//     ПослОператоров
//   END
// ]
void Parser::statementsPrc()
{
    if (scanner.lex == Scanner::Lex::NAME)
    {
        variableOrCallPrc();
    } 
    else if (scanner.lex == Scanner::Lex::IF)
    {
        ifStatementPrc();
    }
    else if (scanner.lex == Scanner::Lex::WHILE)
    {
        whileStatementPrc();
    }
}

//   Переменная ":=" Выраж
//   | [Имя "."] Имя ["(" Параметр {"," Параметр}] ")"]
void Parser::variableOrCallPrc()
{
    errorIfNotExpectedLex(Scanner::Lex::NAME);
    Item* item = table.findItem(scanner.nameValue);
    scanner.nextLex();

    if (item->typeOfItem == "var")
    {
        generateCode.genAddress(*item);
        checkLex(Scanner::Lex::ASS);
        Item::ItemTypes expressionType = expressionPrc();
        if (item->type != expressionType) 
        {
            errorPtr->contextError("Неверный тип при присваивании");
        }
        generateCode.genSave();
    }
    else if (item->typeOfItem == "procedure" || item->typeOfItem == "module")
    {
        if (scanner.lex == Scanner::Lex::DOT)
        {
            if (item->typeOfItem != "module")
            {
                errorPtr->contextError("Ожидается имя модуля");
            }
            scanner.nextLex();
            // checkLex(Scanner::Lex::NAME);
            errorIfNotExpectedLex(Scanner::Lex::NAME);
            std::string procedureName = item->name + "." + scanner.nameValue;
            item = table.findItem(procedureName);
            if (item->typeOfItem != "procedure")
            {
                errorPtr->contextError("Ожидается процедура");
            }
            scanner.nextLex();
        }
        else if (item->typeOfItem != "procedure")
        {
            errorPtr->contextError("Ожидается имя процедуры");
        }

        if (scanner.lex == Scanner::Lex::LPAR)
        {
            scanner.nextLex();
            checkProcParameters(*item);
            checkLex(Scanner::Lex::RPAR);  
        }
        else if (item->name == "Out.Ln")
        {
            generateCode.genOutLn();
        }
        else if (item->name != "Out.Ln" && item->name != "In.Open")
        {
            errorPtr->contextError("Ожидается скобка");
        }
        
    }
    else
    {
        errorPtr->contextError("Ожидается имя перменной или процедуры");
    }
}

//   IF Выраж THEN
//     ПослОператоров
//   {ELSIF Выраж THEN
//     ПослОператоров}
//   [ELSE
//     ПослОператоров]
//    END
// Updated Parser::ifStatementPrc to correctly handle nested ELSIF and nested IFs
void Parser::ifStatementPrc()
{
    // Stack to hold positions of unconditional exits (GOTO) for all branches
    std::vector<int> exitJumpPositions;

    // 1) Parse 'IF' and the boolean expression
    checkLex(Scanner::Lex::IF);
    Item::ItemTypes expressionType = expressionPrc();
    checkBoolType(expressionType);

    // 2) Prepare placeholder for false jump of first IF
    int falseJumpPos = generateCode.getCmdCounter();
    generateCode.gen(0);            // Placeholder address (override later)
    generateCode.gen(OVM::IFZ);

    // 3) Parse 'THEN' and its statements
    checkLex(Scanner::Lex::THEN);
    sequenceStatementsPrc();

    // 4) After executing the 'THEN' branch, unconditionally jump to end of IF
    int exitPos = generateCode.getCmdCounter();
    generateCode.gen(0);
    generateCode.gen(OVM::GOTO);
    exitJumpPositions.push_back(exitPos);

    // 5) Set the false-jump placeholder of the IF to the next instruction
    generateCode.getMemory()[falseJumpPos] = generateCode.getCmdCounter();

    // 6) Handle all 'ELSIF' clauses
    while (scanner.lex == Scanner::Lex::ELSIF)
    {
        // 6.1) Parse 'ELSIF' and its condition
        checkLex(Scanner::Lex::ELSIF);
        expressionType = expressionPrc();
        checkBoolType(expressionType);

        // 6.2) Placeholder for false jump of this ELSIF
        falseJumpPos = generateCode.getCmdCounter();
        generateCode.gen(0);
        generateCode.gen(OVM::IFZ);

        // 6.3) Parse 'THEN' and its statements
        checkLex(Scanner::Lex::THEN);
        sequenceStatementsPrc();

        // 6.4) Unconditionally jump to end of IF after this branch
        exitPos = generateCode.getCmdCounter();
        generateCode.gen(0);
        generateCode.gen(OVM::GOTO);
        exitJumpPositions.push_back(exitPos);

        // 6.5) Set previous false jump to current instruction
        generateCode.getMemory()[falseJumpPos] = generateCode.getCmdCounter();
    }

    // 7) Handle optional 'ELSE' clause
    if (scanner.lex == Scanner::Lex::ELSE)
    {
        // 7.1) Parse 'ELSE' and its statements
        checkLex(Scanner::Lex::ELSE);
        sequenceStatementsPrc();
    }

    // 8) Expect 'END' and finish IF
    checkLex(Scanner::Lex::END);

    // 9) Patch all recorded exit jumps to point here
    for (int pos : exitJumpPositions)
    {
        generateCode.getMemory()[pos] = generateCode.getCmdCounter();
    }
}



//   WHILE Выраж DO
//     ПослОператоров
//   END
void Parser::whileStatementPrc()
{
    int whileStart = generateCode.getCmdCounter(); // Начало цикла

    checkLex(Scanner::Lex::WHILE);
    Item::ItemTypes expressionType = expressionPrc();
    checkBoolType(expressionType);

    // Условный переход: если условие false, выйти из цикла
    int jumpOutAddr = generateCode.getCmdCounter();
    generateCode.gen(0);            // Временный адрес выхода
    generateCode.gen(OVM::IFZ);     // Переход если 0 (false)

    checkLex(Scanner::Lex::DO);
    sequenceStatementsPrc();
    checkLex(Scanner::Lex::END);

    generateCode.genGoTo(whileStart); // Переход в начало

    // Подставляем адрес после тела, куда прыгать, если условие ложно
    int afterLoop = generateCode.getCmdCounter();
    generateCode.getMemory()[jumpOutAddr] = afterLoop;
}


// Параметр | Выражение
// Пока нет контекстного анализатора
void Parser::parameterPrc()
{
    expressionPrc();
}

// ПростоеВыраж [Отношение ПростоеВыраж]
Item::ItemTypes Parser::expressionPrc() {
    return logicalExpression();
}

Item::ItemTypes Parser::logicalExpression() {
    Item::ItemTypes type = logicalTerm();
    while (scanner.lex == Scanner::Lex::OR) {
        scanner.nextLex();
        Item::ItemTypes type2 = logicalTerm();
        checkBoolType(type);
        checkBoolType(type2);
        generateCode.genLogicalOR();
        type = Item::ItemTypes::Boolean;
    }
    return type;
}

Item::ItemTypes Parser::logicalTerm() {
    Item::ItemTypes type = logicalFactor();
    while (scanner.lex == Scanner::Lex::AND) {
        scanner.nextLex();
        Item::ItemTypes type2 = logicalFactor();
        checkBoolType(type);
        checkBoolType(type2);
        generateCode.genLogicalAND();
        type = Item::ItemTypes::Boolean;
    }
    return type;
}

Item::ItemTypes Parser::logicalFactor() {
    if (scanner.lex == Scanner::Lex::NOT) {
        scanner.nextLex();
        Item::ItemTypes type = logicalFactor();
        checkBoolType(type);
        generateCode.genLogicalNOT();
        return Item::ItemTypes::Boolean;
    }
    return comparison();
}

Item::ItemTypes Parser::comparison() {
    Item::ItemTypes type = simpleExpressionPrc();

    if (scanner.lex == Scanner::Lex::EQ ||
        scanner.lex == Scanner::Lex::NE ||
        scanner.lex == Scanner::Lex::LT ||
        scanner.lex == Scanner::Lex::LE ||
        scanner.lex == Scanner::Lex::GT ||
        scanner.lex == Scanner::Lex::GE) {

        Scanner::Lex op = scanner.lex;
        std::string opStr = scanner.getStringNameOfLex(op);  // Сохраняем строку ДО nextLex()
        scanner.nextLex();

        Item::ItemTypes type2 = simpleExpressionPrc();
        checkIntType(type);
        checkIntType(type2);

        generateCode.genComparison(opStr);  // Используем сохранённую строку
        type = Item::ItemTypes::Boolean;
    }

    return type;
}

// ["+"|"-"] Слагаемое {ОперСлож Слагаемое}.
Item::ItemTypes Parser::simpleExpressionPrc()
{
    Item::ItemTypes termType;
    if (scanner.lex == Scanner::Lex::PLUS ||
        scanner.lex == Scanner::Lex::MINUS)
    {
        auto operation = scanner.lex;
        scanner.nextLex();
        termType = termPrc();
        checkIntType(termType);
        if (operation == Scanner::Lex::MINUS)
        {
            generateCode.genNegative();
        }
        
    }
    else
    {
        termType = termPrc();
        if (scanner.lex == Scanner::Lex::PLUS ||
            scanner.lex == Scanner::Lex::MINUS)
        {
            checkIntType(termType);
        }
    }

    while (scanner.lex == Scanner::Lex::PLUS ||
            scanner.lex == Scanner::Lex::MINUS)
    {
        auto operation = scanner.lex;
        scanner.nextLex();
        termType = termPrc();
        checkIntType(termType);
        if (operation == Scanner::Lex::PLUS)
        {
            generateCode.genAddition();
        }
        else
        {
            generateCode.genSubstraction();
        }
    }

    return termType;
}

// Множитель {ОперацияУмножения Множитель}
Item::ItemTypes Parser::termPrc()
{
    Item::ItemTypes multiplierType;
    multiplierType = multiplierPrc();

    if (scanner.lex == Scanner::Lex::DIV ||
            scanner.lex == Scanner::Lex::MOD ||
            scanner.lex == Scanner::Lex::MULT)
    {
        checkIntType(multiplierType);
    }

    while (scanner.lex == Scanner::Lex::DIV ||
            scanner.lex == Scanner::Lex::MOD ||
            scanner.lex == Scanner::Lex::MULT)
    {
        auto operation = scanner.lex;
        scanner.nextLex();
        multiplierType = multiplierPrc();
        checkIntType(multiplierType);
        generateCode.genOperation(scanner.getStringNameOfLex(operation));
    }

    return multiplierType;
}

// Имя ["(" Выраж | Тип ")"]
// | Число
// | "(" Выраж ")".
Item::ItemTypes Parser::multiplierPrc()
{
    if (scanner.lex == Scanner::Lex::NAME)
    {
        Item* item = table.findItem(scanner.nameValue);
        if (item->typeOfItem == "const")
        {
            generateCode.genConst(std::stoi(item->value));
            scanner.nextLex();
            return item->type;
        }
        else if (item->typeOfItem == "var")
        {
            generateCode.genVar(*item);
            scanner.nextLex();
            return item->type;
        }
        else if (item->typeOfItem == "function")
        {
            scanner.nextLex();
            checkLex(Scanner::Lex::LPAR);
            checkFuncParameters(*item);
            checkLex(Scanner::Lex::RPAR);
            return item->type;
        }
        else
        {
            errorPtr->contextError("Ожидается константа, имя или функция");
        }
    }
    else if (scanner.lex == Scanner::Lex::NUM)
    {
        generateCode.gen(scanner.numValue);
        scanner.nextLex();
        return Item::ItemTypes::Integer;
    }
    else if (scanner.lex == Scanner::Lex::LPAR)
    {
        scanner.nextLex();
        Item::ItemTypes expressionType = expressionPrc();
        checkLex(Scanner::Lex::RPAR);
        return expressionType;
    }
    else
    {
        errorPtr->syntaxError("Имя, число или '('");
    }

    // заглушка !!!!
    return Item::ItemTypes::Integer;
}

void Parser::contextImportPrc() 
{
    errorIfNotExpectedLex(Scanner::Lex::NAME);
    if (scanner.nameValue == "In" || scanner.nameValue == "Out")
    {
        table.newItem(table.moduleItem(scanner.nameValue));
    }
    else
    {
        errorPtr->contextError("Ожидается модуль `In` или `Out`");
    }

    scanner.nextLex();
}

void Parser::contextVarPrc()
{
    errorIfNotExpectedLex(Scanner::Lex::NAME);
    table.newItem(table.varItem(scanner.nameValue, Item::ItemTypes::Integer, "0"));
    scanner.nextLex();
}

void Parser::checkProcParameters(Item item) 
{
    if (item.name == "HALT")
    {
        int value = constExpresionPrc();
        generateCode.genHalt(value);
    }
    else if (item.name == "INC")
    {
        errorIsNotVariable();
        generateCode.genDup();
        generateCode.genLoad();
        if (scanner.lex == Scanner::Lex::COMMA)
        {
            scanner.nextLex();
            Item::ItemTypes expressionType = expressionPrc();
            checkIntType(expressionType);
        }
        else
        {
            generateCode.gen(1);
        }
        generateCode.genAddition();
        generateCode.genSave();
    }
    else if (item.name == "DEC")
    {  
        errorIsNotVariable();
        generateCode.genDup();
        generateCode.genLoad();
        if (scanner.lex == Scanner::Lex::COMMA)
        {
            scanner.nextLex();
            Item::ItemTypes expressionType = expressionPrc();
            checkIntType(expressionType);
        }
        else
        {
            generateCode.gen(1);
        }
        generateCode.genSubstraction();
        generateCode.genSave();
    }
    else if (item.name == "In.Open")
    {
        // skip
    }
    else if (item.name == "In.Int")
    {
        errorIsNotVariable();
        generateCode.genInInt();
    }
    else if (item.name == "Out.Int")
    {
        Item::ItemTypes expressionType = expressionPrc();
        checkIntType(expressionType);
        checkLex(Scanner::Lex::COMMA);
        expressionType = expressionPrc();
        checkIntType(expressionType);
        generateCode.genOutInt();
    }
    else if (item.name == "Out.Ln")
    {
        generateCode.genOutLn();
    }
    else 
    {
        errorPtr->contextError("Неизвестная процедура");
    }
}

void Parser::checkFuncParameters(Item item)
{
    if (item.name == "ABS")
    {
        Item::ItemTypes expressionType = expressionPrc();
        checkIntType(expressionType);
        generateCode.genFunc("ABS");
    }
    else if (item.name == "MIN")
    {
        typePrc();
        generateCode.genFunc("MIN");
    }
    else if (item.name == "MAX")
    {
        typePrc();
        generateCode.genFunc("MAX");
    }
    else if (item.name == "ODD")
    {
        Item::ItemTypes expressionType = expressionPrc();
        checkIntType(expressionType);
        generateCode.genFunc("ODD");
    }
    else
    {
        errorPtr->contextError("Неизвестная функция");
    }
}

void Parser::checkLex(Scanner::Lex lex)
{
    if (Scanner::lex == lex)
    {
        scanner.nextLex();
    }
    else 
    {
        std::string expected = scanner.getStringNameOfLex(lex);
        errorPtr->syntaxError(expected);
    }
}

void Parser::errorIfNotExpectedLex(Scanner::Lex lex)
{
    if (Scanner::lex != lex)
    {
        std::string expected = scanner.getStringNameOfLex(lex);
        errorPtr->syntaxError(expected);
    }
}

void Parser::checkIntType(Item::ItemTypes type)
{
    if (type != Item::ItemTypes::Integer)
    {
        errorPtr->contextError("Ожидается целый тип");
    }
}

void Parser::checkBoolType(Item::ItemTypes type)
{
    if (type != Item::ItemTypes::Boolean)
    {
        errorPtr->contextError("Ожидается логический тип");
    }
}

void Parser::errorIsNotVariable() {
    errorIfNotExpectedLex(Scanner::Lex::NAME);
    Item* paramItem = table.findItem(scanner.nameValue);

    if (paramItem->typeOfItem != "var") 
    {
        errorPtr->contextError("Ожидается имя переменной");
    }

    generateCode.genAddress(*paramItem);

    scanner.nextLex();
}