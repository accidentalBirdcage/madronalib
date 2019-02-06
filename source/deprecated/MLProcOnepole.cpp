
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLDSPDeprecated.h"

// ----------------------------------------------------------------
// class definition

class MLProcOnepole : public MLProc
{
public:
	 MLProcOnepole();
	~MLProcOnepole();
	
	void clear() override;
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcOnepole> mInfo;
	void doParams(void);
		
	// coeffs
	MLSample mK;
	
	// history
	MLSample mY1;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcOnepole> classReg("onepole");
	ML_UNUSED MLProcParam<MLProcOnepole> params[1] = { "frequency" };
	ML_UNUSED MLProcInput<MLProcOnepole> inputs[] = {"in"}; 
	ML_UNUSED MLProcOutput<MLProcOnepole> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation


MLProcOnepole::MLProcOnepole()
{
	mK = 0.f;
	mY1 = 0.f;
	setParam("frequency", 1000.f);
}

MLProcOnepole::~MLProcOnepole()
{
}

void MLProcOnepole::doParams(void) 
{
	static const ml::Symbol frequencySym("frequency");

	const float f = ml::clamp(getParam(frequencySym), 50.f, getContextSampleRate() * 0.25f);
	const float invSr = getContextInvSampleRate();
	mK = fsin1(ml::kTwoPi * f * invSr);
	mParamsChanged = false;
}

void MLProcOnepole::clear()
{
	mY1 = 0.;
}


void MLProcOnepole::process()
{	
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	float dxdt; 
	
	if (mParamsChanged) doParams();
	
	for (int n=0; n<kFloatsPerDSPVector; ++n)
	{
		dxdt = x[n] - mY1;
		mY1 += mK*dxdt;
		y[n] = mY1;
	}
}



   
