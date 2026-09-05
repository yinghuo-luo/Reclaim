# Reclaim C++ + TypeScript + Blueprint baseline

Baseline branch: `baseline/puerts-v8-94-20260905`

Base project commit: `ce3de900faa6a87b1f6cce4951b66e70c6f93cfa` (`0905`, 2026-09-05)

## Architecture boundary

- **C++** remains the authoritative gameplay/runtime layer: networking, RPC validation, replication, damage, GAS execution, inventory/mission state, weapon traces/projectiles, AI perception/navigation/action execution, and animation runtime state.
- **TypeScript** is the hot-iteration policy/orchestration layer. It receives reflected UObject references/events and emits command intents back to C++/Blueprint for validation/execution.
- **Blueprint/DataAsset** remains the asset/configuration/presentation layer. Existing DataAssets are passed to TS as reflected UObject references; no duplicate JSON configuration database is introduced.

The runtime owns one `puerts::FJsEnv` per `UGameInstance` through `UReclaimScriptRuntimeSubsystem`. Blueprint actors can opt into scripting with `UReclaimScriptBindingComponent`.

Initial policy IDs:

- `Player.Character`
- `AI.Enemy`
- `Weapon.Runtime`

See `Docs/SCRIPTING_ARCHITECTURE.md` for the integration contract.

## PuerTS dependency

The repository intentionally does **not** vendor a PuerTS/V8 binary package. Copy the PuerTS plugin and V8 backend you already downloaded into:

```text
Reclaim/Plugins/Puerts/
Reclaim/Plugins/Puerts/ThirdParty/v8_9.4.146.24/
```

Then run:

```powershell
.\Scripts\Puerts\ConfigurePuertsV8_94.ps1
```

The project expects the PuerTS `JsEnv` module and the V8 9.4.146.24 backend (`SupportedV8Versions.V9_4_146_24`). NodeJS and QuickJS are not part of this baseline.

## TypeScript build

Source lives under `TypeScript/Reclaim`; compiled CommonJS is staged under `Content/JavaScript/Reclaim`.

```powershell
.\Scripts\BuildTypeScript.ps1
```

`Config/DefaultGame.ini` stages `Content/JavaScript` as UFS for packaged builds.

## Blueprint wiring

Add `Reclaim Script Binding Component` to the Blueprint class that needs a policy, set `ScriptId`, and optionally assign an existing DataAsset to `ScriptConfig`.

For gameplay policies keep `ExecutionPolicy = Authority Only`. Client-only presentation policies may explicitly use `Non Authority Only`.

`OnScriptCommand` is an **intent channel**, not an authority bypass. Validate the command in existing C++/Blueprint gameplay code before mutating authoritative state.
