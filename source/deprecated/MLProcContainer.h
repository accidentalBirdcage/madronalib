
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_PROC_CONTAINER_H
#define ML_PROC_CONTAINER_H

#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <sstream>

#include "MLProc.h"
#include "MLDSP.h"
#include "MLParameter.h"
#include "MLRatio.h"

#include "JuceHeader.h" // used only for XML loading now. TODO move to creation by scripting and remove.

class MLPublishedInput
{
public:
	MLPublishedInput(const MLProcPtr proc, const int inputIndex, const int index) :
		mIndex(index), 
		mProc(proc), 
		mProcInputIndex(inputIndex), 
		mDest(proc), 
		mDestInputIndex(inputIndex) 
		{}
	~MLPublishedInput() {}
	
	void setDest(const MLProcPtr proc, const int index)
	{
		mDest = proc;
		mDestInputIndex = index;
	}
	
	ml::Symbol mName;
	int mIndex;
	
	// proc and input index that the input signal goes to.
	MLProcPtr mProc;
	int mProcInputIndex;
	
	// proc and input index that the input signal goes to after resampling, if any.
	// if container is not resampled that will be the same as mProc and mProcInputIndex.
	MLProcPtr mDest;
	int mDestInputIndex;
};

typedef std::shared_ptr<MLPublishedInput> MLPublishedInputPtr;

class MLPublishedOutput
{
public:
	MLPublishedOutput(const MLProcPtr proc, const int outputIndex, const int index) :
		mIndex(index), 
		mProc(proc), 
		mOutput(outputIndex), 
		mSrc(proc), 
		mSrcOutputIndex(outputIndex) 
		{}
	~MLPublishedOutput() {}

	void setSrc(const MLProcPtr proc, const int index)
	{
		mSrc = proc;
		mSrcOutputIndex = index;
	}
	
	ml::Symbol mName;
	int mIndex;
	
	// proc and output index that the output signal comes from.
	// if we are resampling this will be the resampler.
	MLProcPtr mProc;
	int mOutput;
	
	// proc and output index that the output signal comes from before resampling, if any.
	// if container is not resampled that will be the same as mProc and mOutput.
	MLProcPtr mSrc;
	int mSrcOutputIndex;
};

typedef std::shared_ptr<MLPublishedOutput> MLPublishedOutputPtr;

// for gathering stats during process()
class MLSignalStats
{
public:
	
	MLSignalStats() :
		mProcs(0),
		mSignalBuffers(0),
		mSignals(0),
		mNanSignals(0),
		mConstantSignals(0)
		{};
	~MLSignalStats() {};
	
	void dump();
	
	int mProcs;
	int mSignalBuffers;
	int mSignals;
	int mNanSignals;
	int mConstantSignals;
};

class MLContainerBase
{
public:
	virtual ~MLContainerBase() {};
	
	// ----------------------------------------------------------------
	#pragma mark graph creation	
	//
	// create a proc.
	virtual MLProcPtr newProc(const ml::Symbol className, const ml::Symbol procName) = 0;
	
	// make a new MLProc and add it to this container so that it can be found by name.
	// name must be unique within this container, otherwise nothing is added 
	// and nameInUseErr is returned.  
	// new proc is added at end of ops list, or at position before proc matching positionName
	// if one is specified.
	virtual MLProc::err addProc(const ml::Symbol className, const ml::Symbol procName) = 0;
	
	// get a procPtr by slash delimited path name, recursively descending into subcontainers.
	// leaf names can be the same if procs have differently named paths.
	virtual MLProcPtr getProc(const ml::Path & pathName) = 0;
	
	// connect one signal output to one signal input to make a graph edge. 
	// The src and dest procs must live in the same container. 
	virtual void addPipe(const ml::Path& src, const ml::Symbol output, const ml::Path& dest, const ml::Symbol input) = 0;

	// build the connection between Procs that a Pipe specifies, by copying
	// Signal ptrs, adjusting block sizes, and creating resamplers as necessary.
	virtual MLProc::err connectProcs(MLProcPtr a, int ai, MLProcPtr b, int bi) = 0;
	
