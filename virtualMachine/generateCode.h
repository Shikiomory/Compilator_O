#include "ovm.h"
#include "../table/Item.h"

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>

#pragma once

class GenerateCode
{
public:
    GenerateCode();

    void gen(int cmd);
    void genConst(int constValue);
    void genVar(Item& item);
    void genFunc(std::string func);
    void genOperation(std::string operation);
    void genNegative();
    void genAddition();
    void genSubstraction();
    void genComparison(std::string operation);
    void genHalt(int exitCode);
    void genInInt();
    void genOutInt();
    void genOutLn();
    void genGoTo(int code);
    std::vector<int>& getMemory();
    void fillGaps(int to);

    void genDup();
    void genLoad();
    void genSTOP();

    void genAddress(Item& item);
    void genSave();

    void genLogicalAND();
    void genLogicalOR();
    void genLogicalNOT();
    void genIfZero();

    int getCmdCounter();

    void runCode();
    void printCode();


private:
    int cmdCounter; 
    OVM ovm;
};
