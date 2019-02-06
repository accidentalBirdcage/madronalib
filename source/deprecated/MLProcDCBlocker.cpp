
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcDCBlocker : public MLProc
{
public:
	 MLProcDCBlocker();
	~MLProcDCBlocker();
	
	void clear() override;
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcDCBlocker> mInfo;
	void calcCoeffs(void);
		
	// coeffs
	double mR;
	
	// history
	MLSample xn1;
	MLSample yn1;
	int mTest;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcDCBlocker> classReg("dc_blocker");
	ML_UNUSED MLProcParam<MLProcDCBlocker> params[1] = { "f" };
	ML_UNUSED MLProcInput<MLProcDCBlocker> inputs[] = {"in"}; 
	ML_UNUSED MLProcOutput<MLProcDCBlocker> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation

MLProcDCBlocker::MLProcDCBlocker()
{
	setParam("f", 200.);	// default cutoff, not steep
}

MLProcDCBlocker::~MLProcDCBlocker()
{
}

void MLProcDCBlocker::calcCoeffs(void) 
{
	static const ml::Symbol fSym("f");

	const float f = getParam(fSym);
	const double omega = f * ml::kTwoPi / getContextSampleRate();
	mR = cos(omega);
	mParamsChanged = false;
}

void MLProcDCBlocker::clear()
{
	xn1 = 0.;
	yn1 = 0.;
}

void MLProcDCBlocker::process()
{	
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();

	if (mParamsChanged) calcCoeffs();

	for (int n = 0; n < kFloatsPerDSPVector; ++n)
	{
		y[n] = x[n] - xn1 + mR*yn1;
		xn1 = x[n];
		yn1 = y[n];
	}
}

