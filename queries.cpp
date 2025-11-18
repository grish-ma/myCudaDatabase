%%writefile queries.cpp
// Queries to write:

// Version 1 -- 
// CREATE TABLE table_name (~~~~)
// SELECT (DISTINCT) col1, col2, ... // or use * for all 
// FROM table
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <numeric>
#include <chrono>

// Using Google Colab to run CUDA
// #include "cuda_runtime.h"
// #include "device_launch_parameters.h"
using namespace std;

// Table class
class Table {
    private:
        int numRows; int numCols;
        string name;
        vector<vector<string>> tableRows;
        vector<string> colNames;

    public:
        // constructor --> numRows = 0; colNames
        Table(vector<string> columns) : name("No Name"), numRows(0), numCols(columns.size()), colNames(columns)
        {
            // tableRows.push_back(columns); // Adding the first row as column name
        }

        Table(vector<string> columns, string nameIn) : name(nameIn), numRows(0), numCols(columns.size()), colNames(columns)
        {
            // tableRows.push_back(columns); // Adding the first row as column name
        }

        Table(Table& toCopy) : name("No Name"), numRows(toCopy.numRows), numCols(toCopy.numCols), colNames(toCopy.colNames)
        {
          
        }

        // Destructor - vectors will automatically clean up
        ~Table() = default;

        // Converting 2D table to 1D array for GPU
        vector<double> flatten() const {
            vector<double> flat;
            
            // Go through every single value
            for (const auto& row : tableRows) {
                for (const auto& value : row)
                {
                    flat.push_back(stod(value));
                }
                // flat.insert(flat.end(), row.begin(), row.end());
            }
            return flat;
        }

        // TODO: Claude generated --  Convert 1D array back to Table
        static Table* fromFlattened(vector<double>& flat, vector<string> cols, int rows) {
            Table* t = new Table(cols);
            int num_cols = cols.size();
            for (int i = 0; i < rows; i++) {
                vector<string> row;
                for (int j = 0; j < num_cols; j++) {
                    row.push_back(to_string(flat[i * num_cols + j]));
                }
                t->addRow(row);
            }
            return t;
        }


        string getName()
        {
            return name;
        }

        void addRow(vector<string> row)
        {
            tableRows.push_back(row);
            numRows++;
        }

        void setColNames(vector<string> row)
        {
            colNames = row;
            numCols = colNames.size();
        }

        int getNumRows()
        { return numRows; }

        int getNumCols()
        { return numCols; }

        int getColumnIndex(string column)
        {
            // Finding column index
            for (int c = 0; c < numCols; c++)
            {
                if(colNames[c] == column)
                {
                    return c;
                }
            }
            cerr << "No column named " << column << " exists " << endl;
            return -1;
        }

        // Return const reference to avoid expensive copying
        const vector<string>& getAllCols() const
        {
            return colNames;
        }

        vector<string> getRow(int index)
        {
            if (numRows == 0)
                cout << endl << endl << "numRows = 0"  << endl << endl;
            //     return nullptr; // 
            return tableRows[index];
        }

        // Return const reference to avoid expensive copying
        const vector<vector<string>>& getAllRows() const
        {
            if (numRows == 0)
                cout << endl << endl << "numRows = 0"  << endl << endl;
            return tableRows;
        }

        string getValue(int row, int col)
        {
            if (numRows == 0 || numCols == 0)
                cout << endl << endl << "numRows or numCols = 0"  << endl << endl;
            return tableRows[row][col];
        }

        void editTableValue(int row, int col, string value)
        {
            cout << "Edited value" << endl << endl;
            tableRows[row][col] = value;
        }

        void printTable()
        {
            cout << "numRows = " << numRows;
            cout << "\tnumCols = " << numCols << endl;;
            for (string s : colNames)
            {
                cout << s << "\t";
            }
            cout << endl;
            for (int i = 0; i < numRows; i++)
            {
                for (int j = 0; j < numCols; j++)
                {
                    string value = tableRows[i][j];
                    cout << value << "\t";
                }
                cout << endl;
            }
        }

        void appendStringtoColumns(string append)
        {
            // TODO: do we have to do it this way? is there a more efficient way?
            vector<string> newCols;
            for (const auto& cols : colNames)
                newCols.push_back(append + "_" + cols);

            colNames = newCols;
        }
};

