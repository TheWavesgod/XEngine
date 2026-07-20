# Shader System

## 1. Module Purpose

The Shader System owns shader compilation. The only shader compiler wired up
today is `SlangCompiler`, which calls the `slangc` executable distributed
under `ThirdParty/slang`. The Shader System caches the bytecode in
`D:/Project/XEngine/Saved/Cache/Shaders/Vulkan/*.spv` keyed by
`(Path, EntryPoint, Stage, Target)`.

The RHI consumes the resulting bytecode; the Renderer drives the
`ShaderSystem` indirectly through `RenderShaderLibrary` which calls
`ShaderSystem::Compile` and stores the produced `RHIShader`.

## 2. Responsibilities

- Provide `ShaderSystem : public ISubsystem`.
- Provide `ShaderCompiler` interface (abstract `Compile` and
  `IsAvailable`).
- Provide `SlangCompiler` concrete impl that shells out to `slangc`
  with the right CLI args for Vulkan SPIR-V.
- Hold compiler outputs in `ShaderModule` (stages, bytecode, reflection
  data, diagnostics).
- Surface compile errors back to callers as `ShaderCompileResult`.

## 3. Non-Responsibilities

- Does not import or manage shader assets (no `ShaderAsset` runtime
  usage; `ShaderAsset` header is a placeholder).
- Does not own GPU shader modules; `RHIShader` wraps a `VkShaderModule`
  and is created by `VulkanResourceFactory::CreateShaderImpl`.
- Does not hot-reload shaders.

## 4. Public API Surface

`Engine/Source/Runtime/Shader/Public/XEngine/Shader/`:

- `ShaderSystem.h` - `class ShaderSystem : public ISubsystem` with
  `m_Compiler`, `IsCompilerAvailable()`, `Compile(ShaderCompileDesc)`.
- `ShaderCompiler.h` - abstract `class ShaderCompiler`.
- `ShaderModule.h` - `struct CompiledShader { Stage, Target, Format,
  EntryPoint, SourcePath, Bytecode, SourceCode, Reflection, Result,
  Diagnostics, IsValid() }`.
- `ShaderReflection.h` - reflection data.
- `ShaderTypes.h` - enums `ShaderStage`, `ShaderTarget`,
  `ShaderCodeFormat`, `ShaderCompileResult`, `ShaderDefine`,
  `ShaderCompileDesc`.

Private:

- `Private/ShaderCompiler.cpp`, `Private/ShaderCache.cpp`,
  `Private/ShaderSystem.cpp`.
- `Private/Slang/SlangCompiler.cpp`, `Private/Slang/SlangReflection.cpp`.

## 5. Dependencies

### Depends on

- `XEngineFoundation`.
- `XEngineCoreRuntime` (include path).
- `ThirdParty/slang` (the `slangc` executable and `slang/include`).

### Used by

- `Runtime/RHI` (PUBLIC) - asks the shader system to compile bytecode.
- `Runtime/Renderer` (PUBLIC) - drives compilation through
  `RenderShaderLibrary`.

## 6. Ownership and Lifetime

- `ShaderSystem` is a single instance owned by the engine.
- `ShaderSystem::m_Compiler` is a unique_ptr constructed lazily inside
  `OnCreate` when `XENGINE_ENABLE_SHADER_COMPILER` is on.
- `CompiledShader` is a value type returned by `Compile`.

## 7. Runtime Flow

- `Engine::Initialize` registers `ShaderSystem` (gated by
  `EnableShaderCompiler` in `EngineConfig`).
- `RenderShaderLibrary::GetOrCreateShader(key)` calls
  `ShaderSystem::Compile` which dispatches to `SlangCompiler::Compile`.
- `SlangCompiler` writes the bytecode to
  `Saved/Cache/Shaders/Vulkan/<Path>.<Entry>.<Stage>.spv` and invokes
  `slangc` to produce the SPIR-V.

## 8. Important Invariants

- Shader cache files are content-addressable by file path; the renderer
  never invalidates them.
- `SlangCompiler` is the single active backend; other compilers would
  have to follow the `ShaderCompiler` interface.
- `CompiledShader::IsValid()` is the only valid check; on failure, the
  diagnostics string is populated.

## 9. Main Classes and Collaborators

- `ShaderSystem`.
- `ShaderCompiler` / `SlangCompiler`.
- `CompiledShader`.
- `RenderShaderLibrary` (in Renderer).

## 10. Design Rationale

- A single Shelling compiler process keeps the runtime light; SPIR-V is
  produced offline (or first-call) and cached as `.spv`.
- The abstract `ShaderCompiler` exists so other languages can be plugged
  in (e.g. SPIR-V cross-compiled from HLSL) without changing consumers.

### Alternatives considered

- In-memory caching only. Rejected: lose the cross-run cache advantage.
- Reflection-based automatic pipeline layout. Deferred - reflection
  output is captured in `CompiledShader::Reflection` but not yet
  consumed.

### Trade-offs

- Each compilation spawns `slangc`; very hot shaders pay a startup cost
  even with cache hits because process spawn is slow on Windows. A
  caching daemon would help but is out of scope for V0.

## 11. Failure Modes and Debugging

- Slang invocation failure leaves the `.spv` empty / stale; the
  ShaderLibrary logs and `IsValid()` reports false. Downstream pipeline
  creation will fail and the pass will skip.
- Reflection might be absent for some entry points; consumers must
  null-check.

## 12. Current Limitations

- No hot reload.
- Slang is the only wired compiler backend.
- Reflection data is generated but unused by the renderer.

## 13. Source References

- `Engine/Source/Runtime/Shader/Public/XEngine/Shader/*.h`
- `Engine/Source/Runtime/Shader/Private/Slang/*.cpp`
- `Engine/Source/Runtime/Shader/CMakeLists.txt`

## 14. Future Work

- Consume `CompiledShader::Reflection` to derive descriptor set layouts
  automatically.
- Add hot reload via filesystem watcher.
