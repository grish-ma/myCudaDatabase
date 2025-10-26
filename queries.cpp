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

        vector<string> getAllCols()
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

        vector<vector<string>> getAllRows()
        {
            if (numRows == 0)
                cout << endl << endl << "numRows = 0"  << endl << endl;
            //     return nullptr; // 
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
        Table* filtered = new Table(toFilter.getAllCols()); // Make a new table with the same column names
        vector<string> tempRow = {};
        for (const auto& tempRow : toFilter.getAllRows())
        {
            if (tempRow[columnIndex] == target)
                filtered->addRow(tempRow);
        }
        return filtered;
    }

    vector<string> combineRows(vector<string> existingRow, vector<string> rowToAdd)
    {
        vector<string> combined;
        for (const auto& s: existingRow) // This const format shows that we do not want to edit the variable, just read it.
        {
            combined.push_back(s);
        }
        for (const auto& s : rowToAdd)
        {
            combined.push_back(s);
        }

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
        for(vector<string> row : toGroup.getAllRows())
        {
            string key = row[groupBy]; // The value in the relevant column = key
            hashTable[key].push_back(stoi(row[toAgg])); 
                // add the int to the vector list from the relevant column, toAgg
            // hashTable[key] += 1; // counting the number of rows with this key
        }
        // Create a table from the hashTable
        Table* grouped = new Table({groupByCol, "aggregate"});
        cout << endl << "groupBy = " << groupBy << endl;

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
        grouped->printTable();
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
        // Removing the duplicate column:
        newCols.erase(newCols.begin() + colIndex1); 

        Table* joined = new Table(newCols);
        unordered_map<string, vector<vector<string>>> hashTable;

        // go through tableOne and insert all the elements in the
        // right spots based on the given colIndex
        for (const auto& row : tableOne.getAllRows())
        {
            string key = row[colIndex1];
            // Use the value in the given column as the hash key
            hashTable[key].push_back(row);
        }

        // Now, look at second table and join based on colIndex1 and 2
        // Add to the "joined" table if we have a matching column value
        for (vector<string> rowToAdd : tableTwo.getAllRows())  
        {
            string key = rowToAdd[colIndex2];
            // Use the value in the given column as the hash key
            if (hashTable.find(key) != hashTable.end())
            {
                for (const auto& existingRow : hashTable[key])
                {   // existingRow is a vector<string>
                    // Add the extra values from tableTwo.getRow(row) to the existing row

                    // colIndex2 is the index to not add to the extended row -- we don't want duplicates
                    // We make a copy to avoid trying to edit the row we are adding

                    vector<string> addCopy = rowToAdd;
                    addCopy.erase(addCopy.begin() + colIndex2);
                    vector<string> combined = combineRows(existingRow, addCopy);
                    joined->addRow(combined);
                }
            }
        }
        return joined;
    }
}



namespace gpu {
    // __global__ Table* filter()
    // {
    //     int thread = threadIdx.x; // ?
    // }
    
    // The following methods will call the relevant kernels, figuring out the correct number of blocks, threads, etc. 
    Table* filter(Table& toFilter, string column, string target)
    {
        Table* result;
        // Create pointers in the GPU

        // Allocate Memory in the GPU

        // Copy the vectors into the GPU

        return result;
    }

    vector<string> combineRows(vector<string> existingRow, vector<string> rowToAdd)
    {
        vector<string> result;

        return result;
    }

    Table* aggregate(Table& toGroup, string groupByCol, string toAggCol, AggType function)
    {
        Table* result;
        return result;
    }

    Table* hash_join(Table& tableOne, Table& tableTwo, string columnIndex1, string columnIndex2)
    {
        Table* result;
        return result;
    }
}

