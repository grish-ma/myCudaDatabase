%%writefile my_cuda_code.cu
// Change to .cu, only using .cpp for the colors

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <numeric>
#include <chrono>

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

        vector<double> flatten() const {
            vector<double> flat;

            // Go through every single value
            for (const auto& row : tableRows) {
                for (const auto& value : row)
                {
                  // cout << "value: " << value << endl;
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

enum AggType {
    COUNT,    // implicitly assigned 0
    AVG,  // implicitly assigned 1
    SUM    // implicitly assigned 2
};

/////////////////////////////////////////////////////////////////////////////////
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
        // Delimiter = ','
        vector<string> rowValues = split(line, ',');
        newTable->addRow(rowValues);
        maxRows--;
    }
    // newTable->printTable();

    inputFile.close();
    return newTable;
}





////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
__global__
void filter_kernel(double* toFilter, double* filtered, double target, int column, int numRows, int numCols, int* count)
{
  int i = threadIdx.x + blockIdx.x*blockDim.x; // i gives us the row that this thread is handling
  int flatIndex;

  if(i < numRows)
  {
    flatIndex = i * numCols + column; // looks directly at the relevant column
   // if (toFilter[flatIndex] == target) // too extract
   if (fabs(toFilter[flatIndex] - target) < 0.1)
    {
      int pos = atomicAdd(count, 1); // added a row to the filtered table. cannot do count++ in GPU.
      for (int j = 0; j < numCols; j++)
      {
        filtered[pos*numCols + j] = toFilter[i*numCols + j]; // Can't use push_back() because GPU doesn't work with dynamic memories
      }

    }
  }

  return;
}

namespace gpu
{
    // "SELECT" Filter based on column value
    // The following methods will call the relevant kernels, figuring out the correct number of blocks, threads, etc.
    Table* filter(Table& toFilter, string column, string target)
    {
        // TIME THE GPU - including memory transfer
        auto startMem = chrono::high_resolution_clock::now();

        Table* result = new Table(toFilter.getAllCols());
        // need to flatten our Tables to 1D.
        vector<double> flat = toFilter.flatten(); // return vector<double> since GPU cannot process strings

        // Create pointers in the GPU. Parameters of the filter_kernel()
        double* toFilter_ptr;
        double* filtered_ptr; // Will point to the filtered table

            // cout << "target: " << target << endl;
        double target_dbl; // = stod(target); // convert target value to double // value doesn't change
        try { target_dbl = stod(target); } catch(...) { target_dbl = 0.0; cerr << "Trying to convert a non-numeric value to a double.";}
        int colIndex = toFilter.getColumnIndex(column); // no pointer because it's value doesn't need to change
        cout << "colIndex: " << colIndex << endl;
        int num_rows = toFilter.getNumRows(); // value doesn't change
        int num_cols = toFilter.getNumCols(); // value doesn't change
        int* d_count; // Keeps count of the number of rows in the filtered table.

        // TODO CHAT CODE
        cout << "num_rows=" << num_rows << " num_cols=" << num_cols << " flat.size()=" << flat.size() << endl;
        if (!flat.empty()) {
          cout << "flat[0..min(5,flat.size()-1)]: ";
          for (int k=0;k<min((size_t)5, flat.size()); ++k) cout << flat[k] << " ";
          cout << endl;
        }


        // Allocate Memory in the GPU for all the pointers
            // cudaMalloc(&ptr, N * sizeof(type)); → type = type of ELEMENTS
              // size = # of bytes = # of elements * sizeof(double)
        cudaMalloc(&toFilter_ptr, flat.size() * sizeof(double));
        cudaMalloc(&filtered_ptr, flat.size() * sizeof(double)); // allocate max amount of memory filtered table can take up (== original table)
        cudaMalloc(&d_count, sizeof(int));
            cudaMemset(d_count, 0, sizeof(int)); // initialize to 0
        // filtered_ptr doesn't need to be copied to the GPU because it doesn't contain anything




        // Copy the vectors into the GPU
        // cudaMemcpy	(	void * 	dst, const void * 	src, size_t 	count, enum cudaMemcpyKind 	kind)
              // cudaMemcpy(dst, src, N * sizeof(type), kind); → type = type of DATA being copied
        cudaMemcpy(toFilter_ptr, flat.data(), flat.size() * sizeof(double), cudaMemcpyHostToDevice);
                              // ^ flat.data() returns the pointer to flat




        // Call the kernel function
        // TODO: UNDERSTAND # OF THREADS AND BLOCKS
        int threads = 256;
        int blocks = (num_rows + threads - 1) / threads;

  // TIMING PURPOSES
        // Create CUDA events
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

      // Record start
        cudaEventRecord(start);
        // This is where the actual filtering happens
        // filter_kernel(double* toFilter, double* filtered, double target, int column, int numRows, int numCols, int count)
      // Clock start
        auto startFilter = chrono::high_resolution_clock::now();
        
        filter_kernel<<<blocks, threads>>>(toFilter_ptr, filtered_ptr, target_dbl, colIndex, num_rows, num_cols, d_count);
        cudaDeviceSynchronize();
        
      // Record end
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
        
      // Clock end  
        auto endFilter = chrono::high_resolution_clock::now();
    

      // Calculating Clock Time
        auto msFilter = std::chrono::duration_cast<std::chrono::milliseconds>(endFilter - startFilter);
        cout << endl << endl << "GPU Time to Filter 2015 table: " << msFilter.count() << " ms\n";
        msFilter = std::chrono::duration_cast<std::chrono::milliseconds>(endFilter - startMem);
        cout << endl << endl << "GPU Time to Filter 2015 table With Memory Transfer: " << msFilter.count() << " ms\n";
      // Calculating CUDA Event Time
        float ms = 0;
        cudaEventElapsedTime(&ms, start, stop);

        cout << "GPU Kernel Time: " << ms << " ms\n";

        // Cleanup
        cudaEventDestroy(start);
        cudaEventDestroy(stop);

        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess) {
            cout << "CUDA Error: " << cudaGetErrorString(err) << endl;
        }



        // Copy results back to CPU
        // We are using count to see how many rows are in the filtered table so that we can use it to copy back
        int resultCount;
        cudaMemcpy(&resultCount, d_count, sizeof(int), cudaMemcpyDeviceToHost);
        // Using resultCount to copy the filtered table
        // cout << "resultCount: " << resultCount << endl;
        vector<double> flatResult = vector<double>(resultCount * num_cols);
        cudaMemcpy(flatResult.data(), filtered_ptr, flatResult.size()*sizeof(double), cudaMemcpyDeviceToHost);

        cudaFree(toFilter_ptr);
        cudaFree(filtered_ptr);
        cudaFree(d_count);



        // Convert back to a table
        result = Table::fromFlattened(flatResult, toFilter.getAllCols(), resultCount);

        return result;
    }
}

void runGPU()
{
    // STEP 0: LOADING DATA - Load first 10 rows of New York Taxi Dataset - 2015
    Table* table_2015 = loadCSV("/content/yellow_tripdata_2015-01.csv", 800, {"VendorID", "passenger_count", "payment_type", "trip_distance", "fare_amount"});
    table_2015->appendStringtoColumns("2015");
            // table_2015->printTable();
    cout << endl << "value = " << table_2015->getValue(0, 1) << endl;

    Table* table_2016 = loadCSV("/content/yellow_tripdata_2016-01.csv", 800, {"VendorID", "passenger_count", "payment_type", "trip_distance", "fare_amount"});
    table_2016->appendStringtoColumns("2016");

    
    // STEP 1: FILTER
    Table* solo_15 = gpu::filter(*table_2015, "2015_passenger_count", "1");
    solo_15->printTable();


    // Table* solo_16 = gpu::filter(*table_2016, "2016_passenger_count", "1");
}

int main()
{
  runGPU();
  return 0;
}