#include <iostream>
#include <fstream>
#include <vector>
#include <sstream> // For string stream to parse CSV
#include <string>
#include <algorithm> 
#include <cctype>

// Checking if branch merge is working
using namespace std;

/**
* Reads a CSV file and stores its contents in a 2D vector.
* @param filename The name of the CSV file to read.
* @return A 2D vector containing the contents of the CSV file.
*/

vector<vector<string>> readCSV(const string &filename) {
    vector<vector<string>> data; // 2D vector to store CSV content
    ifstream file(filename);    // Input file stream to read the file 

    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        return data; // Return empty vector if file cannot be opened
    }

    string line;
    
    // Check if the file is empty
    if (!getline(file, line)) {  
        cerr << "Error: File is empty." << endl;
        return data;
    } else {
        //We did read the first file, so we can store it in the data vector, earlier once after the check it will discard the first line and not store its data
        vector<string> row;  
        stringstream ss(line);
        string cell;
        while (getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        data.push_back(row); 
    }

    // Read the file line by line from second line of the file
    while (getline(file, line)) {
        vector<string> row;  // Vector to store one row of the CSV
        stringstream ss(line); // String stream to split the line
        string cell;

        // Split the line by commas and store in the row vector
        while (getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        data.push_back(row); // Add the row to the 2D vector
    }
    file.close(); // Close the file
    return data;
}
/**
* Prints a 2D vector to the console.
* @param data The 2D vector to print.
*/

void print2DVector(const vector<vector<string>> &data) {
    for (const auto &row : data) {
        for (const auto &cell : row) {
            cout << cell << " "; // Print each cell separated by a space
        }
        cout << endl; // Move to the next line after each row
    }
}


/**
* Determines the data type of a given string value.
* @param value The string value to check.
* @return A string representing the data type ("int", "float", or "string").
*/
string determineDataType(const string &value) {
    if (value.empty()) return "string"; // Handle empty values safely
    // Check if the value consists only of digits
    if (all_of(value.begin(), value.end(), ::isdigit)) {
        return "int";
    }
    // Try to convert the value to a float
    try {
        float temp = stof(value);
        return "float";
    } catch (...) {
        // If conversion fails, return "string"
        return "string";
    }
}

vector<string> getColumnDataTypes(const vector<vector<string>> &data) {
    vector<string> dataTypes;

    // Ensure the dataset has at least two rows (header + at least one data row)
    if (data.size() < 2) {
        return dataTypes;  // Return empty if no valid data row exists
    }

    // Preallocate space for efficiency
    dataTypes.reserve(data[1].size());

    // Iterate through columns and determine data types
    for (const auto &cell : data[1]) {
        std::string type = determineDataType(cell);  // Explicitly store the return value
        dataTypes.push_back(type);  // Store it in the vector
    }

    return dataTypes;
}

//jasdeepbajaj
vector<int> countMissingValues(const vector<vector<string>> &data) {
    vector<int> missingCounts;

    if (data.empty()) return missingCounts;  // Return empty if no data

    int numColumns = data[0].size();
    missingCounts.resize(numColumns, 0);  // Initialize counts to 0 for each column

    // Iterate over each row in the data
    for (const auto &row : data) {
        for (int col = 0; col < numColumns; col++) {
            // If a cell is empty, increment the count for that column
            if (col < row.size() && row[col].empty()) {
                missingCounts[col]++;
            }
        }
    }

    return missingCounts;
}

int main() {
    // Prompt user for the CSV file name
    string filename;
    cout << "Enter the CSV file name: ";
    cin >> filename;
    
    // Ask if there is a header row
    char headerChoice;
    cout << "Does the file have a header row? (y/n): ";
    cin >> headerChoice;
    bool hasHeader = (headerChoice == 'y' || headerChoice == 'Y'); // Check if the user entered 'y' or 'Y' for yes
    // Read the CSV file into a 2D vector
    vector<vector<string>> csvData = readCSV(filename);
    // Check if data was successfully read
    if (csvData.empty()) {
        cout << "No data found or failed to read the file." << endl;
    } else {
        // Print header row (if user says there is one)
    if (hasHeader) {
        cout << "\nHeader row (printed with semicolons):" << endl;
        // The first row is csvData[0]
        for (size_t col = 0; col < csvData[0].size(); ++col) {
            cout << csvData[0][col];
            if (col < csvData[0].size() - 1) {
                cout << ";";
            }
        }
        cout << "\n" << endl;

        // Determine the data types for each column based on the second row (first data row)
        vector<string> dataTypes = getColumnDataTypes(csvData);
        
        // Print the data types for each column
        cout << "Data types per column:" << endl;
        for (size_t col = 0; col < dataTypes.size(); ++col) {
            cout << dataTypes[col];
            if (col < dataTypes.size() - 1) {
            cout << ";"; // Separate data types with semicolons
            }
        }
        cout << "\n" << endl;
        }

    
    // If there's a header, we skip the first row. Otherwise, we print them all.
    cout << "Data rows:" << endl;
    if (hasHeader && csvData.size() > 1) {
        // Create a "sub-vector" from row 1 to the end as first row is header
        vector<vector<string>> dataRows(csvData.begin() + 1, csvData.end());
        print2DVector(dataRows);
    } else if (!hasHeader) {
        // Since there is no header, we print the entire 2D vector
        print2DVector(csvData);
    } else {
        // Taking an edge case where there is no data rows, but only one header row is present as specified by the user
        cout << "(No data rows to print)\n";
    }

    }
    // Print the number of rows and columns in the CSV file
    if (hasHeader) { // If there's a header, we subtract 1 from the total number of rows to exclude the header
        // If there's only one row total, data rows = 0
        cout << "Number of data rows (excluding header): " 
             << (csvData.size() > 1 ? csvData.size() - 1 : 0) 
             << endl;
    } else {
        cout << "Number of data rows: " << csvData.size() << endl;
    }
    cout << "Number of columns: "  << csvData[0].size()  << endl;

    
    //jasdeepbajaj
    // Count and print the number of missing values per column
    vector<int> missingCounts = countMissingValues(csvData);
    cout << "\nMissing values per column:" << endl;
    for (int col = 0; col < missingCounts.size(); col++) {
        cout << "Column " << col + 1 << ": " << missingCounts[col] << " missing values" << endl;
    }

    return 0;

}
