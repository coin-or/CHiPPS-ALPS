/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors: Yan Xu, SAS Institute Inc.                                       *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           * 
 *                                                                           *
 * Copyright (C) 2001-2006, Lehigh University, Yan Xu, and Ted Ralphs.       *
 * Corporation, Lehigh University, Yan Xu, Ted Ralphs, Matthew Salzman and   *
 *===========================================================================*/

#ifndef AlpsKnowledgeBroker_h_
#define AlpsKnowledgeBroker_h_

#include <cmath>
#include <iosfwd>
#include <map>

#include "CoinMessageHandler.hpp"

#include "AlpsCompareActual.h"
#include "AlpsEnumProcessT.h"
#include "AlpsKnowledge.h"
#include "AlpsKnowledgePool.h"
#include "AlpsMessage.h"
#include "AlpsParams.h"
#include "AlpsSolutionPool.h"
#include "AlpsSubTree.h"
#include "AlpsSubTreePool.h"
#include "AlpsModel.h"
#include "AlpsTime.h"

//#############################################################################

/** The base class of knowledge broker class. */
class AlpsKnowledgeBroker {

 private:

    AlpsKnowledgeBroker(const AlpsKnowledgeBroker&);
    AlpsKnowledgeBroker& operator=(const AlpsKnowledgeBroker&);

    /** Stores a master copy of any encodable object for decoding purposes. */
    static std::map<const char*,const AlpsKnowledge*, AlpsStrLess>* decodeMap_;
  
 protected:

    /** The instance name. */
    std::string instanceName_;

    /** Pointer to model. */
    AlpsModel *model_;

    /** Alps phase. (RAMPUP, SEARCH, RAMPDOWN)*/
    AlpsPhase phase_;

    /** @name knowledge pools
     *
     */
    //@{
    /** A subtree pool holding a collection of subtrees. For serial version,
	there is only one subtree in the pool. */
    AlpsSubTreePool* subTreePool_;

    /** A solution pool containing the solutions found. */
    AlpsSolutionPool* solPool_;

    /** The collection of pools managed by the knowledge broker. */
    std::map<AlpsKnowledgeType, AlpsKnowledgePool*>* pools_;
    //@}

    /** @name Exploring subtree 
     *
     */
    //@{
    /** Point to the subtree that being explored. */
    AlpsSubTree* workingSubTree_;
    
    /** Indicate whether needWorkingSubTree_ points to 0. */
    bool needWorkingSubTree_;

    /** The index to be assigned to a new search tree node. */
    AlpsNodeIndex_t nextIndex_;

    /** The maximum index can been assigned on this process. */
    AlpsNodeIndex_t maxIndex_;

    /** The approximate memory size (bytes) of a node with full description. */
    int nodeMemSize_;
    //@}                       

    /** @name Statistics
     *
     */
    //@{
    /***/
    /** Timer. */
    AlpsTimer timer_;

    /** The number of nodes processed. */
    int nodeProcessedNum_;
    
    /** The number of nodes in pool. */
    int nodeLeftNum_;

    /** The depth of the tree explored. */
    int treeDepth_;

    /** The status of termination. */
    AlpsSolStatus termStatus_;
    //@}
    
    /** @name compare base
     *
     */
    //@{
    /** Tree comparison criterion. */
    AlpsCompareBase<AlpsSubTree*>* treeCompare_;

    /** Node comparison criterion. */
    AlpsCompareBase<AlpsTreeNode*>* nodeCompare_;
    //@}
    
    /** @name message handling
     *
     */
    //@{
    /** Message handler. */
    CoinMessageHandler * handler_;

    /** Alps messages. */
    CoinMessages messages_;

    /** The leve of printing message to screen. 
        (-1: no; 0: default; 1: basic; 2: verbose) */
    int msgLevel_;
    
    /** The degree of log file.
        (-1: no; 0: default; 1: basic; 2: verbose) */
    int logFileLevel_;

    /** The log file. */
    std::string logfile_;
    //@}

 public:

    /** Default constructor. */
    AlpsKnowledgeBroker();

    /** Destructor. */
    virtual ~AlpsKnowledgeBroker();

    //-------------------------------------------------------------------------
    /** @name Funcitons related to register knowledge. 
     *
     */
    //@{
    /** Every user derived knowledge class must register. 
	The register methods register the decode method of the class so that 
	later on we can decode objects from buffers. Invoking this 
	registration for class <code>foo</code> is a single line:<br>
	<code>foo().registerClass(name, userKnowledge);</code> */
    void registerClass(const char * name, AlpsKnowledge* userKnowledge) {
	(*decodeMap_)[name] = userKnowledge;
    }

