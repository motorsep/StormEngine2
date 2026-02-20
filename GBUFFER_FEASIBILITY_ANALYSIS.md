# G-Buffer Feasibility Analysis for StormEngine2

## Executive Summary

Adding a slim G-buffer to StormEngine2 for SSAO and SSR is **feasible without overhauling the renderer**, but requires careful insertion of a new pass and modifications to existing shaders. The engine already has the infrastructure needed: MRT support (up to 8 color attachments), a framebuffer management system, an existing depth pre-pass, and a post-processing pipeline with HDR support. The main work involves generating view-space normals into a new render target during the depth pre-pass and inserting screen-space effect passes at the right points in the pipeline.

---

## Current Renderer Architecture

### Rendering Pipeline (Forward Renderer)

The pipeline in `RB_DrawViewInternal()` (`neo/renderer/tr_backend_draw.cpp:3961`) follows this order:

```
1. Clear depth/stencil
2. RB_FillDepthBufferFast()          -- depth pre-pass (all opaque geometry)
3. CopyDepthbuffer()                 -- depth copied to currentDepthImage texture
4. [Glow pass: SS_POST_PROCESS shader passes, copy to glow buffer]
5. RB_DrawInteractions()             -- per-light forward lighting (additive blending)
6. RB_DrawShaderPasses()             -- non-light-dependent passes (emissive, etc.)
7. RB_FogAllLights()                 -- fog/blend lights
8. RB_DrawShaderPasses()             -- transparent surfaces
9. CopyFramebuffer -> currentRenderImage
10. RB_DrawShaderPasses()            -- post-process material stages (heat haze, etc.)
11. [RB_MotionBlur()]                -- motion blur (uses depth + prev frame MVP)
12. [RB_PostProcess()]               -- HDR tonemapping + bloom composite to system FB
```

### Key Existing Assets

| Asset | Format | Purpose |
|-------|--------|---------|
| `viewFramebuffer` | Color: RGBA16F, Depth: D24S8 | Main HDR scene rendering |
| `viewFramebufferDepthImage` | Depth24Stencil8 | Shared depth-stencil |
| `currentDepthImage` | Depth (nearest) | CPU-side copy after depth pre-pass |
| `currentRenderImage` | RGBA8 (linear) | CPU-side copy for post-process reads |
| `glowFramebuffer8/16[4]` | RGBA8 / RGBA16F | Bloom blur chain |
| `shadowMapFramebuffer[5][6]` | Depth array | Cascaded shadow maps |

### MRT Support

Fully implemented in `idFramebuffer` (`neo/renderer/Framebuffer.h:53`):
- Up to 8 color attachments via `SetColorAttachment(int index, idImage*)`
- `glDrawBuffers()` called dynamically based on attachment count (`gl_Framebuffer.cpp:122`)
- Fragment shaders already support 4 named outputs: `fo_Color` (COLOR0), `COLOR1`, `COLOR2`, `COLOR3` (`RenderProgs_GLSL.cpp:127-133`)

### Vertex Data Available

From `idDrawVert` (`neo/idlib/geometry/DrawVert.h:114`):
- `xyz` (vec3) - position
- `st` (half2) - texture coordinates
- `normal` (byte4, bias-encoded) - vertex normal
- `tangent` (byte4, bias-encoded, w = binormal sign) - tangent + TBN handedness
- `color` (byte4) - vertex color
- `color2` (byte4) - skinning weights

All attributes are already bound as vertex inputs (`in_Position`, `in_Normal`, `in_Tangent`, `in_TexCoord`, `in_Color`, `in_Color2`).

---

## What a Slim G-Buffer Needs

For SSAO and SSR, the minimum data required is:

| Channel | Content | Format | Notes |
|---------|---------|--------|-------|
| **Depth** | Hardware depth | D24S8 | **Already available** via `viewFramebufferDepthImage` and `currentDepthImage` |
| **Normals** | View-space normals | RG16F or RGB10A2 | New - must be generated |

Optional but beneficial for higher-quality SSR:

| Channel | Content | Format | Notes |
|---------|---------|--------|-------|
| **Roughness** | Material roughness | R8 (pack into normal.a or separate) | Could pack into normal buffer alpha |
| **Motion vectors** | Per-pixel velocity | RG16F | For temporal filtering; engine already computes prev-frame MVP for motion blur |

---

## Feasibility Assessment: Approach-by-Approach

### Approach A: Extend the Depth Pre-Pass with MRT (Recommended)

**Concept**: Modify `RB_FillDepthBufferFast()` to render into an MRT framebuffer that writes both depth and view-space normals simultaneously.

