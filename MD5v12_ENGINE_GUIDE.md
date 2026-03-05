# MD5 Version 12 — Engine Implementation Guide

**Version:** 2.0
**Date:** March 2026

**Target files:**
- `neo/renderer/Model.h` — version constant
- `neo/renderer/Model_local.h` — class declarations
- `neo/renderer/Model_md5.cpp` — parser and loading
- `neo/renderer/tr_trisurf.cpp` — `R_BuildDeformInfo`
- `neo/renderer/tr_local.h` — `R_BuildDeformInfo` declaration
- `neo/d3xp/anim/Anim.cpp` — animation version gate

**No shader changes required.**

---

## Loading Pipeline Comparison

### v10 (current)

```
ParseMesh:
  1. Parse verts (UV, weight index, weight count)
  2. Parse tris
  3. Parse weights
  4. Build basePose positions from weights
  5. Pack joint indices/weights into color/color2
  6. R_BuildDeformInfo:
     a. Build silhouette data
     b. R_DuplicateMirroredVertexes
     c. R_BuildDominantTris (if unsmoothed tangents)
     d. R_DeriveTangents (compute N/T/B from geometry)
```

### v12 (new)

```
ParseMesh:
  1. Parse verts (UV, weight index, weight count, NORMAL, TANGENT)
  2. Parse tris
  3. Parse weights
  4. Parse optional vertex colors
  5. Build basePose positions from weights
  6. Transform stored normals/tangents: bone-local → model space
  7. Set N/T/B on basePose via SetNormal/SetTangent/SetBiTangentSign
  8. Pack joint indices/weights into color/color2 (same as v10)
  9. R_BuildDeformInfo (hasExplicitTangents=true):
     a. Build silhouette data (same — needed for shadows)
     b. R_DuplicateMirroredVertexes (same — copies our TBN to mirrors)
     c. SKIP R_BuildDominantTris
     d. SKIP R_DeriveTangents (tangents already set)
```

---

## Change Details

### 1. Model.h

```cpp
#define MD5_VERSION             10
#define MD5_VERSION_V12         12   // per-vertex normals, MikkTSpace tangents
```

### 2. Model_local.h

```cpp
// idMD5Mesh: change ParseMesh signature
void ParseMesh( idLexer& parser, int numJoints, const idJointMat* joints, bool isV12 );

// idRenderModelMD5: add member
bool isV12;
```

### 3. tr_local.h

```cpp
deformInfo_t* R_BuildDeformInfo( int numVerts, const idDrawVert* verts,
                                 int numIndexes, const int* indexes,
                                 bool useUnsmoothedTangents,
                                 bool hasExplicitTangents = false );
```

### 4. tr_trisurf.cpp — R_BuildDeformInfo

```cpp
deformInfo_t* R_BuildDeformInfo( int numVerts, const idDrawVert* verts,
                                 int numIndexes, const int* indexes,
                                 bool useUnsmoothedTangents,
                                 bool hasExplicitTangents )
{
    // ... existing setup, sil indexes, sil edges, mirrored verts ...

    if( hasExplicitTangents )
    {
        // v12: normals/tangents pre-set on input verts.
        // R_DuplicateMirroredVertexes already copied full idDrawVert.
        tri.tangentsCalculated = true;
    }
    else
    {
        if( useUnsmoothedTangents )
        {
            R_BuildDominantTris( &tri );
        }
        R_DeriveTangents( &tri );
    }

    // ... rest unchanged ...
}
```

### 5. Model_md5.cpp — LoadModel

```cpp
parser.ExpectTokenString( MD5_VERSION_STRING );
version = parser.ParseInt();

if( version != MD5_VERSION && version != MD5_VERSION_V12 )
{
    parser.Error( "Invalid version %d. Should be %d or %d\n",
                  version, MD5_VERSION, MD5_VERSION_V12 );
}
isV12 = ( version == MD5_VERSION_V12 );

// ... later ...
meshes[i].ParseMesh( parser, defaultPose.Num(), poseMat, isV12 );
```

