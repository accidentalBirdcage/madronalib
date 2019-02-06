//
//  MLProjection.cpp
//  Virta
//
//  Created by Randy Jones on 12/27/15.
//
//

#include "MLProjection.h"

namespace ml
{	
	TableProjection::TableProjection(std::initializer_list<float> values)
	{
		for(float f : values)
		{
			mTable.push_back(f);
		}
	}

	float TableProjection::operator()(float f) const
	{
		float ni = mTable.size() - 1;
		float nf = static_cast<float>(ni);
		float xf = nf*clamp(f, 0.f, 1.f);		
		int xi = static_cast<int>(xf);
		float xr = xf - xi;
		
		if(ni > 0)
		{
			if(f < 1.0f)
			{
				return lerp(mTable[xi], mTable[xi + 1], xr);
			}
			else
			{
				return mTable[ni];
			}
		}
		else if(ni == 0)
		{
			// one item in table
			return mTable[0];
		}
		else
		{
			// empty table
			return 0.f;
		}
	}
}
