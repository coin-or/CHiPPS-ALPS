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
 * Copyright (C) 2001-2019, Lehigh University, Yan Xu, Aykut Bulut, and      *
 *                          Ted Ralphs.                                      *
 * All Rights Reserved.                                                      *
 *===========================================================================*/


#include "iomanip"
#include "AlpsKnowledgeBrokerSerial.h"


//#############################################################################

/** Reading in Alps and user parameter sets, and read in model data. */
void
AlpsKnowledgeBrokerSerial::initializeSearch(int argc,
                                            char* argv[],
                                            AlpsModel& model) {

    // Store a pointer to model
    model.setBroker(this);
    model_ = &model;

    //--------------------------------------------------
    // Read in params.
    //--------------------------------------------------

    model.readParameters(argc, argv);

    //--------------------------------------------------
    // Set up messege, logfile.
    //--------------------------------------------------

    msgLevel_ = model_->AlpsPar()->entry(AlpsParams::msgLevel);
    messageHandler()->setLogLevel(msgLevel_);

    logFileLevel_ = model_->AlpsPar()->entry(AlpsParams::logFileLevel);
    if (logFileLevel_ > 0) {    // Require log file
        logfile_ = model_->AlpsPar()->entry(AlpsParams::logFile);
    }

    if (msgLevel_ > 0) {
	std::cout << "==  Welcome to the Abstract Library for Parallel Search (ALPS) \n";
	std::cout << "==  Copyright 2000-2019 Lehigh University and others \n";
	    std::cout << "==  All Rights Reserved. \n";
	    std::cout << "==  Distributed under the Eclipse Public License 1.0 \n";
	    if (strcmp(ALPS_VERSION, "trunk")){
		std::cout << "==  Version: " << ALPS_VERSION << std::endl;
	    }else{
		std::cout << "==  Version: Trunk (unstable) \n";
	    }
	    std::cout << "==  Build Date: " <<  __DATE__;
#ifdef ALPS_SVN_REV
            // todo(aykut) this will create a problem when ALPS_SVN_REV is not
            // char const * .
            std::cout << "\n==  Revision Number: " ALPS_SVN_REV;
#endif
            std::cout << std::endl;
    }

    //--------------------------------------------------
    // If there two args: xecutable + instance.
    //--------------------------------------------------

    if (argc == 2) {
        model_->AlpsPar()->setEntry(AlpsParams::instance, argv[1]);
    }

    //--------------------------------------------------
    // Read in model data if requred.
    //--------------------------------------------------

    std::string dataFile = model_->AlpsPar()->entry(AlpsParams::instance);

    if (dataFile != "NONE") {
        messageHandler()->message(ALPS_DATAFILE, messages())
            << dataFile.c_str() << CoinMessageEol;

        // Read in instance to be solved.
        model.readInstance(dataFile.c_str());

        if (logFileLevel_ > 0 || msgLevel_ > 0) {
            std::string fileDir = dataFile;
            std::string::size_type pos1 =
                fileDir.rfind ('/', std::string::npos);
            if (pos1 == std::string::npos) {
                // No /, put at beginning.
                pos1 = 0;
                //std::cout << "No /, pos1 = "<< pos1 << std::endl;
            }
            else {
                //std::cout << "Found /, pos1 = "<< pos1 << std::endl;
                ++pos1;
            }

            std::string::size_type pos2 = fileDir.find (".mps", pos1);

            if (pos2 == std::string::npos) {
                // No .mps
                //std::cout << "No .mps" << std::endl;

                pos2 = fileDir.find (".gz", pos1);
                if (pos2 == std::string::npos) {
                    // No .gz
                    pos2 = fileDir.length();
                    //std::cout << "No .gz, pos2 = "<< pos2 << std::endl;
                }
                else {
                    //std::cout << "Found .gz, pos2 = "<< pos2 << std::endl;
                }
            }

            // Sub-string from pos1 to pos2(not included)
            int length = static_cast<int> (pos2 - pos1);
            //std::cout << "pos1=" << pos1 <<", pos2="<< pos2
            //        << ", lenght=" << length << std::endl;

            instanceName_ = fileDir.substr(pos1, length);
            logfile_ = instanceName_ + ".log";

            model_->AlpsPar()->setEntry(AlpsParams::logFile,
                                        logfile_.c_str());
        }

        if (logFileLevel_ > 0) {
            //std::ofstream logFout(logfile_.c_str(), std::ofstream::app);
            std::ofstream logFout(logfile_.c_str());

            // Times
            logFout << "\n================================================"
                    << std::endl;
            logFout << "Problem = " << instanceName_ << std::endl;
            logFout << "Log file = " << logfile_ << std::endl << std::endl;
        }
        if (msgLevel_ > 2) {
            std::cout << "Problem = " << instanceName_ << std::endl;
            std::cout << "Data file = " << dataFile << std::endl;
            std::cout << "Log file = " << logfile_ << std::endl<< std::endl;
        }
    }

    //--------------------------------------------------
    // Preprecesss the model, and
    // do some require work to make the model usable.
    // May adjust parameters like msg level in user's code.
    //--------------------------------------------------

    model.preprocess();
    model.setupSelf();

    //--------------------------------------------------
    // Set up solution pool and subtree pool, set comparision.
    //--------------------------------------------------

    setupKnowledgePools();

    //--------------------------------------------------
    // Register knowledge (useless for serial).
    //--------------------------------------------------

    model.registerKnowledge();

    //------------------------------------------------------
    // Set clock type
    //------------------------------------------------------

    const int clockType =
      model_->AlpsPar()->entry(AlpsParams::clockType);

    timer_.setClockType(clockType);
    subTreeTimer_.setClockType(clockType);
    tempTimer_.setClockType(clockType);
}

