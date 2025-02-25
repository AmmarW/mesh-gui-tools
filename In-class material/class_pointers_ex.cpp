#include <iostream>
using namespace std;

int main() {

    //declare variables
    int var1 = 3;
    int var2 = 4;
    int var3 = 5;
    //declare pointers
    int* pointvar1 = &var1;
    int* pointvar2 = &var2;
    int* pointvar3 = &var3;
    //print the address of the variables
    cout << "Address of var1: " << pointvar1 << endl;
    cout << "Address of var2: " << pointvar2 << endl;
    cout << "Address of var3: " << pointvar3 << endl;
    //print content of the variables
    cout << "Content of var1: " << *pointvar1 << endl;
    cout << "Content of var2: " << *pointvar2 << endl;
    cout << "Content of var3: " << *pointvar3 << endl;
    //delete the pointers
    delete pointvar1;
    delete pointvar2;
    delete pointvar3;
}