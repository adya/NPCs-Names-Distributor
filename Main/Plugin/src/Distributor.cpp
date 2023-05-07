#include "Distributor.h"
#include "LookupNameDefinitions.h"

namespace NND
{
	namespace Distribution
	{
		NameRef DistributedName::GetNameInScope(NameScope scope) {
			switch (scope) {
			default:
			case kName:
				return name;
			case kTitle:
				return title;
			case kObscurity:
				return obscurity;
			}
		}
	}
}