**Why it works**:
- The depth pre-pass (`tr_backend_draw.cpp:963`) already iterates over all opaque geometry
- It already binds `BindShader_Depth()` / `BindShader_DepthSkinned()` per surface
- The depth shader (`depth_vertex.glsl`) currently only transforms position via MVP -- it does NOT pass normals
- The depth fragment shader (`depth_fragment.glsl`) outputs `vec4(0,0,0,1)` -- it discards color entirely

**Required changes**:

1. **New images and framebuffer** (in `Image_intrinsic.cpp` and `gl_Framebuffer.cpp`):
   - Create `gbufferNormalImage` (RG16F or RGBA8, screen-sized)
   - Create `gbufferFramebuffer` with:
     - Color0: `gbufferNormalImage`
     - DepthStencil: `viewFramebufferDepthImage` (shared with view FB)

2. **New depth+normal shaders** (4 variants):
   - `depth_gbuffer.vertex` / `depth_gbuffer.pixel` (static mesh)
   - `depth_gbuffer_skinned.vertex` / `depth_gbuffer_skinned.pixel`
   - Vertex shader: pass `in_Normal` transformed to view-space via model-view matrix
   - Fragment shader: output `fo_Color = vec4(encode(viewNormal), 1.0)`

3. **Modify `RB_FillDepthBufferFast()`** (`tr_backend_draw.cpp:963`):
   - Bind `gbufferFramebuffer` instead of the default framebuffer at the start
   - Use the new `BindShader_DepthGBuffer()` variants instead of `BindShader_Depth()`
   - After depth fill completes, switch back to `viewFramebuffer`

4. **New SSAO/SSR post-process passes** (inserted after depth copy at line ~4064):
   - SSAO pass: reads `gbufferNormalImage` + `currentDepthImage`, outputs AO term
   - Apply AO as a modulation during interaction pass or as a post-multiply
   - SSR pass: reads normals + depth + `currentRenderImage` (after lighting), outputs reflections

5. **New built-in shader enums** in `RenderProgs.h`:
   - `BUILTIN_DEPTH_GBUFFER`, `BUILTIN_DEPTH_GBUFFER_SKINNED`
   - `BUILTIN_SSAO`, `BUILTIN_SSR`

**Impact on existing code**: Minimal. The depth pre-pass logic is self-contained. The `viewFramebuffer` and interaction passes remain unchanged. SSAO/SSR are additive post-process passes.

**Performance cost**:
- Depth pre-pass: slight increase from writing normals (extra MRT output). Modern GPUs handle this with negligible overhead since the geometry is already being processed
- SSAO: one full-screen pass (+ optional blur pass)
- SSR: one full-screen pass (most expensive part)
- Memory: one screen-sized RG16F texture (~12 MB at 1920x1080)

### Approach B: Reconstruct Normals from Depth (No G-buffer)

**Concept**: Skip the normal buffer entirely. Reconstruct normals from the depth buffer using `dFdx`/`dFdy` (screen-space derivatives) in the SSAO shader.

**Pros**:
- Zero changes to the depth pre-pass
- No extra render targets
- No new vertex/fragment shader variants for depth

**Cons**:
- Reconstructed normals are flat per-triangle (no normal maps), producing blocky SSAO on curved surfaces
- Quality is significantly lower than using actual normals
- Not usable for SSR at all (SSR requires smooth, accurate normals for ray direction)

**Verdict**: Acceptable for quick-and-dirty SSAO only, not for SSR.

### Approach C: Full Deferred G-Buffer

**Concept**: Switch to a full deferred rendering pipeline with albedo + normal + specular + roughness G-buffer.

**Verdict**: This IS an overhaul. Would require rewriting the entire interaction/lighting system from forward per-light passes to a deferred resolve. Not recommended under the constraint of avoiding renderer redesign.

---

## Recommended Implementation Plan (Approach A)

### Phase 1: Normal Buffer Generation

```
Files to modify:
  neo/renderer/Image.h                        -- add gbufferNormalImage declaration
  neo/renderer/Image_intrinsic.cpp            -- create gbufferNormalImage (RG16F)
  neo/renderer/Framebuffer.h                  -- add gbufferFramebuffer pointer
  neo/renderer/OpenGL/gl_Framebuffer.cpp      -- allocate gbufferFramebuffer
  neo/renderer/RenderProgs.h                  -- add BUILTIN_DEPTH_GBUFFER enums
  neo/renderer/RenderProgs.cpp                -- register new shader programs
  neo/renderer/tr_backend_draw.cpp            -- modify RB_FillDepthBufferFast()

New files:
  base/renderprogs/depth_gbuffer.vertex       -- depth + normal output vertex shader
  base/renderprogs/depth_gbuffer.pixel        -- normal encoding fragment shader
  base/renderprogs/glsl/depth_gbuffer_vertex.glsl
  base/renderprogs/glsl/depth_gbuffer_fragment.glsl
  (+ skinned variants)
```

