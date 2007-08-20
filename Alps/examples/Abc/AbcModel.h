/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors:                                                                  *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Ted Ralphs, Lehigh University                                    *
 *                                                                           *
 * Conceptual Design:                                                        *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           * 
 *                                                                           *
 * Copyright (C) 2001-2007, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#ifndef AbcModel_h_
#define AbcModel_h_

//#############################################################################
// This file is modified from SbbModel.hpp
//#############################################################################

#include <cmath>

#include "CoinMessageHandler.hpp"
#include "CoinWarmStartBasis.hpp"
#include "OsiCuts.hpp"
#include "OsiSolverInterface.hpp"

#include "AbcBranchActual.h"
#include "AbcCutGenerator.h"
#include "AbcHeuristic.h"
#include "AbcMessage.h"
#include "AlpsModel.h"

#include "AbcParams.h"

class CglCutGenerator;

class AbcBranchDecision;
class AlpsTreeNode;
class AbcNodeDesc;
class AbcTreeNode;

//#############################################################################

/** Model class for ALPS Branch and Cut. */
class AbcModel : public AlpsModel {
    
 public:
    enum AbcIntParam {
	/** The maximum number of nodes before terminating */
	AbcMaxNumNode=0,
	/** The maximum number of solutions before terminating */
	AbcMaxNumSol,
	/** Fathoming discipline
	    Controls objective function comparisons for purposes of 
	    fathoming by bound or determining monotonic variables.
	    If 1, action is taken only when the current objective is
	    strictly worse than the target. Implementation is handled 
	    by adding a small tolerance to the target.
	*/
	AbcFathomDiscipline,
	/** Just a marker, so that a static sized array can store parameters.*/
	AbcLastIntParam
    };

    enum AbcDblParam {
	/** The maximum amount the value of an integer variable can vary from
	    integer and still be considered feasible. */
	AbcIntegerTolerance = 0,
	/** The objective is assumed to worsen by this amount for each
	    integer infeasibility. */
	AbcInfeasibilityWeight,
	/** The amount by which to tighten the objective function cutoff when
	    a new solution is discovered. */
	AbcCutoffIncrement,
	/** Stop when the gap between the objective value of the best known 
	    solution and the best bound on the objective of any solution 
	    is less than this.
	    This is an absolute value. Conversion from a percentage is left 
	    to the client.
	*/
	AbcAllowableGap,
	/** \brief The maximum number of seconds before terminating.
	    A double should be adequate! */
	AbcMaximumSeconds,
	/** Just a marker, so that a static sized array can store parameters.*/
	AbcLastDblParam
    };
    
 private:
    /**@name Shared problem data
     */
    //@{
    ///
    /// Number of rows at continuous
    int numberRowsAtContinuous_;
    /// Number of integers in problem
    int numberIntegers_;
    /// Indices of integer variables
    int* integerVariable_;
    /** Pointer to a warm start basis.  */
    CoinWarmStartBasis* sharedBasis_;
    //@}

    /// Message handler
    CoinMessageHandler * handler_;

    /** Flag to say if handler_ is the default handler.
	The default handler is deleted when the model is deleted. Other
	handlers (supplied by the client) will not be deleted.
    */
    bool defaultHandler_;

    /// Abc messages
    CoinMessages messages_;

    /// Array for integer parameters
    int intParam_[AbcLastIntParam];

    /// Array for double parameters
    double dblParam_[AbcLastDblParam];

    /** The solver associated with this model. */
    OsiSolverInterface* solver_;
    
    /** Ownership of the solver object
	The convention is that AbcModel owns the null solver. Currently there
	is no public method to give AbcModel a solver without giving ownership,
	but the hook is here. */
    bool ourSolver_;

    /// A copy of the solver, taken at the continuous (root) node.
    OsiSolverInterface * continuousSolver_;
    
    /** Pointer to a warm start basis.  */
    CoinWarmStartBasis* basis_;

    /** Pointer to last warm basis.  */
    CoinWarmStartBasis* lastws_;

    /// Minimum degradation in objective value to continue cut generation
    double minimumDrop_;

    /// Best objective
    double bestObjective_;
    
    /// Array holding the incumbent (best) solution.
    double * bestSolution_;

    /** Array holding the current solution.
	This array is used more as a temporary.
    */
    double * currentSolution_;

