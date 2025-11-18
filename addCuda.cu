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
#include <math.h>

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
   // if (toFilter[flatIndex] == target) // too exact
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
__global__  // Making wider rows based on column value
void hash_join_kernel(
    int* count, // for atomicAdd to keep track of output indices
    double* tableTwo, // Table we are trying to join to hashTable
    int column2, // Relevant column index in table two
    double* keys, // exising keys in hashtable from table one
    double* outputKeys, // output keys from table two
    double* values, // exising values
    double* outputValues, // output values
    int tableSize, // number of rows/keys/values in hashtable
    int numColsOne,
    int numColsTwo, // number of columns in table two
    int numRowsTwo) // number of rows in table two
{
    int i = threadIdx.x + blockIdx.x*blockDim.x; // i gives us the row that this thread is handling
    int outCols = numColsOne + numColsTwo - 1;


    if(i >= numRowsTwo)
      return;

    int flatIndex = i * numColsTwo + column2; // looks directly at the relevant column in flattened tableTwo
    double key = tableTwo[flatIndex]; // key for second table

    for (int k = 0; k < tableSize; k++) // Going through all the rows of the hashTable/tableOne
    {
        if (fabs(key-keys[k]) < 0.9)
        {
            int pos = atomicAdd(count, 1); // added a row to the output table. cannot do count++ in GPU.
            if (pos >= numRowsTwo) {
                // avoid overflow
                return;
            }


            outputKeys[pos] = key;
            // Copying table two row to output
            for (int j = 0; j < numColsOne; j++)
            {
                outputValues[pos*(outCols) + j] = values[k*(numColsOne) + j]; // Can't use push_back() because GPU doesn't work with dynamic memories
            }

            // copy tableTwo row except the join column (use i)
            int base = pos*(outCols);
            int dst = base + numColsOne;
            for (int j = 0; j < numColsTwo; ++j) {
                if (j == column2) continue;
                outputValues[dst++] = tableTwo[i * numColsTwo + j];
            }

            // Copying table two row to output
//            for (int j = numColsOne; j < outCols; j++)
//            {
//                outputValues[pos*(outCols) + j] = tableTwo[i*(numColsTwo) + j]; // Can't use push_back() because GPU doesn't work with dynamic memories
//
//            }
        }
    }
    return;
}


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
namespace gpu
{

