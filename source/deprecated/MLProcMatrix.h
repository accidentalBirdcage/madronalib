// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_PROC_MATRIX_H
#define ML_PROC_MATRIX_H

#include "MLProc.h"

static const int kMLMatrixMaxIns = 32;
static const int kMLMatrixMaxOuts = 64;

// ----------------------------------------------------------------
// class definition

class MLProcMatrix : public MLProc
{
public:
	MLProcMatrix();
	~MLProcMatrix();
	
	MLProc::err resize() override;
	void clearConnections();
	void connect(int a, int b);
	void disconnect(int a, int b);
	bool getConnection(int a, int b);

	void process() override;		
	void calcCoeffs();
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcMatrix> mInfo;
	MLSample mGain[kMLMatrixMaxIns + 1][kMLMatrixMaxOuts + 1];
    int mInputs;
    int mOutputs;
    
    // the default signal size of kMLProcessChunkSize is used for these signals, so no need to resize.
    MLSignal mDelayBuffers[kMLMatrixMaxOuts];
};

#endif // ML_PROC_MATRIX_H