	// ----------------------------------------------------------------
	#pragma mark -- I/O
	//
	virtual void publishInput(const ml::Path & procName, const ml::Symbol inputName, const ml::Symbol alias) = 0;
	virtual void publishOutput(const ml::Path & procName, const ml::Symbol outputName, const ml::Symbol alias) = 0;
	
	// ----------------------------------------------------------------
	#pragma mark -- signals
	//	
	virtual MLProc::err addSignalBuffers(const ml::Path & procAddress, const ml::Symbol outputName, 
		const ml::Symbol alias, int trigMode, int bufLength, int frameSize) = 0;
	virtual void gatherSignalBuffers(const ml::Path & procAddress, const ml::Symbol alias, MLProcList& buffers) = 0;
	
	// ----------------------------------------------------------------
	#pragma mark -- parameters
	// 
	virtual MLPublishedParamPtr publishParam(const ml::Path & procName, const ml::Symbol paramName, const ml::Symbol alias, const ml::Symbol type) = 0;
	virtual void addSetterToParam(MLPublishedParamPtr p, const ml::Path & procName, const ml::Symbol param) = 0;
	virtual void setPublishedParam(int index, const MLProperty& val) = 0;
	
protected:
	virtual void routeParam(const ml::Path & procAddress, const ml::Symbol paramName, const MLProperty& val) = 0;
	virtual void makeRoot(const ml::Symbol name) = 0;
	virtual bool isRoot() const = 0;
	virtual void compile() = 0;

public:
	// ----------------------------------------------------------------
	#pragma mark -- building
	// 
	virtual void buildGraph(juce::XmlElement* pDoc) = 0;	
	virtual void dumpGraph(int indent) = 0;	
	virtual void setProcParams(const ml::Path& procName, juce::XmlElement* pelem) = 0;

	typedef std::map<ml::Symbol, MLProcOwner> SymbolProcMapT;	
	
	typedef std::map<ml::Symbol, MLPublishedParamPtr> MLPublishedParamMapT;
	typedef std::map<ml::Symbol, MLPublishedInputPtr> MLPublishedInputMapT;
	typedef std::map<ml::Symbol, MLPublishedOutputPtr> MLPublishedOutputMapT;
};

// An MLProcContainer stores a connected graph of MLProc objects.
// Edges between MLProcs are represented by MLPipe objects.
// 
class MLProcContainer: public MLProc, public MLContainerBase, public MLDSPContext
{
friend class MLDSPEngine;

public:	

	class MLPipe
	{
	friend class MLDSPEngine;
	public:
		
		MLPipe(MLProcPtr a, int ai, MLProcPtr b, int bi) :
			mSrc(a), mSrcIndex(ai), mDest(b), mDestIndex(bi)
			{};
		~MLPipe()
			{};
		
		MLProcPtr mSrc;
		int mSrcIndex;
		MLProcPtr mDest;
		int mDestIndex;
	};

	typedef std::shared_ptr<MLPipe> MLPipePtr;

	MLProcContainer();
	virtual ~MLProcContainer();	
	
	// standard ProcInfo 
	virtual MLProcInfoBase& procInfo() { return mInfo; }
	
	// ----------------------------------------------------------------
	#pragma mark MLDSPContext methods
	//

	virtual void setEnabled(bool t);
	virtual bool isEnabled() const;
	virtual bool isProcEnabled(const MLProc* p) const;

	// ----------------------------------------------------------------
	#pragma mark MLProc methods
	//
	bool isContainer(void) { return true; }
		
	virtual void collectStats(MLSignalStats* pStats);

	virtual void process();
	virtual err prepareToProcess();

	void clear();	// clear buffers, DSP history
	void clearInput(const int idx);
	MLProc::err setInput(const int idx, const MLSignal& sig);
	
	virtual int getInputIndex(const ml::Symbol name);
	virtual int getOutputIndex(const ml::Symbol name);	
	int getNumProcs();
	float getParam(const ml::Symbol paramName);
	//
	virtual void makeRoot(const ml::Symbol name); // mark as root context
	inline virtual bool isRoot() const { return (getContext() == this); }
	virtual void compile();
	