    /// Global cuts
    OsiCuts globalCuts_;
    
    /// Cumulative number of nodes
    int numberNodes_;
    /// Cumulative number of iterations
    int numberIterations_;
    /// Status of problem - 0 finished, 1 stopped, 2 difficulties
    int status_;
    /// Number of entries in #addedCuts_
    int currentNumberCuts_;
    /// Maximum number of cuts
    int maximumNumberCuts_;
    /// How often to scan global cuts
    int howOftenGlobalScan_;

    /** Current limit on search tree depth

	The allocated size of #walkback_. Increased as needed.
    */
    int maximumDepth_;

    /** Maximum number of candidates to consider for strong branching.
	To disable storng branching and use pseudocost branching, 
	set this to 0.
    */
    int numberStrong_;
    /// Number of cut generators
    int numberCutGenerators_;
    // Cut generators
    AbcCutGenerator ** generator_;
    /// Number of heuristics
    int numberHeuristics_;
    // Heuristic solvers
    AbcHeuristic ** heuristic_;

    /// Maximum number of cut passes at root
    int maximumCutPassesAtRoot_;
    /// Maximum number of cut passes
    int maximumCutPasses_;

    /// Variable selection function
    AbcBranchDecision * branchingMethod_;
    /// Number of solutions
    int numberSolutions_;  
    /// Number of heuristic solutions
    int numberHeuristicSolutions_;
    /// Priorities
    int * priority_;
    ///
    AbcPseudocost **pseudoList_;
    ///
    int *pseudoIndices_;

    /** Abc parameters. */
    AbcParams *AbcPar_;
    
 public:
    AbcModel() 
	{
	    init();
	}
    
    AbcModel(const OsiSolverInterface &rhs)
	{   
	    init();
	    solver_ = rhs.clone();
	    ourSolver_ = true ;
	    continuousSolver_ = 0;
	    int numberColumns = solver_->getNumCols();
	    int iColumn;
	    if (numberColumns) {
		// Space for current solution
		currentSolution_ = new double[numberColumns];
		for (iColumn = 0; iColumn < numberColumns; ++iColumn) {
		    if( solver_->isInteger(iColumn)) 
			numberIntegers_++;
		}
	    } else {
		// empty model
		currentSolution_=NULL;
	    }
	    if (numberIntegers_) {
		integerVariable_ = new int [numberIntegers_];
		numberIntegers_=0;
		for (iColumn=0;iColumn<numberColumns;iColumn++) {
		    if( solver_->isInteger(iColumn)) 
			integerVariable_[numberIntegers_++]=iColumn;
		}
	    } else {
		integerVariable_ = NULL;
	    }
	}
   
    ~AbcModel() 
	{
	    if ( handler_ != 0){
		delete handler_;
		handler_ = 0;
	    }
	    if (priority_ != 0) {
		delete [] priority_;
		priority_ = 0;
	    }
	    if (currentSolution_ != 0) {
		delete [] currentSolution_;
		currentSolution_ = 0;
	    }
	    if (bestSolution_ != 0) {
		delete [] bestSolution_;
		bestSolution_ = 0;
	    }
	    if (generator_ != 0) {
		for (int i = 0; i < numberCutGenerators_; ++i)
		    delete generator_[i];
		delete [] generator_;
		generator_ = 0;
	    }
	    if (heuristic_ != 0) {
		//for (int i = 0; i < numberHeuristics_; ++i) {
		//  if (heuristic_[i] != 0) {
		//delete heuristic_[i];
		//heuristic_[i] = 0;
		//  }
		//}
		delete [] heuristic_;
		heuristic_ = 0;
	    }

	    if (integerVariable_ != 0) {
		delete [] integerVariable_;
		integerVariable_ = 0;
	    }
	    if (sharedBasis_ != 0) {
		delete sharedBasis_;
		sharedBasis_ = 0;
	    }
	    if (basis_ != 0) {
		delete basis_;
		basis_ = 0;
	    }
	    if (pseudoList_ != NULL) {
                int i = getNumCols() - 1;
		for (; i >= 0; --i) {
                    //printf("i = %d\n", i);
		    delete pseudoList_[i];
		}
		delete [] pseudoList_;
	    }
	    if (pseudoIndices_ != NULL) {
		delete [] pseudoIndices_;
	    }
	    /* Last thing is to delete solver */
	    if (ourSolver_) {
		delete solver_ ;
		solver_ = 0;
	    }
	    if (continuousSolver_ != 0) {
		delete continuousSolver_ ;
		continuousSolver_ = 0;
	    }
	    delete AbcPar_;
	}
    
