/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Eclipse Public License as part of the       *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors:                                                                  *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Aykut Bulut, Lehigh University                                   *
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
 * Copyright (C) 2001-2018, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#ifndef AlpsModel_h_
#define AlpsModel_h_

#include <string>

#include "CoinError.hpp"

#include "AlpsKnowledge.h"
#include "AlpsTreeNode.h"
#include "AlpsParams.h"

class AlpsKnowledgeBroker;

/*!

  #AlpsModel is a base class for user application problem data. For an
  optimization problem problem parameters, constraint matrix, objective
  coeefficients, variable bounds are problem data.

  User application is required to inherit this for its own model.

  #AlpsModel has three private data fields only, these are ::broker_,
   ::dataFile_ and ::AlpsPar_. Some important virtual functions defined in
   #AlpsModel are following.

   <ul>

   <li> readInstance(). User application sub-class is supposed to
     implement this function to read problems from given input files. For
     example, a MILP solver should implement this function to read mps
     files. See examples/Abc/AbcModel.cpp. In serial execution this function
     will be called by the constructor of knowledge broker. In parallel case it
     will be called by the knowledge broker of the master process only (see Alps
     \ref index "main page" for a brief explanation of Alps design).

     <li> setupSelf(). This function is supposed to set other related problem
     data after the problem is read by readInstance(). In parallel execution
     master process's knowledge broker will read the problem data from input
     file and send it to other processes. Other processes' knowledge broker
     will receive the problem data. All processes will call setupSelf() to set
     the other fields of model class. Other fields may incldue fileds that are
     created from the problem input, which may include user application types
     needed during the search process. In case of Abc example these are
     variables, constraints etc.

     <li> preprocess(). Alps knowledge brokers will call this function after
     setupSelf(). It is for prerpocessing the problem before Alps starts the
     tree search. A MILP solver would carry preseolve operations here. See Blis
     for an example.

     <li> postprocess(). Alps knowledge brokers will call this function after
     the search is terminated. In parallel execution, only knowledge broker of
     master process will call this. See Blis for an example.

     <li> createRoot(). This function should create the root node to start the
     search. In parallel execution only master process calls this function.

     <li> modelLog(). This function is called when the search is ended. In
     parallel execution, only knowledge broker of master process will call
     this. This function is supposed to print problem information/statistics
     etc. See Blis for an example.

     <li> nodeLog(). This function is called every time the node counts hits a
     user defined value in AlpsParams::intParams::nodeLogInterval. It is
     supposed to print log information related to search status. In parallel
     execution this function will be called by master process only.

     <li> fathomAllNodes(). If this function returns true, all nodes will be
     fathomed in the corresponding subtree. This means all of the nodes in
     serial code.

     <li> encode(). This function is supposed to pack the necessary data fields
     into an #AlpsEncoded object. In parallel execution this object will be
     sent/received between knowledge brokers. Once the data is received,
     knowledge brokers will call setupSelf() to set the rest of the fields of
     the user defined model sub-class. Processors other then master will get
     problem data through these AlpsEncoded objects received through the
     network (master gets problem data from readInstance()). In a sense this
     function should pack all the sub-class member fields into an AlpsEncoded
     object that readInstance() sets in the master processor.

     <li> decode(). This function should decode the user sub-class model fields
     into a user sub-class model object. Alps will call setupSelf() to set the
     rest of the fields. This is similar to decodeToSelf() except that it
     unpacks the data into a new user model object, not to this instance.  Read
     decodeToSelf() for mode information.

     <li> decodeToSelf(). This function is supposed to unpack the necessary
     data fields from an #AlpsEncoded object. In parallel execution this
     function will set necessary fields of the user defined model sub-class. In
     this sense this function is similar to readInstance(). readInstance() is
     called by the master process only and reads the necessary data from an
     input file, where this function is called by the other processors and
     supposed to unpack necessary data not from a file but from an AlpsEncoded
     object. After this function, Alps will call setupSelf() to set the rest of
     the fields of the user defined model sub-class. This function is similar
     to decode(), except that it unpacks the data into this, where decodes
     creates a new user model object and unpacks the data to it. Read decode()
     for more information.

   </ul>


 */

class AlpsModel : public AlpsKnowledge {

private:

  AlpsModel(const AlpsModel&);
  AlpsModel& operator=(const AlpsModel&);

protected:
  /** Data file. */
  std::string dataFile_;
  /** The parameter set that is used in Alps. */
  AlpsParams * AlpsPar_;

public:

  ///@name Constructor and Destructor
  //@{
  /** Default construtor. */
  AlpsModel() :
    AlpsKnowledge(AlpsKnowledgeTypeModel),
    AlpsPar_(new AlpsParams) { }
  /** Destructor. */
  virtual ~AlpsModel() { delete AlpsPar_; }
  //@}

  ///@name Get methods
  //@{
  /** Get the input file. */
  inline std::string getDataFile() const { return dataFile_; }
  /** Access Alps Parameters. */
  AlpsParams * AlpsPar() { return AlpsPar_; }
  //@}

  ///@name Set methods
  //@{
  /** Set the data file. */
  inline void setDataFile(std::string infile) { dataFile_ = infile; }
  //@}

  ///@name Virtual functions required to be implemented by sub-class.
  //@{
  /** Read in the instance data. At Alps level, nothing to do. */
  virtual void readInstance(const char* dateFile) {
    throw CoinError("readInstance() is not defined.", "readData",
                    "AlpsModel");
  }
  /** Read in Alps parameters. */
  virtual void readParameters(const int argnum, const char * const * arglist);
  /** Write out parameters. */
  void writeParameters(std::ostream& outstream) const;
  /** Do necessary work to make model ready for use, such as classify
      variable and constraint types.*/
  virtual bool setupSelf() { return true; }
  /** Preprocessing the model. */
  virtual void preprocess() {}
  /** Postprocessing results. */
  virtual void postprocess() {}
  /** Create the root node. Default: do nothing */
  virtual AlpsTreeNode * createRoot() = 0;
  /** Problem specific log. */
  virtual void modelLog() {}
  /** Node log. */
  virtual void nodeLog(AlpsTreeNode *node, bool force);
  /** Return true if all nodes on this process can be fathomed.*/
  virtual bool fathomAllNodes() { return false; }
  //@}

  ///@name Parallel execution related virtual functions
  //@{
  /// Get encode function defined in AlpsKnowledge.
  using AlpsKnowledge::encode;
  /// Pack AlpsPar_ into a given encode object.
  virtual AlpsReturnStatus encode(AlpsEncoded * encoded) const;
  /// Decode the given AlpsEncoded object into this.
  virtual AlpsReturnStatus decodeToSelf(AlpsEncoded & encoded);
  /** Register knowledge class. */
  virtual void registerKnowledge() { /* Default does nothing */ }
  /** Send generated knowledge */
  virtual void sendGeneratedKnowledge() { /* Default does nothing */ }
  /** Receive generated knowledge */
  virtual void receiveGeneratedKnowledge() { /* Default does nothing */ }
  /** Pack knowledge to be shared with others into an encoded object.
      Return NULL means that no knowledge can be shared. */
  virtual AlpsEncoded* packSharedKnowlege() {
    /* Default does nothing */
    AlpsEncoded* encoded = NULL;
    return encoded;
  }
  /** Unpack and store shared knowledge from an encoded object. */
  virtual void unpackSharedKnowledge(AlpsEncoded&)
  { /* Default does nothing */ }
  //@}
};
#endif
