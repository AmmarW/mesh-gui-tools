#include <iostream>
#include <vector>

int main() {
    // Declare a 10-element vector
    std::vector<int> numbers(10);

    // Take input from the user
    std::cout << "Enter 10 integers: ";
    for (int i = 0; i < 10; ++i) {
        std::cin >> numbers[i];
    }

    // Print the vector elements to the screen
    std::cout << "Vector elements: ";
    for (int num : numbers) {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    return 0;
}