    /** Initialize member data */
    void init()
	{
	    numberRowsAtContinuous_ = 0;
	    numberIntegers_ = 0;
	    integerVariable_ = NULL;
	    sharedBasis_ = NULL;
	    handler_ = new CoinMessageHandler();
	    handler_->setLogLevel(2);
	    defaultHandler_ = true;
	    messages_ = AbcMessage();
	    solver_ = NULL;
	    ourSolver_ = false;
	    basis_ = 0;
	    minimumDrop_ = 1.0e-4;
	    bestObjective_ = 1.0e100;
	    bestSolution_ = 0;
	    currentSolution_ = 0;
	    numberNodes_ = 0;
	    numberIterations_ = 0;
	    status_ = 0;
	    currentNumberCuts_ = 0;
	    maximumNumberCuts_ = 1000;
	    howOftenGlobalScan_ = 1;
	    numberStrong_ = 0;
	    numberCutGenerators_ =0;
	    generator_ = NULL;
	    numberHeuristics_ = 0;
	    heuristic_ = NULL;
	    maximumCutPassesAtRoot_ = 20;
	    maximumCutPasses_ = 10;
	    branchingMethod_ = NULL;
	    numberSolutions_ = 0;
	    numberHeuristicSolutions_ = 0;
	    priority_ = NULL;
	    pseudoList_ = NULL;
	    pseudoIndices_ = NULL;
	    
	    continuousSolver_ = 0;

	    // Set values for parameters
	    intParam_[AbcMaxNumNode] = 9999999;
	    intParam_[AbcMaxNumSol] = 9999999;
	    intParam_[AbcFathomDiscipline] = 0;
	    
	    dblParam_[AbcIntegerTolerance] = 1e-6;
	    dblParam_[AbcInfeasibilityWeight] = 0.0;
	    dblParam_[AbcCutoffIncrement] = 1e-5;
	    dblParam_[AbcAllowableGap] = 1.0e-10;
	    dblParam_[AbcMaximumSeconds] = 1.0e100;
	    AbcPar_ = new AbcParams;
	}
    
    /** Read in the problem data */
    virtual void readInstance(const char* dataFile) 
	{
	    solver()->readMps(dataFile, "");
	}

    /** Read in Alps and Abc parameters. */
    void readParameters(const int argnum, const char * const * arglist) {
	std::cout << "Reading in ALPS parameters ..." << std::endl;
        AlpsPar_->readFromArglist(argnum, arglist);
	std::cout << "Reading in ABC parameters ..." << std::endl;
        AbcPar_->readFromArglist(argnum, arglist);
    } 
    
    AbcParams *AbcPar() { return AbcPar_; }
    
    /// Returns solver - has current state
    OsiSolverInterface * solver() const
	{ return solver_; }
    
    /** Assign a solver to the model (model assumes ownership)
	On return, \p solver will be NULL.
	\note Parameter settings in the outgoing solver are not inherited by
	the incoming solver. */
    void assignSolver(OsiSolverInterface *&solver);

    /** Return an empty basis object of the specified size
	A useful utility when constructing a basis for a subproblem 
	from scratch. The object returned will be of the requested 
	capacity and appropriate for the solver attached to the model. */
    CoinWarmStartBasis *getEmptyBasis(int ns = 0, int na = 0) const;

    /// Number of integers in problem
    inline int numberIntegers() const
	{ return numberIntegers_; }
    
    /// Integer variables
    inline const int * integerVariable() const 
	{ return integerVariable_; }
    
    //-------------------------------------------------------------------------
    ///@name Solve methods 
    //@{
    /** \brief Solve the initial LP relaxation
      Invoke the solver's %initialSolve() method.
    */
    void initialSolve(); 

    /** \brief Evaluate a subproblem using cutting planes and heuristics
      The method invokes a main loop which generates cuts, applies heuristics,
      and reoptimises using the solver's native %resolve() method.
      It returns true if the subproblem remains feasible at the end of the
      evaluation.
    */
    bool solveWithCuts( OsiCuts & cuts, int numberTries, 
			AbcTreeNode * node, int & numberOldActiveCuts, 
			int & numberNewCuts, int & maximumWhich, 
			int *& whichGenerator, const bool cutDuringRampup,
			int & found );