    Table* hash_join(Table& tableOne, Table& tableTwo, string columnIndex1, string columnIndex2)
    {
        // Flatten our Tables to 1D // return vector<double> since GPU cannot process strings
        vector<double> flatOne = tableOne.flatten();
        vector<double> flatTwo = tableTwo.flatten();

  // CPU Code to create hashtable ///////////////////////////////////////////////////////////////
        // TO-DO -- do this hashtable thing in the GPU
        int colIndex1 = tableOne.getColumnIndex(columnIndex1);
        int colIndex2 = tableTwo.getColumnIndex(columnIndex2);
        // cout << "Finding columns" << columnIndex1 << "=" << colIndex1 << " and " << columnIndex2 << "=" << colIndex2 << endl;

        if (colIndex1 == -1 || colIndex2 == -1)
        {
            cout << "No column named " << columnIndex1 << " or " << columnIndex2 << " exists " << endl;
            return nullptr;
        }
        vector<string> newCols = tableOne.getAllCols();
        vector<string> twoCols = tableTwo.getAllCols();
        // Concatenate the columns from both tables:
        newCols.insert(newCols.end(), twoCols.begin(), twoCols.end());
        // Removing the duplicate column:
        newCols.erase(newCols.begin() + colIndex2);

        Table* joined = new Table(newCols);
        // unordered_map<string, vector<vector<string>>> hashTable;
        vector<double> vectorHashKeys;
        Table* vectorHashValues = new Table(tableOne.getAllCols()); // Will contain all the row indices from tableOne

        // Go through tableOne and store all the elements in the
        // right spots based on the given colIndex
        // + Now convert to what we can use in the GPU.
        for (const auto& row : tableOne.getAllRows())
        // for (int r = 0; r < tableOne.getNumRows(); r++)
        {
            // const auto& row = tableOne.getRow(r);
            string key = row[colIndex1];
           // cout << "key = " << key << endl;

            // Use the value in the given column as the hash key
            // hashTable[key].push_back(row);
            vectorHashKeys.push_back(stod(key)); //  only recording row index in hashtable
            vectorHashValues->addRow(row); // pushing entire row
        }
        vector<double> flatValues = vectorHashValues->flatten();

/////////////////////////////////////////////////////////////////////////////////////////////////////
        // TIME THE GPU - including memory transfer
        auto startMem = chrono::high_resolution_clock::now();

        // Create pointers in the GPU. Parameters of the hash_join_kernel(). If its value doesn't need to change, don't need a pointer.
        // Any output values you want should be pointers and sent as parameters to __global__ function
        int* d_count;
        double* tableTwo_ptr;
        double* keys_ptr;
        double* outKeys_ptr;
        double* values_ptr;
        double* outValues_ptr;

        // Allocate Memory in the GPU for all the pointers: cudaMalloc(&ptr, N * sizeof(type)); → type = type of ELEMENTS, size = # of bytes = # of elements * sizeof(double)
        cudaMalloc(&tableTwo_ptr, flatTwo.size() * sizeof(double));
        cudaMalloc(&keys_ptr, vectorHashKeys.size()*sizeof(double));
              int maxOutRows = tableTwo.getNumRows();
        cudaMalloc(&outKeys_ptr, sizeof(double) * maxOutRows);
                            // int maxOutSize = flatValues.size() + flatTwo.size();
                            // cudaMalloc(&outKeys_ptr, (maxOutSize)*sizeof(double));
            // Allocating max size possible
        cudaMalloc(&values_ptr, flatValues.size()*sizeof(double));
                int maxOutputSize = tableOne.getNumRows() * tableTwo.getNumRows();
        cudaMalloc(&outValues_ptr, sizeof(double) * maxOutputSize * newCols.size());


                            // cudaMalloc(&outValues_ptr, flatValues.size()*sizeof(double));
            // Allocating max size possible
        cudaMalloc(&d_count, sizeof(int));
            cudaMemset(d_count, 0, sizeof(int)); // initialize to 0
        // Copy the vectors into the GPU using cudaMemcpy(..., cudaMemcpyHostToDevice)
            // Only need to copy input values, not empty output pointers
        cudaMemcpy(tableTwo_ptr, flatTwo.data(), flatTwo.size()*sizeof(double), cudaMemcpyHostToDevice);
        cudaMemcpy(keys_ptr, vectorHashKeys.data(), vectorHashKeys.size()*sizeof(double), cudaMemcpyHostToDevice);
        cudaMemcpy(values_ptr, flatValues.data(), flatValues.size()*sizeof(double), cudaMemcpyHostToDevice);

        // Call the kernel function
     //   int threads = tableTwo.getNumRows();
      //  int blocks = 1; //(num_rows + threads - 1) / threads;
        int threads = 256;
        int blocks = (tableTwo.getNumRows() + threads - 1) / threads;

        // TIMING PURPOSES
        // Create CUDA events
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

        // Record start
        cudaEventRecord(start);

        hash_join_kernel<<<blocks, threads>>>(
            d_count,
            tableTwo_ptr,
            colIndex2,
            keys_ptr,
            outKeys_ptr,
            values_ptr,
            outValues_ptr,
            vectorHashKeys.size(),
            tableOne.getNumCols(),
            tableTwo.getNumCols(),
            tableTwo.getNumRows());
        cudaDeviceSynchronize();

        // Record end
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);

        // Calculating CUDA Event Time
        float ms = 0;
        cudaEventElapsedTime(&ms, start, stop);
        cout << endl << endl << "GPU Hash Join Kernel Time: " << ms << " ms\n";

