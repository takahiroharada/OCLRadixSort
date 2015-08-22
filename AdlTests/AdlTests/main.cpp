/*
		2011 Takahiro Harada
*/

#include <stdio.h>


#include <Adl/Adl.h>
#include <AdlPrimitives/Math/Math.h>

using namespace adl;

#include "UnitTests.h"
#include "RadixSortBenchmark.h"


#undef NUM_TESTS


int main()
{
	{
		runAllTest();
	}
	{
		radixSortBenchmark<TYPE_CL>();
		radixSortBenchmark<TYPE_DX11>();
	}
}


