// PEple.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "PEWarrior.h"

int main()
{
    char* filePath = new char[1024];
    cout << "Path:";
    cin >> filePath;
    cout << "Your input path: " << filePath << endl;
    PEWarrior peWarrior(filePath);
    //peWarrior.modifyEntryPoint(0x1180);
    //peWarrior.checkPE();
    //peWarrior.extendLastSection(0x50);
    //peWarrior.injectMessageBoxA32AtEnd(0x76cc0c30);
    peWarrior.addASection(0x600);
    delete[] filePath;
    return EXIT_SUCCESS;
}
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu
// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
