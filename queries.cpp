// Queries to write:

// Version 1 -- 
// CREATE TABLE table_name (~~~~)
// SELECT (DISTINCT) col1, col2, ... // or use * for all 
// FROM table
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

// Table class
class Table {
    private:
        int numRows; int numCols;
        vector<vector<string>> tableRows;
        vector<string> colNames;

    public:
        // constructor --> numRows = 0; colNames
        Table(vector<string> columns) : numRows(0), numCols(columns.size()), colNames(columns)
        {
            // tableRows.push_back(columns); // Adding the first row as column name
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

        vector<string> getCols()
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

        void printTable()
        {
            cout << "numRows = " << numRows;
            cout << "\tnumCols = " << numCols << endl;;
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
};

namespace cpu 
{
    
    // "SELECT" Filter based on column value
    Table* filter(Table& toFilter, string column, string target)
    {
        // TODO: make finding the column index more efficient
        // Finding column index
        int columnIndex = 0;
        for (int c = 0; c < toFilter.getNumCols(); c++)
        {
            if(toFilter.getCols()[c] == column)
            {
                columnIndex = c;
                break;
            }
        }

        // Go through each row in the original table, look at the value in the relevant column
        Table* filtered = new Table(toFilter.getCols()); // Make a new table with the same column names
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

    // Making wider rows based on column value
    Table* hash_join(Table& tableOne, Table& tableTwo, int colIndex1, int colIndex2)
    {
        vector<string> newCols = tableOne.getCols();
        vector<string> twoCols = tableTwo.getCols();
        // Concatenate the columns from both tables:
        newCols.insert(newCols.end(), twoCols.begin(), twoCols.end());
        // Removing the duplicate column:
        newCols.erase(newCols.begin() + colIndex2); 

        Table* joined = new Table(newCols);
        unordered_map<string, vector<vector<string>>> hashTable;;

        // go through tableOne and insert all the elements in the 
        // right spots based on the given colIndex
        for (vector<string> row : tableOne.getAllRows())
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
    
}




int main()
{
    // needs colNames
    // needs tableRows?
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

    temp = cpu::hash_join(*mainTable, *two, 1, 0); cout << endl << endl;
    temp->printTable();
}
