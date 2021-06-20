# Knapsack problem 
# minimize 

set Item;

param Size {j in Item};
param Profit {j in Item};
param capacity;

var X {j in Item} binary;

minimize Total_profit: sum {j in Item} -Profit[j] * X[j];

subject to Capacity_limit: sum{j in Item} Size[j] * X[j] >= capacity;
