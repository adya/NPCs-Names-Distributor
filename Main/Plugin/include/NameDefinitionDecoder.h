#pragma once

#include "NameDefinition.h"

namespace NND
{
	struct NameDefinitionDecoder
	{
		/// May throw
        NameDefinition decode(const std::filesystem::path& a_path);
    };
}
