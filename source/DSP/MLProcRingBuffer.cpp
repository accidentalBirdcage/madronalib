
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProcRingBuffer.h"

// ----------------------------------------------------------------
// registry section

// MLProcRingBuffer has no signal outputs-- use read method to get buffered signal.  
// We need a ringbuffer if we are transferring signals between procs that may run in
// different threads.

namespace
{
	MLProcRegistryEntry<MLProcRingBuffer> classReg("ringbuffer");
	ML_UNUSED MLProcParam<MLProcRingBuffer> params[] = {"length", "mode", "frame_size"};
	ML_UNUSED MLProcInput<MLProcRingBuffer> inputs[] = {"in"};
}	

static ml::Symbol frameSym("frame_size");
static ml::Symbol modeSym("mode");

// ----------------------------------------------------------------
// implementation

MLProcRingBuffer::MLProcRingBuffer()
{
	// defaults
	// TODO set from component->engine.
	//  will want downsampling for viewing when running at 96k or higher...
	setParam("frame_size", 1);	
	setParam("length", kMLRingBufferDefaultSize);	
	setParam("mode", eMLRingBufferNoTrash);
	mTrig1 = -1.f;
}

MLProcRingBuffer::~MLProcRingBuffer()
{
//	debug() << "MLProcRingBuffer destructor\n";
}

MLProc::err MLProcRingBuffer::resize() 
{	
	static const ml::Symbol frame_sizeSym("frame_size");
	static const ml::Symbol lengthSym("length");
	static const ml::Symbol modeSym("mode");

	MLProc::err e = OK;
	int frameSize = getParam(frame_sizeSym);
	int length = 1 << ml::bitsToContain((int)getParam(lengthSym));
	void * buf;
	
	// debug() << "allocating " << size << " samples for ringbuffer " << getName() << "\n";

	mSingleFrameBuffer.setDims(frameSize);
	buf = mRing.setDims(length, frameSize);
	
	if (!buf)
	{
		debug() << "MLRingBuffer: allocate failed!\n";
		e = memErr;
	}
	else
	{	
		PaUtil_InitializeRingBuffer( &mBuf, sizeof(MLSample), length*frameSize, buf );
        
		// get trash signal
		if (getParam(modeSym) != eMLRingBufferNoTrash)
		{
			mTrashSignal.setDims(length, frameSize);	
		}
	}

	return e;
}

void MLProcRingBuffer::doParams(void) 
{
	mParamsChanged = false;
}


void MLProcRingBuffer::process()
{

	int written;
	const MLSignal& x = getInput(1);
	int frameSize = getParam(frameSym);
	int inputFrameSize = x.getHeight();
	int framesToProcess = ml::min(kFloatsPerDSPVector, x.getWidth());

	// build if needed
	if (mParamsChanged) doParams();

	if (!mRing.getBuffer()) return;

	if(frameSize == 1)
	{
		written = (int)PaUtil_WriteRingBuffer(&mBuf, (void *)x.getConstBuffer(), framesToProcess );	
	}
	else
	{			
		if(inputFrameSize != frameSize)
		{
			debug() << "MLProcRingBuffer: input size mismatch: " << inputFrameSize << " to our " << frameSize << " \n";
		}
		else
		{
			// MLTEST
			// this is wrong! only writes first column of a 2d signal. HACK while 2d signal inputs are still too big.
			// TODO fix that issue in compiler. 
			framesToProcess = 1;
			
			// write tall signal to 1D buffer, rotating frames
			for(int i=0; i<framesToProcess; ++i)
			{
				// get one frame of source
				for(int j=0; j<frameSize; ++j)
				{
					mSingleFrameBuffer[j] = x(i, j);
				}
				written = (int)PaUtil_WriteRingBuffer(&mBuf, (void *)mSingleFrameBuffer.getConstBuffer(), frameSize );	
			}
		}
	}
}

// read the ring buffer into the given plane of the destination signal.
//
int MLProcRingBuffer::readToSignal(MLSignal& outSig, int frames, int plane)
{
	int lastRead = 0;
	MLSample * outBuffer = outSig.getBuffer() + outSig.plane(plane);
	void * trashBuffer = (void *)mTrashSignal.getBuffer();
	
	int mode = (int)getParam(modeSym);
	int frameSize = getParam(frameSym);
	int framesToRead = ml::min(frames, (int)outSig.getWidth());
	int framesAvailable = (int)PaUtil_GetRingBufferReadAvailable( &mBuf )/frameSize;
    
    // return if we have not accumulated enough signal.
	if (framesAvailable < framesToRead) return 0;
	
	// depending on trigger mode, trash samples up to the ones we will return.
	switch(mode)
	{
		default:
		case eMLRingBufferNoTrash:
		break;

		case eMLRingBufferMostRecent:			
			// TODO modify pa ringbuffer instead of reading to trash buffer. 
			PaUtil_ReadRingBuffer( &mBuf, trashBuffer, (framesAvailable - framesToRead)*frameSize );  
		break;
	}
	
	if(frameSize == 1)
	{
		// 1D read.
		lastRead = (int)PaUtil_ReadRingBuffer( &mBuf, outBuffer, framesToRead );
	}
	else
	{
		// 2D read.
		int height = outSig.getHeight();
		if(height < frameSize)
		{
			debug() << " MLProcRingBuffer::readToSignal: signal too small! ( frame size " << frameSize << ", height " << height << " )\n";
			return 0;
		}
		
		int stride = outSig.getRowStride();
		for(int i=0; i<framesToRead; ++i)
		{
			int avail = (int)PaUtil_GetRingBufferReadAvailable( &mBuf );
			if(avail >= frameSize)
			{
				// read to temp buffer
				PaUtil_ReadRingBuffer( &mBuf, mSingleFrameBuffer.getBuffer(), frameSize );
				
				// rotate into one column of destination 
				float* outPtr = outBuffer + i;		
				for(int j=0; j<frameSize; ++j)
				{
					outPtr[j*stride] = mSingleFrameBuffer[j]; 
				}
				outPtr++;
				lastRead++;
			}
		}
	}
	return lastRead;
}