    /** \brief Reoptimise an LP relaxation
      Invoke the solver's %resolve() method.
    */
    bool resolve();
    //@}
    
    //-------------------------------------------------------------------------
    ///@name Methods returning info on how the solution process terminated
    //@{
    /// Are there a numerical difficulties?
    bool isAbandoned() const;
    /// Is optimality proven?
    bool isProvenOptimal() const;
    /// Is  infeasiblity proven (or none better than cutoff)?
    bool isProvenInfeasible() const;
    /// Node limit reached?
    bool isNodeLimitReached() const;
    /// Solution limit reached?
    bool isSolutionLimitReached() const;
    /// Get how many iterations it took to solve the problem.
    int getIterationCount() const
	{ return solver_->getIterationCount(); }
    /// Get how many Nodes it took to solve the problem.
    int getNodeCount() const
	{ return numberNodes_; }
    /// Increment the count of nodes
    void incrementNodeCount(int s = 1) 
	{ numberNodes_ += s; }
    
    /** Final status of problem
      0 finished, 1 stopped, 2 difficulties
    */
    inline int status() const
    { return status_; }
    //@}

    //-------------------------------------------------------------------------
    /**@name Problem information methods 
       
       These methods call the solver's query routines to return
       information about the problem referred to by the current object.
       Querying a problem that has no data associated with it result in
       zeros for the number of rows and columns, and NULL pointers from
       the methods that return vectors.
       
       Const pointers returned from any data-query method are valid as
       long as the data is unchanged and the solver is not called.
    */
    //@{
    /// Number of rows in continuous (root) problem.
    int numberRowsAtContinuous() const
	{ return numberRowsAtContinuous_; }

    void setNumberRowsAtContinous(const int value)
	{
	    numberRowsAtContinuous_ = value;
	}
    
    
    /// Get number of columns
    int getNumCols() const
	{ return solver_->getNumCols(); }
    
    /// Get number of rows
    int getNumRows() const
	{ return solver_->getNumRows(); }
    
    /// Get number of nonzero elements
    int getNumElements() const
	{ return solver_->getNumElements(); }
    
    /// Get pointer to array[getNumCols()] of column lower bounds
    const double * getColLower() const
	{ return solver_->getColLower(); }
  
    /// Get pointer to array[getNumCols()] of column upper bounds
    const double * getColUpper() const
	{ return solver_->getColUpper(); }
    
    /** Get pointer to array[getNumRows()] of row constraint senses.
	<ul>
	<li>'L': <= constraint
	<li>'E': =  constraint
	<li>'G': >= constraint
	<li>'R': ranged constraint
	<li>'N': free constraint
	</ul>
    */
    const char * getRowSense() const
	{ return solver_->getRowSense(); }
    
    /** Get pointer to array[getNumRows()] of rows right-hand sides
	<ul>
	<li> if rowsense()[i] == 'L' then rhs()[i] == rowupper()[i]
	<li> if rowsense()[i] == 'G' then rhs()[i] == rowlower()[i]
	<li> if rowsense()[i] == 'R' then rhs()[i] == rowupper()[i]
	<li> if rowsense()[i] == 'N' then rhs()[i] == 0.0
	</ul>
    */
    const double * getRightHandSide() const
	{ return solver_->getRightHandSide(); }
  
    /** Get pointer to array[getNumRows()] of row ranges.
	<ul>
	<li> if rowsense()[i] == 'R' then
	rowrange()[i] == rowupper()[i] - rowlower()[i]
	<li> if rowsense()[i] != 'R' then
	rowrange()[i] is 0.0
	</ul>
    */
    const double * getRowRange() const
	{ return solver_->getRowRange(); }
    
    /// Get pointer to array[getNumRows()] of row lower bounds
    const double * getRowLower() const
	{ return solver_->getRowLower(); }
    
    /// Get pointer to array[getNumRows()] of row upper bounds
    const double * getRowUpper() const
	{ return solver_->getRowUpper(); }
    
    /// Get pointer to array[getNumCols()] of objective function coefficients
    const double * getObjCoefficients() const
	{ return solver_->getObjCoefficients(); }
  
