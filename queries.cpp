// Queries to write:

// Version 1 -- 
// CREATE TABLE table_name (~~~~)
// SELECT (DISTINCT) col1, col2, ... // or use * for all 
// FROM table
#include <iostream>
#include <string>
#include <vector>
using namespace std;

struct Row {
    vector<string> rowValues; // TODO: change to doubles or floats
};

struct Table {
    vector<Row> tableRows;
    vector<string> colNames;
};

void printTable(Table table)
{
    for (int i = 0; i < table.tableRows.size(); i++)
    {
        Row row = table.tableRows[i];
        for (int j = 0; j < row.rowValues.size(); j++)
        {
            string value = row.rowValues[j];
            cout << "value = " << value << endl;
        }
    }
}

Table executeCreateTable(vector<string> columns, vector<Row> rows)
{
    Table table;
    table.colNames = columns;
    table.tableRows = rows;
    
    return table;
}

// Table addRow(vector<string> columns, vector<Row> rows)
// {
//     Table table;
//     table.colNames = columns;
//     table.tableRows = rows;
    
//     return table;
// }

int main()
{
    // needs colNames
    // needs tableRows?
    vector<string> colNames = {
        "VendorID"
        "passenger_count",
        "trip_distance"
    };
    
    // Hard-coding rows here for now.
    Row row1;
    row1.rowValues = {
        "1", //VendorID
        "1", //tpep_dropoff_datetime
        "1.59", //trip_distance
    };
    
    Row row2;
    row2.rowValues = {
        "1", //VendorID
        "2", //tpep_dropoff_datetime
        "3.30", //trip_distance
    };
    
    vector<Row> rows;
    rows.push_back(row1);
    rows.push_back(row2);

    Table mainTable = executeCreateTable(colNames, rows);
    printTable(mainTable);
}