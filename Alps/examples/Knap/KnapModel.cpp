/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors: Yan Xu, Lehigh University                                       *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           * 
 *                                                                           *
 * Copyright (C) 2001-2007, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include <typeinfo>

#include "CoinSort.hpp"

#include "AlpsKnowledge.h"

#include "KnapModel.h"

//#############################################################################
static double*  ratio;

// Compare function used in Quick Sort
int ratioGreater (const void * a, const void * b) {
    if(ratio[*((int*)a)] < ratio[*((int*)b)])
	return 1;
    else if (ratio[*((int*)a)] > ratio[*((int*)b)])
	return -1;
    return 0;
}

//#############################################################################

// Set the sequence of items in the knapsack
void KnapModel::setSequence(const int * seq) {
    int num = getNumItems();
    if (num > 0) {
	sequence_ = new int [num];
	for(int i = 0; i < num; ++i) {
	    sequence_[i] = *(seq+i);
	}
    }
}
    
//#############################################################################

void KnapModel::readInstance(const char* dataFile)
{
    setDataFile(dataFile);
    std::ifstream data_stream(getDataFile().c_str());

    if (!data_stream){
	std::cout << "Error opening input data file. Aborting.\n";
	abort();
    }

    std::string key;
    int value1, value2;

    // FIXME: There should be error checking on the file format. 
    while (data_stream >> key){
	if (key == "CAPACITY") {
	    data_stream >> value1;
	    setCapacity(value1);
	} else if  (key == "ITEM") {
	    data_stream >> value1;
	    data_stream >> value2;
	    addItem(value1, value2);
	}
    }

    //    std::cout << "Knapsack Capacity: " << getCapacity() << "\n";
    //    std::cout << "Number of Items:   " << getNumItems() << "\n\n";

    // Order the inputted items base on profit/capacity
    orderItems();
}

//#############################################################################

// Quick Sort items nondescently based on ratio, return the sequence. 
void sortRatio(int * sequence, double * ratio, int num) {
    for(int j = 0; j < num; j++) 
	sequence[j] = j;
    qsort(sequence, num, sizeof(*sequence), ratioGreater);
}

//#############################################################################

void 
KnapModel::orderItems()
{
    const int n     = items_.size();
    ratio           = new double [n];
    sequence_       = new int [n];

    for (int i = 0; i < n; ++i) {  // cost/size
	ratio[i] = static_cast<double>(items_[i].second) / static_cast<double>
	    (items_[i].first); 
    }  
 
    // CoinSort_2(ratio.begin(), ratio.end(), items_.begin());
    // CoinSort_2(&ratio[0], &ratio[0]+n, &items_[0]);

    sortRatio(sequence_, ratio, n);

    delete [] ratio;
}

//#############################################################################

AlpsEncoded* 
KnapModel::encode() const 
{ 
    AlpsReturnCode status = ALPS_OK;
    //  AlpsEncoded* encoded = new AlpsEncoded(typeid(*this).name());
    AlpsEncoded* encoded = new AlpsEncoded("ALPS_MODEL");

    //------------------------------------------------------
    // Encode Alps part. 
    //------------------------------------------------------

    status = encodeAlps(encoded);
    
    //------------------------------------------------------
    // Encode Knap part. 
    //------------------------------------------------------

    const int size = items_.size();
    int* weight; 
    int* profit;
    int i;

    if (size > 0) {
	weight = new int[size];
	profit = new int[size];
	for(i = 0; i < size; ++i) {
	    weight[i] = items_[i].first;
	    profit[i] = items_[i].second;
	}
    }
    else { // size == 0
	weight = 0; profit = 0;
    }

    // Write the model data into representation_
    encoded->writeRep(size);
    encoded->writeRep(capacity_);
    encoded->writeRep(weight, size);
    encoded->writeRep(profit, size);
    encoded->writeRep(sequence_, size);

    return encoded;
}

//#############################################################################

void
KnapModel::decodeToSelf(AlpsEncoded& encoded)
{ 
    int size, cap, i;
    std::vector<std::pair<int, int> > items;
    int* weight;  // By default of readRep, don't need allocate memory
    int* profit;
    int* seq;
    
    AlpsReturnCode status = ALPS_OK;

    //------------------------------------------------------
    // Decode Alps part. 
    // NOTE: Nothing to do for Bcps part.
    //------------------------------------------------------

    status = decodeAlps(encoded);

    //------------------------------------------------------
    // Decode Blis part. 
    //------------------------------------------------------

    encoded.readRep(size).readRep(cap);
    encoded.readRep(weight, size).readRep(profit, size).readRep(seq, size);
    items.reserve(size);
    for(i = 0; i < size; ++i)
	items.push_back(std::make_pair(weight[i], profit[i]));

#if defined NF_DEBUG_MORE
    std::cout << "\nMODEL: decode: Knapsack Capacity: " << cap << "\n";
    std::cout << "decode: Number of Items:   " << size << "\n";
    std::cout <<"Weight: " <<std::endl;
    for (i = 0; i < size; ++i) {
	std::cout << weight[i] << " ";
    }
    std::cout <<"\nProfit: " <<std::endl;
    for (i = 0; i < size; ++i) {
	std::cout << profit[i] << " ";
    }
    std::cout << "\nEND OF DECODE"<<  std::endl <<  std::endl;;
#endif

    capacity_ = cap;
    items_.insert(items_.begin(), items.begin(), items.end());
    sequence_ = seq;
    seq = NULL;    
}

//#############################################################################
#if 0
AlpsKnowledge*
KnapModel::decode(AlpsEncoded& encoded) const 
{ 
    int size, cap, i;
    std::vector<std::pair<int, int> > items;
    int* weight;  // By default of readRep, don't need allocate memory
    int* profit;
    int* seq;

    encoded.readRep(size).readRep(cap);
    encoded.readRep(weight, size).readRep(profit, size).readRep(seq, size);
    items.reserve(size);
    for(i = 0; i < size; ++i)
	items.push_back(std::make_pair(weight[i], profit[i]));

#if defined NF_DEBUG_MORE
    std::cout << "\nMODEL: decode: Knapsack Capacity: " << cap << "\n";
    std::cout << "decode: Number of Items:   " << size << "\n";
    std::cout <<"Weight: " <<std::endl;
    for (i = 0; i < size; ++i) {
	std::cout << weight[i] << " ";
    }
    std::cout <<"\nProfit: " <<std::endl;
    for (i = 0; i < size; ++i) {
	std::cout << profit[i] << " ";
    }
    std::cout << "\nEND OF DECODE"<<  std::endl <<  std::endl;;
#endif

    return new KnapModel(cap, items, seq); 
}
#endif
//#############################################################################
