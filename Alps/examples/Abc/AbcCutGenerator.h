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
 * Copyright (C) 2001-2011, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

//#############################################################################
// This file is modified from SbbCutGenerator.hpp
//#############################################################################

#ifndef AbcCutGenerator_h_
#define AbcCutGenerator_h_

#include "OsiSolverInterface.hpp"
#include "OsiCuts.hpp"

class AbcModel;
class OsiRowCut;
class OsiRowCutDebugger;
class CglCutGenerator;

//#############################################################################

/** Interface between Abc and Cut Generation Library.
   
    \c AbcCutGenerator is intended to provide an intelligent interface between
    Abc and the cutting plane algorithms in the CGL. A \c AbcCutGenerator is
    bound to a \c CglCutGenerator and to an \c AbcModel. It contains parameters
    which control when and how the \c generateCuts method of the
    \c CglCutGenerator will be called.

    The builtin decision criteria available to use when deciding whether to
    generate cuts are limited: every <i>X</i> nodes, when a solution is found,
    and when a subproblem is found to be infeasible. The idea is that the class
    will grow more intelligent with time.
    
    \todo Add a pointer to function member which will allow a client to install
        their own decision algorithm to decide whether or not to call the CGL
        \p generateCuts method. Create a default decision method that looks
	at the builtin criteria.
  
    \todo It strikes me as not good that generateCuts contains code specific to
	individual CGL algorithms. Another set of pointer to function members,
	so that the client can specify the cut generation method as well as
	pre- and post-generation methods? Taken a bit further, should this
	class contain a bunch of pointer to function members, one for each
	of the places where the cut generator might be referenced?
	Initialization, root node, search tree node, discovery of solution,
	and termination all come to mind. Initialization and termination would
	also be useful for instrumenting sbb.
*/

class AbcCutGenerator  {
    
 public:
    
    /** \name Generate Cuts */
    //@{
    /** Generate cuts for the client model.

	Evaluate the state of the client model and decide whether to 
	generate cuts. The generated cuts are inserted into and returned 
	in the collection of cuts \p cs.
	
	If \p fullScan is true, the generator is obliged to call the CGL
	\c generateCuts routine.  Otherwise, it is free to make a local 
	decision. The current implementation uses \c whenCutGenerator_ 
	to decide.

	The routine returns true if reoptimisation is needed (because the 
	state of the solver interface has been modified).
    */
    bool generateCuts( OsiCuts &cs, bool fullScan); 
    //@}

    
    /**@name Constructors and destructors */
    //@{
    /// Default constructor 
    AbcCutGenerator (); 
    
    /// Normal constructor
    AbcCutGenerator(AbcModel * model,CglCutGenerator * generator,
		    int howOften=1, const char * name=NULL,
		    bool normal=true, bool atSolution=false, 
		    bool infeasible=false);
    
    /// Copy constructor 
    AbcCutGenerator (const AbcCutGenerator &);
    
    /// Assignment operator 
    AbcCutGenerator & operator=(const AbcCutGenerator& rhs);
    
    /// Destructor 
    ~AbcCutGenerator ();
    //@}

    /**@name Gets and sets */
    //@{
    /** Set the client model.
	
	In addition to setting the client model, refreshModel also calls
	the \c refreshSolver method of the CglCutGenerator object.
    */
    void refreshModel(AbcModel * model);
    
    /// return name of generator
    inline const char * cutGeneratorName() const
	{ 
	    return generatorName_; 
	}
    
    /** Set the cut generation interval
	
	Set the number of nodes evaluated between calls to the Cgl object's
	\p generateCuts routine.
	
	If \p value is positive, cuts will always be generated at the specified
	interval.
	If \p value is negative, cuts will initially be generated at the 
	specified interval, but Abc may adjust the value depending on the 
	success of cuts produced by this generator.

	A value of -100 disables the generator, while a value of -99 means
	just at root.
    */
    void setHowOften(int value) ;
    
    /// Get the cut generation interval.
    inline int howOften() const
	{ return whenCutGenerator_; }
    
    /// Get whether the cut generator should be called in the normal place
    inline bool normal() const
	{ return normal_; }
    /// Set whether the cut generator should be called in the normal place
    inline void setNormal(bool value) 
	{ normal_=value; }
    /// Get whether the cut generator should be called when a solution is found
    inline bool atSolution() const
	{ return atSolution_; }
    /// Set whether the cut generator should be called when a solution is found
    inline void setAtSolution(bool value) 
	{ atSolution_=value; }
    /** Get whether the cut generator should be called when the subproblem is
	found to be infeasible.
    */
    inline bool whenInfeasible() const
	{ return whenInfeasible_; }
    /** Set whether the cut generator should be called when the subproblem is
	found to be infeasible.
    */
    inline void setWhenInfeasible(bool value) 
	{ whenInfeasible_=value; }
    /// Get the \c CglCutGenerator bound to this \c AbcCutGenerator.
    inline CglCutGenerator * generator() const
	{ return generator_; }
    //@}
  
 private:
    /// The client model
    AbcModel *model_;

    // The CglCutGenerator object
    CglCutGenerator * generator_;

    /** Number of nodes between calls to the CglCutGenerator::generateCuts
	routine.
    */
    int whenCutGenerator_;
    
    /// Name of generator
    char * generatorName_;
    
    /// Whether to call the generator in the normal place
    bool normal_;

    /// Whether to call the generator when a new solution is found
    bool atSolution_;

    /// Whether to call generator when a subproblem is found to be infeasible
    bool whenInfeasible_;  
};

#endif