    /** This method returns the pointer to an empty object of the registered
	class <code>name</code>. Then the <code>decode()</code> method of that
	object can be used to decode a new object of the same type from the
	buffer. This method will be invoked as follows to decode an object
	whose type is <code>name</code>:<br>
	<code>obj = AlpsKnowledge::decoderObject(name)->decode(buf) </code> */
    static const AlpsKnowledge* decoderObject(const char* name) {
	return (*decodeMap_)[name];
    }
    //@}

    //------------------------------------------------------

    /** @name Funcitons related to exploring subtree. 
     *  
     */
    //@{
    /** Do some initialization for search. */
    virtual void initializeSearch(int argc, 
				  char* argv[], 
				  AlpsModel& model) = 0;

    /** Explore the tree rooted as the given root. */
    virtual void search(AlpsTreeNode* root) = 0;  

    /** Search best solution for a given model. */
    virtual void searchModel(AlpsModel *model) {

	AlpsTreeNode* root = model->createRoot();
	search(root);
    }
    //@}

    //------------------------------------------------------

    /** @name Get/set phase.
     *  
     */
    //@{
    AlpsPhase getPhase() { return phase_; }
    void setPhase(AlpsPhase ph) { phase_ = ph; }
    //@}

    //@{
    AlpsModel *getModel() { return model_; }
    void setModel(AlpsModel *m) { model_ = m; }
    //@}

