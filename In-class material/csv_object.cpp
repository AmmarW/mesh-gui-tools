#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <fstream>

class CSVObject {
private:
    std::vector<std::vector<std::string>> data;

public:
    CSVObject() {
        // Hard-coded arrays
        std::vector<std::string> array1 = {"A1", "B1", "C1"};
        std::vector<std::string> array2 = {"A2", "B2", "C2", "D2"};
        std::vector<std::string> array3 = {"A3", "B3"};
        std::vector<std::string> array4 = {"A4", "B4", "C4", "D4", "E4"};
        std::vector<std::string> array5 = {"A5", "B5", "C5"};

        // Combine arrays into data as columns
        size_t maxLength = std::max({array1.size(), array2.size(), array3.size(), array4.size(), array5.size()});
        data.resize(maxLength, std::vector<std::string>(5, ""));

        for (size_t i = 0; i < array1.size(); ++i) data[i][0] = array1[i];
        for (size_t i = 0; i < array2.size(); ++i) data[i][1] = array2[i];
        for (size_t i = 0; i < array3.size(); ++i) data[i][2] = array3[i];
        for (size_t i = 0; i < array4.size(); ++i) data[i][3] = array4[i];
        for (size_t i = 0; i < array5.size(); ++i) data[i][4] = array5[i];
    }

    std::string toCSV() const {
        std::ostringstream oss;
        for (const auto& row : data) {
            for (size_t i = 0; i < row.size(); ++i) {
                oss << row[i];
                if (i < row.size() - 1) {
                    oss << ",";
                }
            }
            oss << "\n";
        }
        return oss.str();
    }

    void saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (file.is_open()) {
            file << toCSV();
            file.close();
        } else {
            std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
        }
    }
};


int main() {
    CSVObject csv;
    std::cout << csv.toCSV();
    csv.saveToFile("output.csv");
    return 0;
}