    /// Get objective function sense (1 for min (default), -1 for max)
    double getObjSense() const
	{ return solver_->getObjSense(); }
    ///
    AbcPseudocost ** getPseudoList() { return pseudoList_; }
    
    ///
    int *getPseudoIndices() { return pseudoIndices_; }
    
    /// Return true if variable is continuous
    bool isContinuous(int colIndex) const
	{ return solver_->isContinuous(colIndex); }
    
    /// Return true if variable is binary
    bool isBinary(int colIndex) const
	{ return solver_->isBinary(colIndex); }
  
    /** Return true if column is integer.
	Note: This function returns true if the the column
	is binary or a general integer.
    */
    bool isInteger(int colIndex) const
	{ return solver_->isInteger(colIndex); }
    
    /// Return true if variable is general integer
    bool isIntegerNonBinary(int colIndex) const
	{ return solver_->isIntegerNonBinary(colIndex); }
  
    /// Return true if variable is binary and not fixed at either bound
    bool isFreeBinary(int colIndex) const
	{ return solver_->isFreeBinary(colIndex); }
  
    /// Get pointer to row-wise copy of matrix
    const CoinPackedMatrix * getMatrixByRow() const
	{ return solver_->getMatrixByRow(); }
    
    /// Get pointer to column-wise copy of matrix
    const CoinPackedMatrix * getMatrixByCol() const
	{ return solver_->getMatrixByCol(); }
    
    /// Get solver's value for infinity
    double getInfinity() const
	{ return solver_->getInfinity(); }
    //@}
    
    /**@name Methods related to querying the solution */
    //@{
    /** Call this to really test if a valid solution can be feasible
	Solution is number columns in size.
	If fixVariables true then bounds of continuous solver updated.
	Returns objective value (worse than cutoff if not feasible)
    */
    double checkSolution(double cutoff, 
			 const double * solution,
			 bool fixVariables);
    
    /// Record a new incumbent solution and update objectiveValue
    bool setBestSolution(ABC_Message how,
			 double & objectiveValue, 
			 const double *solution,
			 bool fixVariables = false);

    /** Test the current solution for feasiblility.	
	Scan all objects for indications of infeasibility. This is broken down
	into simple integer infeasibility (\p numberIntegerInfeasibilities)
	and all other reports of infeasibility (\pnumberObjectInfeasibilities).
    */
    bool feasibleSolution(int & numberIntegerInfeasibilities);

    /** Solution to the most recent lp relaxation.
	
	The solver's solution to the most recent lp relaxation.
    */
    inline double * currentSolution() const
	{ return currentSolution_; }

    /// Get pointer to array[getNumCols()] of primal solution vector
    const double * getColSolution() const
	{ return solver_->getColSolution(); }
  
    /// Get pointer to array[getNumRows()] of dual prices
    const double * getRowPrice() const
	{ return solver_->getRowPrice(); }
  
    /// Get a pointer to array[getNumCols()] of reduced costs
    const double * getReducedCost() const
	{ return solver_->getReducedCost(); }
  
    /// Get pointer to array[getNumRows()] of row activity levels.
    const double * getRowActivity() const
	{ return solver_->getRowActivity(); }
  
    /// Get current objective function value
    double getCurrentObjValue() const
	{ return solver_->getObjValue(); }

    /// Get best objective function value
    double getObjValue() const
	{ return bestObjective_; }

    /** Set the best objective value. It is not necessary the value from
	the bestSolution_. It can be get from other processes. */
    void setObjValue(double obj) 
	{ bestObjective_ = obj; }
  
    /** The best solution to the integer programming problem.

	The best solution to the integer programming problem found during
	the search. If no solution is found, the method returns null.
    */
    const double * bestSolution() const
	{ return bestSolution_; }
  
    /// Get number of solutions
    int getSolutionCount() const
	{ return numberSolutions_; }
  
    /// Set number of solutions (so heuristics will be different)
    void setSolutionCount(int value) 
	{ numberSolutions_=value; }

    /// Get number of heuristic solutions
    int getNumberHeuristicSolutions() const 
	{ return numberHeuristicSolutions_; }

    /// Set objective function sense (1 for min (default), -1 for max,)
    void setObjSense(double s) { solver_->setObjSense(s); }

