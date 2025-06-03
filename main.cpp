#include "iostream"
#include "sourceCodeDriver/Driver.h"
#include "parser/Parser.h"
#include "error/Error.h"
#include "scanner/Scanner.h"
#include "virtualMachine/ovm.h"
#include "virtualMachine/generateCode.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <locale>
#include <memory>

int main(int argc, char const* argv[])
{
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "���������� ����� O" << std::endl;
    std::shared_ptr<Error> errorPtr = std::make_shared<Error>();
    Driver driver(errorPtr);

    if (argc < 2)
    {
        errorPtr->error("� �������� ��������� ������ ���� ������ ���� *.o");
    }

    driver.resetText(argv[1]);

    Scanner scanner(driver, errorPtr);
    Parser parser(scanner, errorPtr);
    parser.compile();

    std::cout << "Compiled";

    //// OVM ovm;
    //// ovm.printCode();
    //// ovm.test_input();
    //// ovm.run();



    return 0;
}