	// ----------------------------------------------------------------
	#pragma mark graph creation
	//
	MLProcPtr newProc(const ml::Symbol className, const ml::Symbol procName);
	virtual MLProc::err addProc(const ml::Symbol className, const ml::Symbol procName); 
	virtual void addPipe(const ml::Path& src, const ml::Symbol output, const ml::Path& dest, const ml::Symbol input);
	virtual MLProc::err connectProcs(MLProcPtr a, int ai, MLProcPtr b, int bi);
	//	
	virtual MLProcPtr getProc(const ml::Path & pathName); 
	void getProcList(MLProcList& pList, const ml::Path & pathName, int copies, bool enabledOnly = true);
	//
	virtual void publishInput(const ml::Path & procName, const ml::Symbol inputName, const ml::Symbol alias);
	virtual void publishOutput(const ml::Path & procName, const ml::Symbol outputName, const ml::Symbol alias);	
	
	//ml::Symbol getOutputName(int index);

	// ----------------------------------------------------------------
	#pragma mark signals
	//
	// methods of MLContainerBase
	virtual MLProc::err addSignalBuffers(const ml::Path & procAddress, const ml::Symbol outputName, 
		const ml::Symbol alias, int trigMode, int bufLength, int frameSize = 1);
	virtual void gatherSignalBuffers(const ml::Path & procAddress, const ml::Symbol alias, MLProcList& buffers);

private:
	err addBufferHere(const ml::Path & procName, ml::Symbol outputName, ml::Symbol alias, 
		int trigMode, int bufLength, int frameSize);
	MLProc::err addProcAfter(ml::Symbol className, ml::Symbol alias, ml::Symbol afterProc); 
public:

	//
	// ----------------------------------------------------------------
	#pragma mark parameters
	// 
	virtual MLPublishedParamPtr publishParam(const ml::Path & procName, const ml::Symbol paramName, const ml::Symbol alias, const ml::Symbol type);
	virtual void addSetterToParam(MLPublishedParamPtr p, const ml::Path & procName, const ml::Symbol param);
	virtual void setPublishedParam(int index, const MLProperty& val);
	virtual void routeParam(const ml::Path & procAddress, const ml::Symbol paramName, const MLProperty& val);
	
	MLPublishedParamPtr getParamPtr(int index) const;
	int getParamIndex(const ml::Symbol name);
	const std::string& getParamGroupName(int index);	
	float getParamByIndex(int index);
	int getPublishedParams();

	// ----------------------------------------------------------------
	#pragma mark xml loading / saving
	//
	void scanDoc(juce::XmlDocument* pDoc, int* numParameters);
    ml::Symbol RequiredAttribute(juce::XmlElement* parent, const char * name);
    ml::Path RequiredPathAttribute(juce::XmlElement* parent, const char * name);     
    void buildGraph(juce::XmlElement* pDoc);
	virtual void dumpGraph(int indent);			
	virtual void setProcParams(const ml::Path& procName, juce::XmlElement* pelem);	

	// ----------------------------------------------------------------
	#pragma mark buffer pool
	//
	MLSignal* allocBuffer(int frameSize = 1);
	void freeBuffer(MLSignal* pBuf);
	
protected:
	virtual err buildProc(juce::XmlElement* parent);
	
	// count number of param elements in document.
	// this is used to return param info to host before graph is built. 
	int countPublishedParamsInDoc(juce::XmlElement* pElem);	
	
	MLProcFactory& theProcFactory;

private:
	
	// append a Proc to the end of the proc list, to be run in order by process().
	// Currently the compiler is dumb: procs will be run in the order they are added.
	void addProcToOps (MLProcPtr proc);		

	// private doc building mathods
	void setPublishedParamAttrs(MLPublishedParamPtr p, juce::XmlElement* pelem);
	
	// dump all MLProc subclasses registered at static init time
	void printClassRegistry(void){ theProcFactory.printRegistry(); }

	void dumpMap();
	
// ----------------------------------------------------------------
#pragma mark data
	
private:		
	MLProcInfo<MLProcContainer> mInfo;
	
protected:	