    /** Set the maximum number of cut passes at root node (default 20)
	Minimum drop can also be used for fine tuning */
    inline void setMaximumCutPassesAtRoot(int value)
	{maximumCutPassesAtRoot_ = value; }
    /** Get the maximum number of cut passes at root node */
    inline int getMaximumCutPassesAtRoot() const
	{ return maximumCutPassesAtRoot_; }
    
    /** Set the maximum number of cut passes at other nodes (default 10)
	Minimum drop can also be used for fine tuning */
    inline void setMaximumCutPasses(int value)
	{ maximumCutPasses_ = value; }
    /** Get the maximum number of cut passes at other nodes (default 10) */
    inline int getMaximumCutPasses() const
	{ return maximumCutPasses_; }
    /// Number of entries in the list returned by #addedCuts()
    int currentNumberCuts() const
	{ return currentNumberCuts_; }
    void setCurrentNumberCuts(int value) 
	{
	    currentNumberCuts_ += value;
	}
    
    //@}

    //-------------------------------------------------------------------------

    /** \name Branching Decisions
	
	See the AbcBranchDecision class for additional information.
    */
    //@{
    /// Get the current branching decision method.
    inline AbcBranchDecision * branchingMethod() const
	{ return branchingMethod_; }
    /// Set the branching decision method.
    inline void setBranchingMethod(AbcBranchDecision * method)
	{ branchingMethod_ = method; }
    /** Set the branching method
	\overload
    */
    inline void setBranchingMethod(AbcBranchDecision & method)
	{ branchingMethod_ = &method; }
    //@}

    //-------------------------------------------------------------------------

    /**@name Message handling */
    //@{
    /// Pass in Message handler (not deleted at end)
    void passInMessageHandler(CoinMessageHandler * handler)
	{
	    if (defaultHandler_) {
		delete handler_;
		handler_ = NULL;
	    }
	    defaultHandler_ = false;
	    handler_ = handler;
	}
    
    /// Set language
    void newLanguage(CoinMessages::Language language) 
	{ messages_ = AbcMessage(language); }
    void setLanguage(CoinMessages::Language language)
	{ newLanguage(language); }
    /// Return handler
    CoinMessageHandler * messageHandler() const
	{ return handler_; }
    /// Return messages
    CoinMessages messages() 
	{ return messages_; }
    /// Return pointer to messages
    CoinMessages * messagesPointer() 
	{ return &messages_; }
    //@}

    //-------------------------------------------------------------------------

    bool checkInteger(double value) const
	{
	    double integerTolerance =
		getDblParam(AbcModel::AbcIntegerTolerance);
	    double nearest = floor(value + 0.5);
	    if (fabs(value - nearest) <= integerTolerance) 
		return true;
	    else 
		return false;
	}

    /** \brief Identify integer variables and create corresponding objects.
	
	Record integer variables and create an integer object for each one.
	If \p startAgain is true, a new scan is forced, overwriting any 
	existing integer variable information.
    */
    void findIntegers(bool startAgain);

  /** Add one generator - up to user to delete generators.
      howoften affects how generator is used. 0 or 1 means always,
      >1 means every that number of nodes.  Negative values have same
      meaning as positive but they may be switched off (-> -100) by code if
      not many cuts generated at continuous.  -99 is just done at root.
      Name is just for printout
  */
    void addCutGenerator(CglCutGenerator * generator,
			 int howOften=1, const char * name=NULL,
			 bool normal=true, bool atSolution=false, 
			 bool infeasible=false);
    /** \name Heuristics and priorities */
    //@{
    /// Add one heuristic
    void addHeuristic(AbcHeuristic * generator);
    //@}

    /** Perform reduced cost fixing
	Fixes integer variables at their current value based on reduced cost
	penalties.
    */
    void reducedCostFix() ;

    /** Remove inactive cuts from the model
	
	An OsiSolverInterface is expected to maintain a valid basis, but not a
	valid solution, when loose cuts are deleted. Restoring a valid solution
	requires calling the solver to reoptimise. If it's certain the solution
	will not be required, set allowResolve to false to suppress
	reoptimisation.
    */
    //void takeOffCuts(OsiCuts &cuts, int *whichGenerator,
//		     int &numberOldActiveCuts, int &numberNewCuts,
    //	     bool allowResolve);
    void takeOffCuts();

