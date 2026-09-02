# Reclaim M4 New-Canon Incremental Patch

This is an incremental source/config/documentation patch, not a full project archive.

## Included

```text
AGENTS.md
PROJECT_FRAMEWORK.md
CURRENT_USAGE_M0_M4.md

Source/Reclaim/Mutation/ReclaimMutationLoadComponent.h
Source/Reclaim/Mutation/ReclaimMutationLoadComponent.cpp

Source/Reclaim/AbilitySystem/Abilities/ReclaimMutationAbilities.h
Source/Reclaim/AbilitySystem/Abilities/ReclaimMutationAbilities.cpp

Source/Reclaim/Core/ReclaimM4CanonLibrary.h
Source/Reclaim/Core/ReclaimM4CanonLibrary.cpp

Tools/Editor/setup_m0_m4_new_canon.py
```

## Not included

- `.uasset`
- `.umap`
- formal art
- third-party/Fab content
- full project source copy

## Delete files?

No whole-file deletion is required.

Keep the old:

```text
Tools/Editor/setup_m0_m4_assets_fix5.py
```

The new setup script imports it as the proven editor-asset utility layer.

Old dormant assets such as ArcGun, Crawler-named BP paths and old M6/M7 placeholder data are intentionally not deleted because they may have serialized references. They are no longer canonical content unless a future milestone explicitly reuses/replaces them.

## Apply order

1. Back up / commit the project.
2. Extract this ZIP over the project root.
3. Compile `ReclaimEditor`.
4. Open Unreal Editor.
5. Run:

```text
py "<ProjectRoot>/Tools/Editor/setup_m0_m4_new_canon.py"
```

6. Verify 0 gameplay-critical errors.
7. Run the 1P / 2P / 4P M4 regression checklist.

## Important

Do not run `setup_m0_m4_assets_fix5.py` by itself after the new canonical setup and expect new-canon data to remain. Its old data table is intentionally preserved as a lower-level dependency; the new wrapper applies the current M4 canon on top during the same execution.

The patch does not claim an Unreal build was executed in this artifact-generation environment.
