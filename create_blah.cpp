#include <fstream>
#include <iostream>

int main() {
    std::ofstream outFile("blah.txt");
    if (!outFile) {
        std::cerr << "Failed to create blah.txt" << std::endl;
        return 1;
    }
    outFile << "Hello, this is blah.txt" << std::endl;
    outFile.close();
    std::cout << "Created blah.txt" << std::endl;
    return 0;
}
