
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProcDelayInput.h"


// ----------------------------------------------------------------
// registry section

// MLProcDelayInput has no signal outputs-- use MLProcDelayOut method to get multiple taps, buffered if needed.  
// We need a ringbuffer if we are transferring signals backwards in the DSP graph, because we may be run 
// with varying vector sizes.

// TODO get rid of this object and have the DSP compiler add SignalBuffer objects where needed.

namespace
{
	MLProcRegistryEntry<MLProcDelayInput> classReg("delay_input");
	ML_UNUSED MLProcParam<MLProcDelayInput> params[1] = {"length"};
	ML_UNUSED MLProcInput<MLProcDelayInput> inputs[] = {"in"};
}	

// ----------------------------------------------------------------
// implementation

MLProcDelayInput::MLProcDelayInput()
{
	setParam("length", 0.1f);	
}

MLProcDelayInput::~MLProcDelayInput()
{
//	debug() << "MLProcDelayInput destructor\n";
}

MLProc::err MLProcDelayInput::resize() 
{	
	static const ml::Symbol lengthSym("length");

	MLProc::err e = OK;
	const float sr = getContextSampleRate();
	int lenBits = ml::bitsToContain((int)(getParam(lengthSym) * sr));
	int length = 1 << lenBits;
	mLengthMask = length - 1;
	
	MLSample* pBuf = mBuffer.setDims(length);
	if (!pBuf)
	{
		e = memErr;
	}

	return e;
}

void MLProcDelayInput::clear() 
{	
	mBuffer.clear();
	mWriteIndex = 0;
}

void MLProcDelayInput::process()
{
	const MLSignal& x = getInput(1);
	
	// write input to delay line
	for (int n=0; n<kFloatsPerDSPVector; ++n)
	{
		mWriteIndex &= mLengthMask;
		mBuffer[mWriteIndex] = x[n];
		mWriteIndex++;
	}		
}

