#ifndef ASSERT_MACROS_H
#define ASSERT_MACROS_H

#include <cassert>

namespace CXL {

#define ASSERTM(cond, msg) assert(cond && msg)

} // namespace CXL

#endif // ASSERT_MACROS_H