        // Cleanup
        cudaEventDestroy(start);
        cudaEventDestroy(stop);

        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess) {
            cout << "CUDA Error: " << cudaGetErrorString(err) << endl;
        }

        // Copy results back to CPU using cudaMemcpy(..., cudaMemcpyDeviceToHost)
        int resultCount = 0;
        int outCols = (int)newCols.size(); // number of columns in joined table

        cudaMemcpy(&resultCount, d_count, sizeof(int), cudaMemcpyDeviceToHost);
                // vector<double> keysResult = vector<double>(resultCount);
                // cudaMemcpy(keysResult.data(), outKeys_ptr, keysResult.size()*sizeof(double), cudaMemcpyDeviceToHost);
        cout << "Hash Join resultCount = " << resultCount << endl;

        vector<double> valuesResult;
        if (resultCount > 0) {
            // allocate host buffer of the full size: rows * columns
            valuesResult.assign((size_t)resultCount * outCols, 0.0);
            // copy the full block back
            cudaMemcpy(valuesResult.data(),
                       outValues_ptr,
                       valuesResult.size() * sizeof(double),
                       cudaMemcpyDeviceToHost);
        }
        cudaMemcpy(valuesResult.data(), outValues_ptr, resultCount*sizeof(double), cudaMemcpyDeviceToHost);

        // Calculate total time including memory transfers
        auto endMem = chrono::high_resolution_clock::now();
        auto msTotal = std::chrono::duration_cast<std::chrono::milliseconds>(endMem - startMem);
        cout << "GPU Hash Join Time With Memory Transfer: " << msTotal.count() << " ms\n";

        cudaFree(d_count);
        cudaFree(tableTwo_ptr);
        cudaFree(keys_ptr);
        cudaFree(outKeys_ptr);
        cudaFree(values_ptr);
        cudaFree(outValues_ptr); // Free all pointers


        // Unflatten Tables if needed
        joined = Table::fromFlattened(valuesResult, newCols, resultCount);

        return joined;
    }


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
        // cout << "colIndex: " << colIndex << endl;
        int num_rows = toFilter.getNumRows(); // value doesn't change
        int num_cols = toFilter.getNumCols(); // value doesn't change
        int* d_count; // Keeps count of the number of rows in the filtered table.

        // TODO CHAT CODE
        // cout << "num_rows=" << num_rows << " num_cols=" << num_cols << " flat.size()=" << flat.size() << endl;
        if (!flat.empty()) {
          // cout << "flat[0..min(5,flat.size()-1)]: ";
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

        filter_kernel<<<blocks, threads>>>(
            toFilter_ptr,
            filtered_ptr,
            target_dbl,
            colIndex,
            num_rows,
            num_cols,
            d_count);
        cudaDeviceSynchronize();

      // Record end
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);

      // Clock end
        auto endFilter = chrono::high_resolution_clock::now();


      // Calculating Clock Time
        auto msFilter = std::chrono::duration_cast<std::chrono::milliseconds>(endFilter - startFilter);
        msFilter = std::chrono::duration_cast<std::chrono::milliseconds>(endFilter - startMem);
        cout << endl << endl << "GPU Time to Filter table With Memory Transfer: " << msFilter.count() << " ms\n";
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
    Table* table_2015 = loadCSV("/content/yellow_tripdata_2015-01.csv", 100000, {"VendorID", "passenger_count", "payment_type", "trip_distance", "fare_amount"});
    table_2015->appendStringtoColumns("2015");
            // table_2015->printTable();

    Table* table_2016 = loadCSV("/content/yellow_tripdata_2016-01.csv", 100000, {"VendorID", "passenger_count", "payment_type", "trip_distance", "fare_amount"});
    table_2016->appendStringtoColumns("2016");


    // STEP 1: FILTER
    Table* solo_15 = gpu::filter(*table_2015, "2015_passenger_count", "1");
      // solo_15->printTable();
    int temp = solo_15->getNumRows();
    cout << "Number of rows in filtered 2015 table = " << (temp) << endl;
    Table* solo_16 = gpu::filter(*table_2016, "2016_passenger_count", "1");
      // solo_16->printTable();
    temp = solo_16->getNumRows();
    cout << "Number of rows in filtered 2016 table = " << (temp) << endl;


    // STEP 2: JOIN
    Table* joined = gpu::hash_join(*solo_15, *solo_16, "2015_trip_distance", "2016_trip_distance");
    // joined->printTable();

    // STEP 3: AGGREGATE
    Table* agg = gpu::aggregate(*joined, "2015_VendorID", "2015_fare_amount", AVG);
    // agg->printTable();
}

int main()
{
    runGPU();
    return 0;
}