### 6. Model_md5.cpp — ParseMesh

**Extended vertex parsing:**

```cpp
// After existing UV and weight index parsing:
idList<idVec3> vertNormals;
idList<idVec4> vertTangents;
if( isV12 )
{
    vertNormals.SetNum( count );
    vertTangents.SetNum( count );
}

// Inside vertex loop, after firstWeight/numWeights:
if( isV12 )
{
    parser.Parse1DMatrix( 3, vertNormals[i].ToFloatPtr() );
    parser.Parse1DMatrix( 4, vertTangents[i].ToFloatPtr() );
}
```

**Optional vertex colors (after weights, before closing brace):**

```cpp
if( isV12 )
{
    if( parser.CheckTokenString( "numvertexcolors" ) )
    {
        int numColors = parser.ParseInt();
        for( int i = 0; i < numColors; i++ )
        {
            parser.ExpectTokenString( "vertexcolor" );
            parser.ParseInt();
            idVec4 color;
            parser.Parse1DMatrix( 4, color.ToFloatPtr() );
            // Store for future use
        }
    }
}
parser.ExpectTokenString( "}" );
```

**Normal/tangent transform (after basePose positions, before weight packing):**

```cpp
if( isV12 )
{
    for( int i = 0; i < texCoords.Num(); i++ )
    {
        // Find dominant joint (highest weight)
        int bestJoint = 0;
        float bestWeight = 0.0f;
        int wnum = firstWeightForVertex[i];
        for( int j = 0; j < numWeightsForVertex[i]; j++ )
        {
            if( tempWeights[wnum + j].jointWeight > bestWeight )
            {
                bestWeight = tempWeights[wnum + j].jointWeight;
                bestJoint = tempWeights[wnum + j].joint;
            }
        }

        const idMat3 jointRot = joints[bestJoint].ToMat3();

        idVec3 modelNormal = jointRot * vertNormals[i];
        modelNormal.Normalize();

        idVec3 modelTangent = jointRot * vertTangents[i].ToVec3();
        modelTangent.Normalize();

        basePose[i].SetNormal( modelNormal );
        basePose[i].SetTangent( modelTangent );
        basePose[i].SetBiTangentSign( vertTangents[i].w );
    }
}
```

**R_BuildDeformInfo call:**

```cpp
deformInfo = R_BuildDeformInfo( texCoords.Num(), basePose, tris.Num(), tris.Ptr(),
                                shader->UseUnsmoothedTangents(),
                                isV12 );
```

### 7. Anim.cpp — LoadAnim

```cpp
parser.ExpectTokenString( MD5_VERSION_STRING );
int version = parser.ParseInt();
if( version != MD5_VERSION && version != MD5_VERSION_V12 )
{
    parser.Error( "Invalid version %d. Should be %d or %d\n",
                  version, MD5_VERSION, MD5_VERSION_V12 );
}
```

### 8. Binary Cache Versions

```cpp
// Model_md5.cpp
static const byte MD5B_VERSION = 107;  // was 106

// Anim.cpp
static const byte B_ANIM_MD5_VERSION = 102;  // was 101
```

---

## Why No Shader Changes

The shaders already do `cross(N, T) * T.w` for bitangent — the MikkTSpace formula.
`SetNormal()` / `SetTangent()` / `SetBiTangentSign()` pack into byte format,
shaders unpack with `* 2.0 - 1.0`. GPU skinning in `skinning.inc` transforms
position via `color`/`color2`. CPU skinning in `TransformVertsAndTangents()`
transforms normal/tangent via blended joint matrix and preserves `tangent[3]`
(bitangent sign). Both paths handle v12 data without modification.

---

## Vertex Colors — Future Work

Parsed but not rendered. `color`/`color2` are used for skinning. Options:
- New vertex attribute (`COLOR2`)
- Texture/SSBO indexed by vertex ID
- Bake into diffuse texture offline