//#############################################################################

void
AlpsKnowledgeBrokerSerial::rootSearch(AlpsTreeNode* root)
{
    AlpsReturnStatus status = AlpsReturnStatusOk;

    timer_.start();

    root->setBroker(this);
    root->setQuality(-ALPS_OBJ_MAX);
    root->setDepth(0);
    root->setIndex(0);
    root->setExplicit(1); // True.

    const int mns = model_->AlpsPar()->entry(AlpsParams::solLimit);
    setMaxNumKnowledges(AlpsKnowledgeTypeSolution, mns);

    //------------------------------------------------------
    // Construct tree.
    //------------------------------------------------------

    workingSubTree_ = new AlpsSubTree(this);
    workingSubTree_->setNodeSelection(nodeSelection_);

#ifdef NF_DEBUG_MORE
    // Useless work, just for testing subtree pool.
    subTreePool_->addKnowledge(workingSubTree_,
                               workingSubTree_->getQuality());
    subTreePool_->popKnowledge();
#endif

    workingSubTree_->setNextIndex(1);  // one more than root's index

    messageHandler()->message(ALPS_S_SEARCH_START, messages())
        << CoinMessageEol;

    //------------------------------------------------------
    // Search the best solution.
    //------------------------------------------------------

    const int nodeLimit = model_->AlpsPar()->entry(AlpsParams::nodeLimit);

    timer_.limit_ = model_->AlpsPar()->entry(AlpsParams::timeLimit);
    status = workingSubTree_->exploreSubTree(root,
                                             nodeLimit,
                                             timer_.limit_,
                                             nodeProcessedNum_,
                                             nodeBranchedNum_,
                                             nodeDiscardedNum_,
                                             nodePartialNum_,
                                             treeDepth_);

    updateNumNodesLeft();

    model_->postprocess();

    timer_.stop();

    searchLog();

    /* Problem specific log. */
    model_->modelLog();
}

//#############################################################################