enum AggType {
    COUNT,    // implicitly assigned 0
    AVG,  // implicitly assigned 1
    SUM    // implicitly assigned 2
};


// Claude-generated: TODO
static vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    
    while (getline(tokenStream, token, delimiter)) {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);
        tokens.push_back(token);
    }
    
    return tokens;
}


// Load ALL columns
Table* loadCSV(string filepath, int maxRows)
{
    ifstream inputFile(filepath);
    // Check if the CSV opened successfully
    if (!inputFile.is_open()) {
        cerr << "Error opening file!" << endl;
        return nullptr; // Or handle the error appropriately
    }
    string columnString;
    getline(inputFile, columnString);
    // Create table with ALL columns in the CSV
    vector<string> columns = split(columnString, ',');
    Table* newTable = new Table(columns);

    string line; 
    while (maxRows > 0 && getline(inputFile, line)) {
        // Process each line here
        // cout << "line: " << line << endl;
        // Delimter = ','
        vector<string> rowValues = split(line, ',');
        newTable->addRow(rowValues);
        maxRows--;
    }
    // newTable->printTable();

    inputFile.close();
    return newTable;
}

// Load chosen columns
Table* loadCSV(string filepath, int maxRows, vector<string> columnsToLoad)
{
    ifstream inputFile(filepath);
    // Check if the CSV opened successfully
    if (!inputFile.is_open()) {
        cerr << "Error opening file!" << endl;
        return nullptr; // Or handle the error appropriately
    }

    string columnString;
    getline(inputFile, columnString);
    vector<string> csvColumns = split(columnString, ',');
    vector<int> column_indices;

    // Find indices of columns we want to load
    for (int i = 0; i < csvColumns.size(); i++)
    {
        // Check if the current CSV column is one we want to load
        for (const string& targetCol : columnsToLoad)
        {
            if (csvColumns[i] == targetCol)
            {
                column_indices.push_back(i);
                break;
            }
        }
    }

    // Create table with only certain columns in the CSV
    Table* newTable = new Table(columnsToLoad);

    string line;
    while (maxRows > 0 && getline(inputFile, line)) {
        // Process each line here
        // cout << "line: " << line << endl;
        // Delimter = ','
        vector<string> allRowValues = split(line, ',');

        // Only extract columns specified in column_indices
        vector<string> filteredRowValues;
        for (int idx : column_indices)
        {
            if (idx < allRowValues.size())
            {
                filteredRowValues.push_back(allRowValues[idx]);
            }
        }

        newTable->addRow(filteredRowValues);
        maxRows--;
    }
    // newTable->printTable();

    inputFile.close();
    return newTable;
}


void writeCSV(string filepath, Table& toWrite)
{
    ofstream outputFile(filepath, ios::out);

    if (outputFile.is_open()) {
        for (const auto& cols: toWrite.getAllCols())
        {
            outputFile << cols << ", ";
        }
        outputFile << "\n";

        for (const auto& row: toWrite.getAllRows())
        {
            for (const auto& value: row)
            {
                outputFile << value << ", ";
                // cout << "value = " << value << endl;
            }
            outputFile << "\n";
        }
    }
    else {
        cerr << "Error opening output file!" << endl;
    }
    outputFile.close();
}

namespace cpu 
{
    
    // "SELECT" Filter based on column value
    Table* filter(Table& toFilter, string column, string target)
    {
        // TODO: make finding the column index more efficient
        int columnIndex = toFilter.getColumnIndex(column);
        if (columnIndex == -1)
        {
            return nullptr;
        }

        // Go through each row in the original table, look at the value in the relevant column
        const vector<string>& cols = toFilter.getAllCols();
        Table* filtered = new Table(cols); // Make a new table with the same column names
        const vector<vector<string>>& allRows = toFilter.getAllRows();
        
        // Pre-allocate space to reduce reallocations (estimate ~50% match rate)
        // Access private member through a workaround - we'll use reserve if we add a method
        // For now, just iterate efficiently
        for (const auto& tempRow : allRows)
        {
            if (tempRow[columnIndex] == target)
                filtered->addRow(tempRow);
        }
        return filtered;
    }

    vector<string> combineRows(const vector<string>& existingRow, const vector<string>& rowToAdd)
    {
        vector<string> combined;
        combined.reserve(existingRow.size() + rowToAdd.size());
        combined.insert(combined.end(), existingRow.begin(), existingRow.end());
        combined.insert(combined.end(), rowToAdd.begin(), rowToAdd.end());
        return combined;        
    }