### Phase 2: SSAO

```
New files:
  base/renderprogs/ssao.vertex                -- fullscreen quad vertex shader
  base/renderprogs/ssao.pixel                 -- SSAO fragment shader
  base/renderprogs/ssao_blur.pixel            -- bilateral blur for SSAO
  base/renderprogs/glsl/ variants

Files to modify:
  neo/renderer/tr_backend_draw.cpp            -- insert SSAO pass after depth copy
  neo/renderer/Image.h / Image_intrinsic.cpp  -- SSAO result texture
  neo/renderer/Framebuffer.h / gl_Framebuffer.cpp -- SSAO framebuffer
  neo/renderer/RenderProgs.h / .cpp           -- BUILTIN_SSAO, BUILTIN_SSAO_BLUR
```

SSAO application options:
- **Option 1**: Multiply AO into the ambient interaction pass (modify `interactionAmbient` shader to sample AO texture). Most physically correct.
- **Option 2**: Apply as a full-screen post-multiply after all lighting. Simpler but affects direct lighting too.

### Phase 3: SSR

```
New files:
  base/renderprogs/ssr.pixel                  -- screen-space ray march
  base/renderprogs/glsl/ssr_fragment.glsl

Files to modify:
  neo/renderer/tr_backend_draw.cpp            -- insert SSR pass after lighting
  neo/renderer/Image.h / Image_intrinsic.cpp  -- SSR result texture
```

SSR requires reading the lit scene, so it must run after `RB_DrawInteractions()` and after `currentRenderImage` is captured. It composites reflections on top.

---

## Potential Complications and Mitigations

### 1. Perforated surfaces in depth pre-pass
The depth pre-pass handles perforated (alpha-tested) surfaces separately via `RB_FillDepthBufferGeneric()`. This function uses the full material shader setup. For these surfaces, the G-buffer normal write would need to either:
- Use a special shader that handles alpha test AND writes normals (preferred)
- Skip normal output for perforated surfaces (acceptable -- these are typically fences, grates, etc.)

### 2. Depth buffer sharing between FBOs
The `viewFramebufferDepthImage` can be attached to multiple FBOs simultaneously (one at a time for rendering). The G-buffer FBO would share this depth attachment with `viewFramebuffer`, so depth written during the G-buffer pass is automatically available for the interaction pass's `GLS_DEPTHFUNC_EQUAL` test. This is already how shadow mapping FBOs work in the engine.

### 3. Translucent surfaces
Translucent surfaces don't write depth and therefore won't have normals in the G-buffer. SSAO and SSR should naturally handle this -- they only affect opaque geometry that wrote depth.

### 4. Subviews/mirrors
Subviews render recursively before the main view. Each subview would need its own G-buffer pass if SSAO/SSR should apply there. Initially, it's reasonable to skip SSAO/SSR for subviews.

### 5. Model-view matrix availability during depth pass
The current depth shader only uses MVP. For view-space normals, we need the model-view matrix. This is already available as a render parm (`RENDERPARM_MODELVIEWMATRIX_X/Y/Z/W`) and is set up during interaction rendering. We would need to also set it during the depth pass for each surface's space change.

### 6. Editor modes
The engine skips HDR and glow in editor mode (`com_editors`). SSAO/SSR should follow the same pattern.

---

## Conclusion

Adding a slim G-buffer (depth + view-space normals) is well within the engine's existing capabilities:

- **MRT infrastructure**: Already built and working (8 color attachments, dynamic `glDrawBuffers`)
- **Framebuffer management**: Clean API for creating new FBOs with shared depth
- **Depth pre-pass**: Already iterates all opaque geometry -- just needs normal output added
- **Post-processing pipeline**: Already has HDR FBO -> system FB resolve with fullscreen quad passes
- **Shader system**: Supports adding new built-in shaders with minimal boilerplate
- **Vertex data**: Normals and tangents already present in vertex format

The changes are additive and modular. The forward lighting pipeline (interaction passes) remains completely untouched. The only existing function that needs modification is `RB_FillDepthBufferFast()`, and even that change is a straightforward FBO bind swap and shader swap.

**Risk level**: Low-to-moderate. The main integration risk is ensuring the shared depth buffer works correctly when switching between the G-buffer FBO and the view FBO, but the engine already does this pattern with shadow map FBOs.
