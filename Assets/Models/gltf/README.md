# glTF Validation Assets

This folder contains validation glTF assets for XEngine Stage 7E.

Current assets:
- Cube with texture
- DamagedHelmet

Stage 7A:
- AssetSystem may register metadata for files in this folder.
- No glTF parsing is performed yet.

Stage 7E:
- GltfImporter V0 imports these assets using fastgltf 0.9.
- Imported data becomes TextureAsset, MeshAsset, and MaterialAsset CPU-side assets.
- SceneAsset and scene hierarchy integration are intentionally deferred to Stage 7F.

Notes:
- Keep original license and README files for each model when available.
- Do not commit entire sample asset repositories.
- These assets are validation assets, not engine source code.
