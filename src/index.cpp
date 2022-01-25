#include "index.h"

using namespace mii;
using namespace std;

Index::Index() {};

Index::Index(istream& inp)
{
}

void Index::import(std::string mpath)
{

}

namespace mii { // needed for overload
ostream& operator<<(ostream& lhs, const Index& rhs)
{
    return lhs;
}
} // namespace mii