#include <fstream>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <cassert>

int main(int argc, char **argv) 
{

    if (argc < 2) {
	std::cout << "Usage: ./toAmplDat inputfile" << std::endl;
	abort();
    }
    
    std::ifstream data_stream(argv[1]);
    
    if (!data_stream){
	std::cout << "Error opening input data file. Aborting.\n";
	abort();
    }
    
    std::string key;
    int value1, value2;
    double capacity;
    
    std::vector<int> sizes;
    std::vector<int> profits;
    
    while (data_stream >> key){
	if (key == "CAPACITY") {
	    data_stream >> value1;
	    capacity = value1;
	} 
	else if  (key == "ITEM") {
	    data_stream >> value1;
	    data_stream >> value2;
	    sizes.push_back(value1);
	    profits.push_back(value2);
	}
    }
    
    int j;
    int numItems = sizes.size();

    assert(numItems == profits.size());
    
    char fName [100];

    sprintf(fName, "%s.dat", argv[1]);

    std::ofstream fout(fName);

    std::cout << "create date file " << fName << std::endl;
    
    fout << "set Item:=";
    for (j = 0; j < numItems; ++j) {
	if (j % 10 == 0) fout << std::endl;
	fout << j << " ";
    }
    fout << std::endl << ";" << std::endl << std::endl;
    
    fout << "param : Size    Profit :=" << std::endl;
    for (j = 0; j < numItems; ++j) {
	fout << j << "\t" << sizes[j] << "\t" << profits[j] << std::endl;
    }
    fout << ";" << std::endl << std::endl;
    
    fout << "param capacity := " << capacity << ";" << std::endl;

    return 0;
}
