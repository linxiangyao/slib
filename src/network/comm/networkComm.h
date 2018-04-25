#ifndef S_NETWORK_COMM_H_
#define S_NETWORK_COMM_H_
#include <vector>
#include <stdio.h>
#include "../../socket/socketLib.h"
S_NAMESPACE_BEGIN

enum EUnpackResultType
{
	EUnpackResultType_ok,
	EUnpackResultType_fail,
	EUnpackResultType_needMoreData,
};

S_NAMESPACE_END
#endif
