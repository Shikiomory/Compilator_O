#include "generateCode.h"
#include "ovm.h"
#include "../table/Item.h"

#include <limits>
#include <unordered_map>
#include <string>
#include <vector>

GenerateCode::GenerateCode() : ovm() {
    cmdCounter = 0;
} 

void GenerateCode::gen(int cmd)
{
    ovm.getMemory()[cmdCounter] = cmd;
    cmdCounter++;
}

void GenerateCode::genConst(int constValue)
{
    if (constValue >= 0)
    {
        gen(constValue);
    }
    else 
    {
        gen(ovm.NEG);
        gen(constValue);
    }
}

void GenerateCode::genVar(Item& item)
{
    genAddress(item);
    gen(ovm.LOAD);
}

void GenerateCode::genAddress(Item& item)
{
    gen(std::stoi(item.addr));
    item.addr = std::to_string(cmdCounter + 1); // Адрес+2 = cmdCounter+1. Чтобы fillGaps отработал, т.к. смотрит на 2 шага назад
}

void GenerateCode::genFunc(std::string func)
{
    if (func == "ABS")
    {
        gen(ovm.DUP);       // x, x
        gen(0);             // x, x, 0
        gen(cmdCounter+3);  // x, x, 0, Addr
        gen(ovm.IFGE);      // x
        gen(ovm.NEG);       // -x
    }
    else if (func == "MAX")
    {
        gen(std::numeric_limits<int>::max());
    }
    else if (func == "MIN")
    {
        gen(std::numeric_limits<int>::max());
        gen(ovm.NEG);
        gen(1);
        gen(ovm.SUB);
    }
    else if (func == "ODD")
    {
        gen(2);         // x, 2
        gen(ovm.MOD);   // x MOD 2
        gen(0);         // x MOD 2, 0
        gen(0);         // x MOD 2, 0 , Addr - временно 0
        gen(ovm.IFEQ);
    }
}

void GenerateCode::genOperation(std::string operation)
{
    if (operation == "DIV")
    {
        gen(ovm.DIV);
    }
    else if (operation == "MULT")
    {
        gen(ovm.MULT);
    }
    else if (operation == "MOD")
    {
        gen(ovm.MOD);
    }
}

void GenerateCode::genNegative()
{
    gen(ovm.NEG);
}

void GenerateCode::genAddition()
{
    gen(ovm.ADD);
}

void GenerateCode::genSubstraction()
{
    gen(ovm.SUB);
}

void GenerateCode::genSave()
{
    gen(ovm.SAVE);
}

void GenerateCode::genHalt(int exitCode)
{
    genConst(exitCode);
    gen(ovm.STOP);
}

void GenerateCode::genDup()
{
    gen(ovm.DUP);
}

void GenerateCode::genLoad()
{
    gen(ovm.LOAD);
}

void GenerateCode::genInInt()
{
    gen(ovm.IN);
    gen(ovm.SAVE);
}

void GenerateCode::genOutInt()
{
    gen(ovm.OUT);
}

void GenerateCode::genOutLn()
{
    gen(ovm.LN);
}

void GenerateCode::genAND() {
    gen(ovm.AND);
}
void GenerateCode::genOR() {
    gen(ovm.OR);
}
void GenerateCode::genNOT() {
    gen(ovm.NOT);
}

void GenerateCode::genComparison(std::string operation)
{
    // Зарезервировать место, куда запишем адрес перехода при выполнении условия
    int jumpIfTrue = getCmdCounter();
    gen(0);  // заглушка, заменим на адрес после GOTO

    // Инвертированная логика: если условие ЛОЖНО, переходим, иначе — кладём 1
    if (operation == "=") {
        gen(ovm.IFNE);  // если НЕ равно, перейти (значит, условие ЛОЖНО)
    }
    else if (operation == "#") {
        gen(ovm.IFEQ);  // если равно, перейти (значит, ЛОЖНО для #)
    }
    else if (operation == "<") {
        gen(ovm.IFGE);  // если >=, перейти (значит, ЛОЖНО для <)
    }
    else if (operation == "<=") {
        gen(ovm.IFGT);  // если >, перейти (значит, ЛОЖНО для <=)
    }
    else if (operation == ">") {
        gen(ovm.IFLE);  // если <=, перейти (значит, ЛОЖНО для >)
    }
    else if (operation == ">=") {
        gen(ovm.IFLT);  // если <, перейти (значит, ЛОЖНО для >=)
    }

    // Если условие ИСТИНА → кладём 1
    gen(1);

    // Прыжок через 1 к окончанию (перепрыгиваем 0)
    int jumpOverFalse = getCmdCounter();
    gen(0);          // заглушка
    gen(ovm.GOTO);   // GOTO конец

    // Если условие ЛОЖНО → приходим сюда и кладём 0
    ovm.getMemory()[jumpIfTrue] = getCmdCounter();
    gen(0);

    // Завершающий адрес для GOTO после 1
    ovm.getMemory()[jumpOverFalse] = getCmdCounter();
}



void GenerateCode::genGoTo(int code)
{
    gen(code);
    gen(ovm.GOTO);
}

void GenerateCode::fillGaps(int to)
{
    while (to > 0)
    {
        auto tmp = ovm.getMemory()[to-2];
        ovm.getMemory()[to-2] = cmdCounter;
        to = tmp;
    }
}

void GenerateCode::genSTOP()
{
    gen(ovm.STOP);
}

int GenerateCode::getCmdCounter()
{
    return cmdCounter;
}

void GenerateCode::runCode()
{
    ovm.run();
}

void GenerateCode::printCode()
{
    ovm.printCode(cmdCounter);
}