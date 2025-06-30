#include <fstream>
#include <iostream>

using namespace std;

int main() {
    ofstream file("./logic.txt");

    file << "File just created";
    file.close();
    return 0;
}