    /// Set an integer parameter
    inline bool setIntParam(AbcIntParam key, int value) {
	intParam_[key] = value;
	return true;
    }

    /// Set a double parameter
    inline bool setDblParam(AbcDblParam key, double value) {
	dblParam_[key] = value;
	return true;
    }

    /// Get an integer parameter
    inline int getIntParam(AbcIntParam key) const {
	return intParam_[key];
    }

    /// Get a double parameter
    inline double getDblParam(AbcDblParam key) const {
	return dblParam_[key];
    }

    /*! \brief Set cutoff bound on the objective function.
      When using strict comparison, the bound is adjusted by a tolerance to
      avoid accidentally cutting off the optimal solution.
    */
    void setCutoff(double value);

    /// Get the cutoff bound on the objective function - always as minimize
    inline double getCutoff() const
	{ 
	    double value ;
	    solver_->getDblParam(OsiDualObjectiveLimit, value) ;
	    return value * solver_->getObjSense() ; 
	}

    /// Set the \link AbcModel::AbcMaxNumNode maximum node limit \endlink
    inline bool setMaximumNodes( int value)
	{ return setIntParam(AbcMaxNumNode,value); }

    /// Get the \link AbcModel::AbcMaxNumNode maximum node limit \endlink
    inline int getMaximumNodes() const
	{ return getIntParam(AbcMaxNumNode); }

    /** Set the
	\link AbcModel::AbcMaxNumSol maximum number of solutions \endlink
	desired.
    */
    inline bool setMaximumSolutions( int value) {
	return setIntParam(AbcMaxNumSol,value);
    }
    /** Get the
	\link AbcModel::AbcMaxNumSol maximum number of solutions \endlink
	desired.
    */
    inline int getMaximumSolutions() const {
	return getIntParam(AbcMaxNumSol);
    }
    
    /** Set the
	\link AbcModel::AbcIntegerTolerance integrality tolerance \endlink
    */
    inline bool setIntegerTolerance( double value) {
	return setDblParam(AbcIntegerTolerance,value);
    }
    /** Get the
	\link AbcModel::AbcIntegerTolerance integrality tolerance \endlink
    */
    inline double getIntegerTolerance() const {
	return getDblParam(AbcIntegerTolerance);
    }

    /** Set the
	\link AbcModel::AbcInfeasibilityWeight
	weight per integer infeasibility \endlink
    */
    inline bool setInfeasibilityWeight( double value) {
	return setDblParam(AbcInfeasibilityWeight,value);
    }
    /** Get the
	\link AbcModel::AbcInfeasibilityWeight
	weight per integer infeasibility \endlink
    */
    inline double getInfeasibilityWeight() const {
	return getDblParam(AbcInfeasibilityWeight);
    }
    
    /** Set the \link AbcModel::AbcAllowableGap allowable gap \endlink
	between the best known solution and the best possible solution.
    */
    inline bool setAllowableGap( double value) {
	return setDblParam(AbcAllowableGap,value);
    }
    /** Get the \link AbcModel::AbcAllowableGap allowable gap \endlink
	between the best known solution and the best possible solution.
    */
    inline double getAllowableGap() const {
	return getDblParam(AbcAllowableGap);
    }

    /// Set the minimum drop to continue cuts
    inline void setMinimumDrop(double value)
	{ minimumDrop_=value; }
    /// Get the minimum drop to continue cuts
    inline double getMinimumDrop() const
	{ return minimumDrop_; }

    //-------------------------------------------------------------------------

    /** Do necessary work to make model usable. Return success or not. */
    virtual bool setupSelf();

    int numberStrong() const
	{ return numberStrong_; }
    
    void setNumberStrong(int number)
	{
	    if (number<0)
		numberStrong_=0;
	    else
		numberStrong_=number;
	}

    /// Priorities
    inline const int * priority() const { return priority_;}

    /// Returns priority level for an object (or 1000 if no priorities exist)
    inline int priority(int sequence) const
	{ 
	    if (priority_)
		return priority_[sequence];
	    else
		return 1000;
	}
    
    /** The method that encodes the model into a encoded object. */
    virtual AlpsEncoded* encode() const;
  
    /** The method that decodes the model from a encoded object. */
    virtual void decodeToSelf(AlpsEncoded&);
    
};

//#############################################################################

#endif