	// TODO 
	// params inputs and outputs can all be managed by some new
	// class that indexes ptrs quickly by both names and integers
	// required: getIndexByName, getObjectByIndex, addObjectByName
	// 
	// map to published params by alias name
	MLPublishedParamMapT mPublishedParamMap;
	// vector of published params, for speedy access by integer	
	std::vector<MLPublishedParamPtr> mPublishedParams;	
			
	// map to published inputs by name
	MLPublishedInputMapT mPublishedInputMap;	
	// by index
	std::vector<MLPublishedInputPtr> mPublishedInputs;
	
	// map to published outputs by name
	MLPublishedOutputMapT mPublishedOutputMap;	
	// by index
	std::vector<MLPublishedOutputPtr> mPublishedOutputs;

	// vector of processors in order of processing operations.
	// This is what gets iterated on during process().
	std::vector<MLProc*> mOpsVec;
		
	// MLTEST PROCS ARE OWNED HERE
	// map to processors by name.   
	SymbolProcMapT mProcMap;
	
	// list of processors in order of creation. 
	std::list<MLProcPtr> mProcList;

	// List of Pipes created by addPipe().  Pipes are not named. 
	std::list<MLPipePtr> mPipeList; 

	// signal buffers for running procs.
	std::list<MLSignalPtr> mBufferPool;

	// parameter groups
	MLParamGroupMap mParamGroups;
	
	MLSignalStats* mStatsPtr;
	
private: // TODO more data should be private

};

// ----------------------------------------------------------------
#pragma mark compiler temps
// TODO move into class

// represents a signal and its lifetime in the DSP graph.
//
class compileSignal
{
public:
	enum
	{
		kNoLife = -1
	};

	compileSignal() : 
		mpSigBuffer(0), 
		mLifeStart(kNoLife), 
		mLifeEnd(kNoLife), 
		mPublishedInput(0), 
		mPublishedOutput(0),
		mFrameSize(1)
		{};
	~compileSignal(){};
	
	void setLifespan(int start, int end)
	{
		mLifeStart = start;
		mLifeEnd = end;
	}
	
	// get union of current lifespan with the one specified by [start, end].
	void addLifespan(int start, int end)
	{
		if (mLifeStart == kNoLife)
		{
			mLifeStart = start;
			mLifeEnd = end;
		}
		else
		{
			mLifeStart = ml::min(mLifeStart, start);
			mLifeEnd = ml::max(mLifeEnd, end);
		}
	}
	
	MLSignal* mpSigBuffer;
	int mLifeStart;
	int mLifeEnd;
	int mPublishedInput;
	int mPublishedOutput;
	int mFrameSize;
};

// a class representing a single processing node with inputs and outputs when compiling.
//
class compileOp
{
public:
	compileOp(MLProc* p) :
		procRef(p) {};
	~compileOp(){};

	int listIdx;
	MLProc* procRef;
	std::vector<ml::Symbol> inputs;
	std::vector<ml::Symbol> outputs;
};

// another temporary object for compile.
// TODO move inside class
// a buffer shared between multiple signals at different times. 
// the lifetime of each signal is stored in the signal itself.
class sharedBuffer
{
public:
	sharedBuffer() : mFrameSize(1) {};
	~sharedBuffer(){};
	bool canFit(compileSignal* sig);
	void insert(compileSignal* sig);

	// which signals are contained in this shared buffer?
	// sorted by signal lifetime. lifetimes cannot overlap.
	std::list<compileSignal*> mSignals;
	int mFrameSize;
};

// different functions to pack a signal into a list of shared buffers. 
// a new sharedBuffer is added to the list if it is needed.
// this is not quite a bin packing problem, because we are not allowed to 
// move the signals in time. 
//
void packUsingWastefulAlgorithm(compileSignal* sig, std::list<sharedBuffer>& bufs);
void packUsingFirstFitAlgorithm(compileSignal* sig, std::list<sharedBuffer>& bufs);

std::ostream& operator<< (std::ostream& out, const compileOp & r);
std::ostream& operator<< (std::ostream& out, const sharedBuffer & r);

#endif
