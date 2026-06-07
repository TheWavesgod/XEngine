#pragma once

#include <XEngine/Core/Handle.h>
#include <XEngine/Renderer/MaterialTypes.h>

namespace XEngine
{
    // Runtime handle to a renderer-owned material record.
    // It is distinct from AssetHandle and does not identify CPU-side asset data.
    struct MaterialTag {};
    using MaterialHandle = Handle<MaterialTag>;
}