    // COUNT function in SQL
    Table* aggregate(Table& toGroup, string groupByCol, string toAggCol, AggType function)
    {
        int sum = 0;
        int groupBy = toGroup.getColumnIndex(groupByCol);
        int toAgg = toGroup.getColumnIndex(toAggCol); // TODO: used to get sum, when function != COUNT
        // Group By Column: groupByCol
        if (groupBy == -1 || toAgg == -1)
            return nullptr;

        // Sum up the values in aggCol
        // First group them by groupByCol using a hashtable
        unordered_map<string, vector<int>> hashTable;
                     // key      // value

        // TODO: Currently not taking any other functions, just summing them up
        const vector<vector<string>>& allRows = toGroup.getAllRows();
        for(const auto& row : allRows)
        {
            string key = row[groupBy]; // The value in the relevant column = key
            hashTable[key].push_back(stoi(row[toAgg])); 
                // add the int to the vector list from the relevant column, toAgg
            // hashTable[key] += 1; // counting the number of rows with this key
        }
        // Create a table from the hashTable
        Table* grouped = new Table({groupByCol, "aggregate"});
        // cout << endl << "groupBy = " << groupBy << endl;

        vector<string> temp;

        for (const auto& row : hashTable)
        {
            // temp = toGroup.getRow(count).push_back(hashTable[key]); count++;
            temp.clear();
            temp.push_back(row.first);
            if (function == COUNT)
                temp.push_back(to_string(row.second.size())); // Convert to string
            else 
            {
                double sum = accumulate(row.second.begin(), row.second.end(), 0);
                if (function == SUM)
                {
                    temp.push_back(to_string(  sum  ));
                }
                else // function == AVG
                {
                    temp.push_back(to_string(  sum / row.second.size()  ));
                }
            }
            grouped->addRow(temp);
        }        
        // grouped->printTable();
        return grouped;
    }

    // Making wider rows based on column value
    Table* hash_join(Table& tableOne, Table& tableTwo, string columnIndex1, string columnIndex2)
    {
        int colIndex1 = tableOne.getColumnIndex(columnIndex1);
        int colIndex2 = tableTwo.getColumnIndex(columnIndex2);
        if (colIndex1 == -1 || colIndex2 == -1)
            return nullptr;

        vector<string> newCols = tableOne.getAllCols();
        vector<string> twoCols = tableTwo.getAllCols();
        // Concatenate the columns from both tables:
        newCols.insert(newCols.end(), twoCols.begin(), twoCols.end());
        // Removing the duplicate column from tableTwo (not tableOne):
        newCols.erase(newCols.begin() + tableOne.getNumCols() + colIndex2); 

        Table* joined = new Table(newCols);
        // OPTIMIZATION: Store row indices instead of full rows to save memory
        unordered_map<string, vector<int>> hashTable;

        // go through tableOne and store row indices instead of full rows
        const vector<vector<string>>& tableOneRows = tableOne.getAllRows();
        for (int i = 0; i < tableOneRows.size(); i++)
        {
            string key = tableOneRows[i][colIndex1];
            hashTable[key].push_back(i);  // Store index instead of full row
        }

        // Now, look at second table and join based on colIndex1 and 2
        // Add to the "joined" table if we have a matching column value
        const vector<vector<string>>& tableTwoRows = tableTwo.getAllRows();
        for (int j = 0; j < tableTwoRows.size(); j++)
        {
            const auto& rowToAdd = tableTwoRows[j];
            string key = rowToAdd[colIndex2];
            // Use the value in the given column as the hash key
            if (hashTable.find(key) != hashTable.end())
            {
                for (int rowIdx : hashTable[key])
                {
                    const auto& existingRow = tableOneRows[rowIdx];
                    // Add the extra values from tableTwo to the existing row
                    // colIndex2 is the index to not add to the extended row -- we don't want duplicates
                    
                    // Reserve space to avoid reallocations
                    vector<string> combined;
                    combined.reserve(existingRow.size() + rowToAdd.size() - 1);
                    combined.insert(combined.end(), existingRow.begin(), existingRow.end());
                    
                    // Add rowToAdd except for the duplicate column
                    for (int k = 0; k < rowToAdd.size(); k++)
                    {
                        if (k != colIndex2)
                            combined.push_back(rowToAdd[k]);
                    }
                    joined->addRow(combined);
                }
            }
        }
        return joined;
    }
}

