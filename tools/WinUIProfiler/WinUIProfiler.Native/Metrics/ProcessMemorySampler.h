#pragma once
#include "..\Exports.h"

namespace wup
{
// Maps documented fields only; private_usage is commit, never resident memory.
void SetMemoryValues(WupMemoryResult& result, uint64_t working_set,
    uint64_t private_usage, const uint64_t* private_working_set);
}
