// Filename:  knapGen.cpp
// Date:      Mar. 29, 2003
// Author:    Yan Xu
// Purpose:   Generate knapsack test data for Alps
// Input:     number of items(n), upbound of coefficients(r), 
//            capacity ratio (c: range from 0 to 1), type (1=uncorr., 
//            2=weakly corr., 3=strongly corr.)
// Output:    A data file (input.txt)
// Usage:     To compile the code use:
//
//              g++ -o knapGen knapGen.cpp
//
//            To run it, use
//
//              ./knapGen n r c type
//
//#############################################################################

#include <ctime>
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char * argv[])
{
  if (argc != 5) {
    cout << "Usage: excutable n r c type ";
    abort();
  }

  // initialize random generator.
  srand ( time(NULL) ); 

  int numItems = atoi(argv[1]);
  int R = atoi(argv[2]);
  double ratio = atof(argv[3]);
  int type = atoi(argv[4]);

  int weight;
  int profit;
  int sum;

  int i;
  sum = 0;
  ofstream fout("input.txt");

  switch (type) {
  case 1:  // No correlated
    for (i = 0; i < numItems; ++i) {
      weight = 1 + static_cast<int>(double(rand()) / RAND_MAX * (R - 1));
      profit = 1 + static_cast<int>(double(rand()) / RAND_MAX * (R - 1));
      sum += weight;
      fout << "ITEM" << " " << weight << " " << profit << endl;
    }
    break;
  case 2:  // Weakly correlated
    for (i = 0; i < numItems; ++i) {
      weight = 1 + static_cast<int>(double(rand()) / RAND_MAX * (R - 1));
      profit = weight - R/10 + 
	static_cast<int>(double(rand())/RAND_MAX * (R/5));
      if (profit <= 0)
	profit = 1;
      sum += weight;
      fout << "ITEM" << " " << weight << " " << profit << endl;
    }
    break;
  case 3:  // Strongly correlated
    for (i = 0; i < numItems; ++i) {
      weight = 1 + static_cast<int>(double(rand()) / RAND_MAX * (R - 1));
      profit = weight + 10;
      sum += weight;
      fout << "ITEM" << " " << weight << " " << profit << endl;
    }
    break;
  }

  fout << "CAPACITY " << static_cast<int>(sum * ratio) << endl;
  fout.close();
}