    /** @name Interface with the knowledge pools 
     *  
     */
    //@{
    /** Set up knowledge pools for this broker. */
    inline void setupKnowledgePools() {
	pools_ = new std::map<AlpsKnowledgeType, AlpsKnowledgePool*>;
	pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
			( ALPS_SOLUTION, solPool_ ) );
	pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
			( ALPS_SUBTREE, subTreePool_ ) );
	
	//--------------------------------------------------
	// NOTE: User can add her own rules by broker.setTreeCompare().
	//--------------------------------------------------

	const int treeCompareRule = 
	    model_->AlpsPar()->entry(AlpsParams::subTreeCompareRule);
	
	if (treeCompareRule == 0) {      // Best Quality
	    treeCompare_ = new AlpsCompareSubTreeBest;
	}
	else if (treeCompareRule == 1) { // Quantity
	    treeCompare_ = new AlpsCompareSubTreeQuantity;
	}
	else if (treeCompareRule == 2) { // the depth of root
	    treeCompare_ = new AlpsCompareSubTreeBreadth;
	}
	else if (treeCompareRule == 3) { // Hybrid
	    // Not implement
	    throw CoinError("Sorry, hybrid tree comparison isn't implemented", 
			    "setupKnowledgePools", "AlpsSubTree"); 
	}
	else {
	    std::cout << "treeCompareRule = " << treeCompareRule << std::endl;
	    throw CoinError("Unknown subtree compare rule", 
			"getGoodness", "AlpsSubTree"); 
	}
	subTreePool_->setComparison(*treeCompare_);

	//--------------------------------------------------
	// NOTE: User can add her own rules by broker.setNodeCompare().
	//--------------------------------------------------

	const int strategy =
	    model_->AlpsPar()->entry(AlpsParams::nodeSelStrategy);
	if (strategy == 0) {        // best bound
	    nodeCompare_ = new AlpsCompareTreeNodeBest;
	}
	else if (strategy == 1) {   // depth first
	    nodeCompare_ = new AlpsCompareTreeNodeDepth;
	}
	else if (strategy == 2) {   // best estimate
	    nodeCompare_ = new AlpsCompareTreeNodeEstimate;
	}
	else if (strategy == 3) {   // breath first
	    nodeCompare_ = new AlpsCompareTreeNodeBreadth;
	}
	else {
            std::cout << "ERROR: unknown searchStrategy " << strategy 
                      << std::endl;
	    throw CoinError("Unknown search strategy", 
			    "setupKnowledgePools", "AlpsSubTree"); 
	}
    }
    
    /** Add a knowledge pool into the Knowledge pools */
    inline void addKnowledgePool(AlpsKnowledgeType kt, AlpsKnowledgePool* kp) {
	if(kt == ALPS_SOLUTION || kt == ALPS_SUBTREE) {
	    // AlpsKnowledgePool* akp = static_cast<AlpsKnowledgePool*>(kp);
	    pools_->insert
		(std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>(kt, kp));
	}
	else
	    throw CoinError("Broker doesn't manage this type of knowledge", 
			    "addKnowledgePool()", "AlpsKnowledgeBroker"); 
    }
  
    /** Retrieve a knowledge pool in the Knowledge base */
    inline  AlpsKnowledgePool* getKnowledgePool(AlpsKnowledgeType kt) const { 
	if(kt == ALPS_SOLUTION || kt == ALPS_SUBTREE) 
	    return (*pools_)[kt];
	else
	    throw CoinError("Broker doesn't manage this type of knowledge", 
			    "getKnowledgePool()", "AlpsKnowledgeBroker"); 
    }

    /** Query the number of knowledge in the given type of a knowledge pool. */
    virtual int getNumKnowledges(AlpsKnowledgeType kt) const {
	if(kt == ALPS_SOLUTION || kt == ALPS_SUBTREE)
	    return getKnowledgePool(kt)->getNumKnowledges();
	else
	    throw CoinError("Broker doesn't manage this type of knowledge", 
			    "getNumKnowledgePool()", "AlpsKnowledgeBroker"); 
    }

    /** Query the max number of knowledge can be stored in a given 
	type of knowledge pools.*/
    virtual int getMaxNumKnowledges(AlpsKnowledgeType kt) const {
	if(kt == ALPS_SOLUTION || kt == ALPS_SUBTREE)
	    return getKnowledgePool(kt)->getMaxNumKnowledges();
	else
	    throw CoinError("Broker doesn't manage this type of knowledge", 
			    "getMaxNumKnowledges()", "AlpsKnowledgeBroker"); 
    }

    /** Set the max number of knowledge can be stored in a given 
	type o fknowledge pools. */
    virtual void setMaxNumKnowledges(AlpsKnowledgeType kt, int num) {
	if(kt == ALPS_SOLUTION || kt == ALPS_SUBTREE)
	    getKnowledgePool(kt)->setMaxNumKnowledges(num);
	else
	    throw CoinError("Broker doesn't manage this type of knowledge", 
			    "setMaxNumKnowledges()", "AlpsKnowledgeBroker"); 
    }

    /** Query whether there are knowledges in the given type of 
	knowledge pools. */
    virtual bool hasKnowledge(AlpsKnowledgeType kt) const {
	if(kt == ALPS_SOLUTION || kt == ALPS_SUBTREE)
	    return getKnowledgePool(kt)->hasKnowledge();
	else
	    throw CoinError("Broker doesn't manage this type of knowledge", 
			    "hasKnowledge()", "AlpsKnowledgeBroker"); 
    }

    /** Get a knowledge, but doesn't remove it from the pool*/
    virtual std::pair<AlpsKnowledge*, double> 
	getKnowledge(AlpsKnowledgeType kt) const {
	if(kt == ALPS_SOLUTION || kt == ALPS_SUBTREE)   
	    return getKnowledgePool(kt)->getKnowledge();
	else
	    throw CoinError("Broker doesn't manage this type of knowledge", 
			    "getKnowledge()", "AlpsKnowledgeBroker"); 
    }

    /** Remove the a knowledge from the given type of knowledge pools.*/
    virtual void popKnowledge(AlpsKnowledgeType kt) {
	if(kt == ALPS_SOLUTION || kt == ALPS_SUBTREE)
	    getKnowledgePool(kt)->popKnowledge();
	else
	    throw CoinError("Broker doesn't manage this type of knowledge", 
			    "popKnowledge()", "AlpsKnowledgeBroker"); 
    } 

    /** Get the best knowledge in the given type of knowledge pools. */
    virtual std::pair<AlpsKnowledge*, double> 
	getBestKnowledge(AlpsKnowledgeType kt) const { 
	if(kt == ALPS_SOLUTION || kt == ALPS_SUBTREE)
	    return getKnowledgePool(kt)->getBestKnowledge();
	else
	    throw CoinError("Broker doesn't manage this type of knowledge", 
			    "getBestKnowledge()", "AlpsKnowledgeBroker"); 
    }

    /** Get all knowledges in the given type of knowledge pools. */
    virtual void getAllKnowledges (AlpsKnowledgeType kt, 
				   std::vector<std::pair<AlpsKnowledge*, 
				   double> >& kls)  const { 
	if(kt == ALPS_SOLUTION || kt == ALPS_SUBTREE)
	    getKnowledgePool(kt)->getAllKnowledges(kls);
	else
	    throw CoinError("Broker doesn't manage this type of knowledge", 
			    "popKnowledge()", "AlpsKnowledgeBroker"); 
    }

    /** Add a knowledge in the given type of knowledge pools. */
    virtual void addKnowledge(AlpsKnowledgeType kt, 
			      AlpsKnowledge* kl, 
			      double value ) { 
	if(kt == ALPS_SOLUTION || kt == ALPS_SUBTREE)
	    getKnowledgePool(kt)->addKnowledge(kl, value);
	else
	    throw CoinError("Broker doesn't manage this type of knowledge", 
			    "popKnowledge()", "AlpsKnowledgeBroker"); 
    }
    //@}


    /** @name Querty and set statistics
     *
     */
    /** Query the number of node processed by this process. */
    int getNumNodesProcessed() const {
	return nodeProcessedNum_;
    }

    /** Query the number of left nodes on this process. */
    virtual int getNumNodesLeft() {
	nodeLeftNum_ = 0;
	if (workingSubTree_ != 0)
	    nodeLeftNum_ += workingSubTree_->getNumNodes();
   
	std::vector<AlpsSubTree*> subTreeVec = 
	    subTreePool_->getSubTreeList().getContainer();
	std::vector<AlpsSubTree*>::iterator pos1 = subTreeVec.begin();
	std::vector<AlpsSubTree*>::iterator pos2 = subTreeVec.end();
	for ( ; pos1 != pos2; ++pos1) {
	    nodeLeftNum_ += (*pos1)->getNumNodes();
	}
	
	return 	nodeLeftNum_;
    }

    /** Query search termination status. */
    AlpsSolStatus getTermStatus() const {
	return termStatus_;
    }

    /** Set terminate status. */
    void setTermStatus(AlpsSolStatus status) {
	termStatus_ = status;
    }

    /** Query timer. */
    AlpsTimer & timer() {
	return timer_;
    }

    /** Search statistics log. */
    virtual void searchLog() = 0;
    //@}

    /** @name Query and set the approximate memory size of a tree node
     *
     */
    //@{
    int getNodeMemSize() { return nodeMemSize_; }
    void setNodeMemSize(int ms) { nodeMemSize_ = ms; }
    //@}
	 
    /** @name Report the best result
     *  
     */
    //@{
    /** The process queries the objective value of the incumbent that 
	it stores.*/
    virtual double getIncumbentValue() const = 0;

    /** The process (serial) / the master (parallel) queries the quality
        of the best solution that it knows. */
    virtual double getBestQuality() const = 0;

    /** The process (serial) / the master (parallel) outputs the best 
	solution that it knows to a file or std::out. */
    virtual void printBestSolution(char* outputFile = 0) const = 0;
	
    /** The process (serial) / the master (parallel) outputs the best 
	solution found and its objective value to a file or std::out. */
    virtual void printBestResult(char* outputFile = 0) const = 0;
    //@}

    /** Qeury the global rank of process. Note: not useful for serial code.*/
    virtual int getProcRank() const { return -10; }

    /** Query the type (master, hub, or worker) of the process */
    virtual AlpsProcessType getProcType() 
	{ return AlpsProcessTypeWorker; } /* Serial is worker!*/
    
    
    /** @name Query and set node index
     *  
     */
    //@{
    /** Query the next index assigned to a newly created node, and then
	increment the nextIndex_ by 1. */
    AlpsNodeIndex_t nextNodeIndex() { return nextIndex_++; }

    /** Query the next index assigned to a newly created node. */
    AlpsNodeIndex_t getNextNodeIndex() const { return nextIndex_; }

    /** Set nextIndex_. */
    void setNextNodeIndex(AlpsNodeIndex_t s) { nextIndex_ = s; } 

    /** Queriy the upper bound of node indices. */
    AlpsNodeIndex_t getMaxNodeIndex() const { return maxIndex_; }
  
    /** Set the upper bound of node indices. */
    void setMaxNodeIndex(AlpsNodeIndex_t s) { maxIndex_ = s; }
    //@}   

    /** @name Query and set comparision
     *  
     */
    //@{
    AlpsCompareBase<AlpsSubTree*>* getTreeCompare() const { 
	return treeCompare_; 
    }
    void setTreeCompare(AlpsCompareBase<AlpsSubTree*>* tc) {
	treeCompare_ = tc;
	subTreePool_->setComparison(*treeCompare_);
    }
    AlpsCompareBase<AlpsTreeNode*>* getNodeCompare() const {
	return nodeCompare_;
    }
    void setNodeCompare(AlpsCompareBase<AlpsTreeNode*>* nc) {
	nodeCompare_ = nc;
    }
    //@}
   
    /**@name Message handling */
    //@{
    /// Pass in Message handler (not deleted at end)
    void passInMessageHandler(CoinMessageHandler * handler);
    /// Set language
    void newLanguage(CoinMessages::Language language);
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
};
#endif