void runCPU()
{
    // STEP 0: LOADING DATA - Load first 10 rows of New York Taxi Dataset - 2015
    Table* table_2015 = loadCSV("/content/yellow_tripdata_2015-01.csv", 1000, {"VendorID", "passenger_count", "payment_type", "trip_distance", "fare_amount"});
    // Table* table_2015 = loadCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/yellow_tripdata_2015-01.csv", 500);
    if (!table_2015) {
        cerr << "Failed to load table_2015" << endl;
        return;
    }
    table_2015->appendStringtoColumns("2015");

    Table* table_2016 = loadCSV("/content/yellow_tripdata_2016-01.csv", 1000, {"VendorID", "passenger_count", "payment_type", "trip_distance", "fare_amount"});
    // Table* table_2016 = loadCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/yellow_tripdata_2016-01.csv", 500);
    if (!table_2016) {
        cerr << "Failed to load table_2016" << endl;
        delete table_2015;
        return;
    }
    table_2016->appendStringtoColumns("2016");

    // START CLOCK
    // BENCHMARK THE CPU
    auto startFilter = chrono::high_resolution_clock::now();
    // STEP 1: FILTER
    Table* solo_15 = cpu::filter(*table_2015, "2015_passenger_count", "1");
    if (!solo_15) {
        cerr << "CPU filter returned nullptr" << endl;
        delete table_2015;
        delete table_2016;
        return;
    }
    auto endFilter = chrono::high_resolution_clock::now();
    auto usFilter = std::chrono::duration_cast<chrono::microseconds>(endFilter - startFilter);
    cout << endl << endl << "CPU Time to Filter 2015 table: " << usFilter.count() << " μs \n";
    cout << "Number of rows = " << (solo_15->getNumRows()) << endl;

    // writeCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/CSV/written15.csv", *solo_15);
    Table* solo_16 = cpu::filter(*table_2016, "2016_passenger_count", "1");
    if (!solo_16) {
        cerr << "CPU filter returned nullptr" << endl;
        delete table_2015;
        delete table_2016;
        delete solo_15;
        return;
    }
    cout << "Number of rows = " << (solo_16->getNumRows()) << endl;    
         // writeCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/CSV/written16.csv", *solo_16);

   
    
         // STEP 2: JOIN - Joining January 2015 and 2016 data for single passengers
    // TODO: potentially create a new column with location + distance as a joiner.
    auto startJoin = chrono::high_resolution_clock::now();
    Table* joined = cpu::hash_join(*solo_15, *solo_16, "2015_trip_distance", "2016_trip_distance");
    if (!joined) {
        cerr << "CPU hash_join returned nullptr" << endl;
        delete table_2015;
        delete table_2016;
        delete solo_15;
        delete solo_16;
        return;
    }
        // writeCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/CSV/joined.csv", *joined); // joined->printTable();
    auto endJoin = chrono::high_resolution_clock::now();
    auto msJoin = std::chrono::duration_cast<std::chrono::milliseconds>(endJoin - startJoin);
    cout << endl << endl << "CPU Time to Hash Join 2015 and 2016 tables: " << msJoin.count() << " ms\n";

    // STEP 3: AGGREGATE

    auto startAgg = chrono::high_resolution_clock::now();
    Table* agg = cpu::aggregate(*joined, "2015_VendorID", "2015_fare_amount", AVG);
    if (!agg) {
        cerr << "CPU aggregate returned nullptr" << endl;
        delete table_2015;
        delete table_2016;
        delete solo_15;
        delete solo_16;
        delete joined;
        return;
    }
        // writeCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/CSV/aggregate.csv", *agg); // joined->printTable();
    auto endAgg = chrono::high_resolution_clock::now();
    auto msAgg = std::chrono::duration_cast<std::chrono::milliseconds>(endAgg - startAgg);
    cout << endl << endl << "CPU Time to Aggregate joined table: " << msJoin.count() << " ms\n";
    agg->printTable();
    
    
    // Clean up memory
    delete table_2015;
    delete table_2016;
    delete solo_15;
    delete solo_16;
    delete joined;
    delete agg;
}

int main()
{
    runCPU();
}