void runCPU()
{
    // STEP 0: LOADING DATA - Load first 10 rows of New York Taxi Dataset - 2015
    Table* table_2015 = loadCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/yellow_tripdata_2015-01.csv", 300);
    table_2015->appendStringtoColumns("2015");

    Table* table_2016 = loadCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/yellow_tripdata_2016-01.csv", 300);
    table_2016->appendStringtoColumns("2016");

    // START CLOCK
    // BENCHMARK THE CPU
    auto start = chrono::high_resolution_clock::now();

    // STEP 1: FILTER
    Table* solo_15 = cpu::filter(*table_2015, "2015_passenger_count", "1");
        // writeCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/CSV/written15.csv", *solo_15);
    Table* solo_16 = cpu::filter(*table_2016, "2016_passenger_count", "1");    
         // writeCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/CSV/written16.csv", *solo_16);

    // STEP 2: JOIN - Joining January 2015 and 2016 data for single passengers
    // TODO: potentially create a new column with location + distance as a joiner.
    Table* joined = cpu::hash_join(*solo_15, *solo_16, "2015_trip_distance", "2016_trip_distance");
        // writeCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/CSV/joined.csv", *joined); // joined->printTable();


    // STEP 3: AGGREGATE
    Table* agg = cpu::aggregate(*joined, "2015_VendorID", "2015_fare_amount", AVG);
        // writeCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/CSV/aggregate.csv", *agg); // joined->printTable();
    auto end = chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    cout << endl << endl << "CPU Time: " << ms.count() << " ms\n";

    
    // Clean up memory
    delete table_2015;
    delete table_2016;
    delete solo_15;
    delete solo_16;
    delete joined;
    delete agg;
}

void runGPU()
{
    // STEP 0: LOADING DATA - Load first 10 rows of New York Taxi Dataset - 2015
    Table* table_2015 = loadCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/yellow_tripdata_2015-01.csv", 300);
    table_2015->appendStringtoColumns("2015");

    Table* table_2016 = loadCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/yellow_tripdata_2016-01.csv", 300);
    table_2016->appendStringtoColumns("2016");

    // START CLOCK
    // TIME THE GPU
    auto start = chrono::high_resolution_clock::now();

    // STEP 1: FILTER
    Table* solo_15 = gpu::filter(*table_2015, "2015_passenger_count", "1");
        // writeCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/CSV/written15_GPU.csv", *solo_15);
    Table* solo_16 = gpu::filter(*table_2016, "2016_passenger_count", "1");    
         // writeCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/CSV/written16_GPU.csv", *solo_16);

    // STEP 2: JOIN - Joining January 2015 and 2016 data for single passengers
    // TODO: potentially create a new column with location + distance as a joiner.
    Table* joined = gpu::hash_join(*solo_15, *solo_16, "2015_trip_distance", "2016_trip_distance");
        // writeCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/CSV/joined_GPU.csv", *joined); // joined->printTable();


    // STEP 3: AGGREGATE
    Table* agg = gpu::aggregate(*joined, "2015_VendorID", "2015_fare_amount", AVG);
        // writeCSV("C:/Users/grish/OneDrive/Desktop/GitHub/myCudaDatabase/CSV/aggregate_GPU.csv", *agg); // joined->printTable();
    
    auto end = chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    cout << endl << endl << "GPU Time: " << ms.count() << " ms\n";

    
    // Clean up memory
    delete table_2015;
    delete table_2016;
    delete solo_15;
    delete solo_16;
    delete joined;
    delete agg;
}

void testingMySQL()
{
    vector<string> colNames = {
        "trip_id", 
        "VendorID",
        "fare",
        "passengers",
    };

    // Hard-coding rows here for now.
    Table* mainTable = new Table(colNames);
    mainTable->addRow({"1", "2", "15", "1"});
    mainTable->addRow({"2", "1", "22", "2"});
    mainTable->addRow({"3", "2", "8", "1"});
    mainTable->addRow({"4", "1", "30", "3"});
    // Print Table
    cout<< "mainTable" << endl;
    mainTable->printTable();
    cout << endl;



    vector<string> colNames2 = {"VendorID", "vendor_name", "commission"};
    Table* two = new Table(colNames2);
    two->addRow({"1", "Creative Mobile", "5%"});
    two->addRow({"2", "VeriFone", "3%"});
    // Print Table
    cout<< "two" << endl;
    two->printTable();
    cout << endl;

    // Testing filter()
            // dereferencing mainTable because we never want to allow nullptr
    Table* temp = cpu::filter(*mainTable, "passengers", "1");
    temp->printTable();

    temp = cpu::hash_join(*mainTable, *two, "VendorID", "VendorID"); cout << endl << endl;
    temp->printTable();
}

int main()
{
    runCPU();

    // TODO: edit so that timer is only timing the SQL queries
    auto startGPU = chrono::high_resolution_clock::now();
    // runGPU();
    auto endGPU = chrono::high_resolution_clock::now();
    auto msGPU = std::chrono::duration_cast<std::chrono::milliseconds>(endGPU - startGPU);

    cout << endl << endl << "GPU Time: " << msGPU.count() << " ms\n";
}
