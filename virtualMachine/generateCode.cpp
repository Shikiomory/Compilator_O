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
    else if (operation == "*")
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

void GenerateCode::genComparison(std::string operation) {
    if (operation == "=") {
        gen(ovm.CMPEQ);
    }
    else if (operation == "#") {
        gen(ovm.CMPNE);
    }
    else if (operation == "<") {
        gen(ovm.CMPLT);
    }
    else if (operation == "<=") {
        gen(ovm.CMPLE);
    }
    else if (operation == ">") {
        gen(ovm.CMPGT);
    }
    else if (operation == ">=") {
        gen(ovm.CMPGE);
    }
}
 
void GenerateCode::genGoTo(int code)
{
    gen(code);
    gen(ovm.GOTO);
}

std::vector<int>& GenerateCode::getMemory() {
    return ovm.getMemory();
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

void GenerateCode::genLogicalAND() {
    gen(ovm.AND);
}

void GenerateCode::genLogicalOR() {
    gen(ovm.OR);
}

void GenerateCode::genLogicalNOT() {
    gen(ovm.NOT);
}

void GenerateCode::genIfZero() {
    gen(ovm.IFZ);
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