# RHI Cleanup Stages 5–8 — Design Review

## Review Baseline

Reviewed against:

- `Docs/Project_Cache.md` and `Docs/plan.md`;
- completed Stage 1–4 implementation documents;
- the checked-in RHI/Renderer/Editor source on 2026-06-27;
- `Docs/AI/Stage_09_CSM_Implementation_Plan.md`, especially its explicit
  `ShadowResourceCache` ownership model.

The source is currently between Stages 4 and 5: public view fields have partly
landed, while several Vulkan and Renderer callers still use the old texture
path. The revised documents use that state as the migration baseline.

## Decisions

| Finding | Impact | Resolution |
|---|---|---|
| General `RHITexture` subresource-view cache has hidden lifetime, no eviction boundary, and duplicates Stage 9 ownership | High | Removed. Semantic renderer owners retain `shared_ptr<RHITextureView>` |
| Texture base cache can complicate `VkImageView`/`VkImage` destruction order | High | Views are explicitly reset/destroyed before their source texture |
| View-based attachment draft still transitioned only mip 0/layer 0 while tracking one layout per texture | High | Stage 5 conservatively transitions the whole texture until subresource tracking exists |
| Depth-only draft required a fragment shader | High | Fragment shader is optional when no color attachment exists |
| Depth bias was written into `VkPipelineDepthStencilStateCreateInfo` | Build blocker | Moved to `VkPipelineRasterizationStateCreateInfo` |
| Pipeline cache keyed only the depth-bias enable flag | High | All baked bias values are included in equality/hash |
| Stage 7 deferred-deletion queue was not fence-aware | High | Removed from this cleanup; deferred destruction requires frame-fence integration |
| Several Vulkan capability mappings conflated unrelated limits | High | Replaced with a minimal set mapped directly from real properties/enabled features |
| Fixed descriptor arrays were treated as bindless | Medium | Fixed arrays remain valid; descriptor indexing is a separate future feature |
| Stage 8 proposed private Vulkan concrete casts from Editor | Medium | Keep a narrow public native-interop escape hatch for the Vulkan ImGui bridge |
| Documents mixed `shared_ptr`/`unique_ptr`, `RHIPipeline`/`RHIGraphicsPipeline`, and several attachment field names | Build blocker | Standardized on current source names and `shared_ptr` |
| Device-level sampled-texture update API was shadow-specific | Medium | Prefer rebuilding the infrequently changed frame bind group; otherwise design a generic validated update service |

## Resulting Boundary

```text
RHIResourceFactory
  └─ creates textures and views; validates/normalizes descriptors

RHITexture
  └─ owns the image only

Renderer feature/resource owner
  └─ owns views according to semantic lifetime and reuse policy

VulkanCommandList / VulkanDescriptor
  └─ borrows views, resolves their source images, records native use

Future RenderGraph
  └─ may add frame-aware view interning; no legacy RHI cache to unwind
```

## Revised Documents

- [Stage 5](RHI_Cleanup_05_ViewBasedRenderPass_And_BindGroup.md): explicit
  ownership, view consumers, and whole-image V0 layout rule.
- [Stage 6](RHI_Cleanup_06_DepthOnlyPipeline.md): correct Vulkan depth-only
  and depth-bias state.
- [Stage 7](RHI_Cleanup_07_Capabilities_Validation_DebugNames.md): truthful
  capabilities, validation, and debug names without unsafe deferred deletion.
- [Stage 8](RHI_Cleanup_08_Migration_Checklist.md): current-source migration,
  default-view removal, native interop, and final audits.

## Required Stage Gates

Do not start the next stage until the current gate passes:

1. Stage 5: normal materials and editor targets use explicitly owned views;
   a temporary array/layer-view test passes Vulkan validation.
2. Stage 6: a vertex-only depth pipeline records zero color attachments.
3. Stage 7A: invalid descriptor tests fail before backend creation; capabilities
   match the selected physical/logical device.
4. Stage 7B: RenderDoc shows correct names/object types.
5. Stage 8: no texture default view/cache or transitional creation call remains;
   Editor and Sandbox smoke tests pass.

Only after all five gates should Stage 9 treat CSM as Renderer-only work.