void
AlpsKnowledgeBrokerSerial::searchLog()
{
    char printSolution = model_->AlpsPar()->entry(AlpsParams::printSolution);

    if (msgLevel_ > 0) {
        std::cout << std::endl;
        if (getSolStatus() == AlpsExitStatusOptimal) {
            messageHandler()->message(ALPS_T_COMPLETE, messages())
               << CoinMessageEol;
        }
        else if (getSolStatus() == AlpsExitStatusNodeLimit) {
            messageHandler()->message(ALPS_T_NODE_LIMIT, messages())
                << nodeProcessedNum_ << nodeLeftNum_ << CoinMessageEol;
        }
        else if (getSolStatus() == AlpsExitStatusTimeLimit) {
            messageHandler()->message(ALPS_T_TIME_LIMIT, messages())
                << nodeProcessedNum_ << nodeLeftNum_ << CoinMessageEol;
        }
        else if (getSolStatus() == AlpsExitStatusFeasible) {
            messageHandler()->message(ALPS_T_FEASIBLE, messages())
                << nodeProcessedNum_ << nodeLeftNum_ << CoinMessageEol;
        }
        else {
            messageHandler()->message(ALPS_T_INFEASIBLE, messages())
                << nodeProcessedNum_ << nodeLeftNum_ << CoinMessageEol;
        }

        if (hasKnowledge(AlpsKnowledgeTypeSolution)) {
            AlpsSolution *solution = dynamic_cast<AlpsSolution *>
               (getBestKnowledge(AlpsKnowledgeTypeSolution).first);
            if (solution->getDepth() >= 0){
               messageHandler()->message(ALPS_S_FINAL_SOL_WD, messages())
                  << getBestKnowledge(AlpsKnowledgeTypeSolution).second <<
                  solution->getDepth() << CoinMessageEol;
            }else{
               messageHandler()->message(ALPS_S_FINAL_SOL, messages())
                  << getBestKnowledge(AlpsKnowledgeTypeSolution).second <<
                  CoinMessageEol;
            }
        }
        else {
            messageHandler()->message(ALPS_S_FINAL_NO_SOL, messages())
                << CoinMessageEol;
        }
        if (nodePartialNum_){
           messageHandler()->message(ALPS_S_FINAL_NODE_FULL, messages())
              << nodeProcessedNum_ << CoinMessageEol;
           messageHandler()->message(ALPS_S_FINAL_NODE_PARTIAL, messages())
              << nodePartialNum_ << CoinMessageEol;
        }else{
           messageHandler()->message(ALPS_S_FINAL_NODE_PROCESSED, messages())
              << nodeProcessedNum_ << CoinMessageEol;
        }
        messageHandler()->message(ALPS_S_FINAL_NODE_BRANCHED, messages())
            << nodeBranchedNum_ << CoinMessageEol;
        messageHandler()->message(ALPS_S_FINAL_NODE_DISCARDED, messages())
            << nodeDiscardedNum_ << CoinMessageEol;
        messageHandler()->message(ALPS_S_FINAL_NODE_LEFT, messages())
            << nodeLeftNum_ << CoinMessageEol;
        messageHandler()->message(ALPS_S_FINAL_DEPTH, messages())
            << treeDepth_ << CoinMessageEol;
        messageHandler()->message(ALPS_S_FINAL_CPU, messages())
            << timer_.getCpuTime() << CoinMessageEol;
        messageHandler()->message(ALPS_S_FINAL_WALLCLOCK, messages())
            << timer_.getWallClockTime() << CoinMessageEol;

        if (peakMemory_ > 0.0001) {
            messageHandler()->message(ALPS_PEAK_MEMORY, messages())
                << peakMemory_ << CoinMessageEol;
        }

        if (printSolution && hasKnowledge(AlpsKnowledgeTypeSolution)) {
            AlpsSolution *solution = dynamic_cast<AlpsSolution *>
                (getBestKnowledge(AlpsKnowledgeTypeSolution).first);
            solution->print(std::cout);
        }
    }

    if(logFileLevel_ > 0) {
        std::ofstream fout(logfile_.c_str(), std::ofstream::app);
        fout << std::endl;

        if (hasKnowledge(AlpsKnowledgeTypeSolution)) {
            fout << "Best solution quality = " << getBestQuality() << std::endl;
        }
        else {
            fout << "No solution was found." << std::endl;
        }
        fout << "Number of nodes processed = "<<nodeProcessedNum_ << std::endl;
        fout << "Number of nodes partially processed = "<<nodePartialNum_<<std::endl;
        fout << "Number of nodes branched = "<<nodeBranchedNum_ << std::endl;
        fout << "Number of nodes discarded = "<<nodeDiscardedNum_ << std::endl;
        fout << "Number of nodes left in queue= " << nodeLeftNum_ << std::endl;
        fout << "Tree depth = " << treeDepth_ << std::endl;

        fout << "Search CPU time =  " << timer_.getCpuTime() << " seconds"
             << std::endl;
        fout << "Search wallclock = " << timer_.getWallClockTime() <<" seconds"
              << std::endl;
        fout << std::endl;

        if (printSolution && hasKnowledge(AlpsKnowledgeTypeSolution)) {
            AlpsSolution *solution = dynamic_cast<AlpsSolution *>
                (getBestKnowledge(AlpsKnowledgeTypeSolution).first);
            solution->print(fout);
        }
    }
}

//#############################################################################
