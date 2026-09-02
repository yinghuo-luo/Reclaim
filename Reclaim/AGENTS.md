# AGENTS.md — Project Reclaim / 净界计划

> UE 5.6 · Windows PC · 1–4 人合作 PVE FPS/Roguelite · Listen Server
>
> This file is the durable project contract for Codex. Exact class/file layout, milestone scope, data models and acceptance gates are defined in `PROJECT_FRAMEWORK.md`. Read both before structural work.

## 1. Instruction priority

Apply instructions in this order:

1. Current explicit user request.
2. This `AGENTS.md`.
3. `PROJECT_FRAMEWORK.md`.
4. Existing intentional code/tests/config if newer than the documents.
5. Agent inference.

Do not silently reconcile a conflict that changes authority, persistence, replication, or public architecture. Report it first.

The central rule is:

```text
Configuration, decision, execution, authority, persistence and presentation
must each have a clear primary owner.
```

---

## 2. Current project canon and Demo contract

```text
Engine            Unreal Engine 5.6
Platform          Windows PC
Players           1–4
Network           Listen Server; 1 Host + 0–3 Clients
Flow              Main Menu → Host/Join → Lobby → Role Select → Ready
                  → Mission → Result → Lobby
Mission           末世森林隔离区 / 清除繁殖巢
Run target        15–20 minutes
Roles             Vanguard / Ranger / Engineer / Warden
Role rule         Same role cannot be selected twice in one Lobby
Authority         Server authoritative gameplay
Solo              Same multiplayer path; one-player Listen Server
Player identity   Adapter / 适应者行动小队
Combat identity   realistic firearms as sustained primary damage;
                  mutation abilities/devices as limited tactical tools
Roguelite         functional-organ samples → Field Purifier three-choice
                  → personal adaptation build
Ability limiter   Mutation Load 0–100
```

P0 includes:

- four roles, two core abilities each;
- Assault Rifle / Tactical Shotgun;
- Grenade Launcher as the P0 special weapon;
- Auto Turret / portable protection device;
- three base enemies: Hound / Spitter / Brute plus one Elite;
- Health 100 and Protection 75 using the already validated two-layer combat structure;
- Mutation Load with server authority;
- Downed / Revive / Emergency Reinsertion;
- server Enemy Director scaled by effective player count;
- three activity points, Breeding Den, extraction, optional Apex Hunt;
- team-shared temporary resources: Scrap / Electronics / Chemical / Tissue;
- personal organ/adaptation choices and personal run build;
- extraction settlement, Bio Lab / Armory, local SaveGame meta progression.

Do not add these unless explicitly requested:

```text
Dedicated Server
automatic matchmaking
cross-platform
Host Migration
Mid-Run Join
mission reconnect restore
voice chat
anti-cheat/backend accounts
cloud save
open world/runtime procedural terrain
formal Boss for the Demo
second biome
full 3D base
ultimate skills/full class trees
large permanent economy/tech tree
```

---

## 3. M0–M4 compatibility rule

M0–M4 is one stable programming baseline. The new world/background is part of that baseline, not a separate transition milestone.

Do not rewrite already validated systems merely to modernize old internal names:

```text
Session / Lobby / Travel
replicated CharacterMovement
M2 weapon authority and damage
M3 role/co-op RPC and life-state flow
M4 server AI / Director
```

When an old serialized/internal identifier such as `Robot`, `Shield`, `Crawler`, `PurificationPulse`, `RewardTerminal`, `Alloy`, or similar is referenced by existing Blueprint/C++/Save data, preserve the identifier unless a safe repository-wide migration is explicitly requested.

For all player-visible text, new Data Assets, new Gameplay Tags, documentation, tuning and future code, use the current canon directly:

```text
Adapter / 适应者
Protection / 防护
Mutation Load / 变异负荷
Hound / Spitter / Brute / Elite
Organ Sample / 功能器官样本
Field Purifier / 便携纯化
Scrap / Electronics / Chemical / Tissue
Breeding Den / 主巢
Apex Hunt
Bio Lab / Armory
```

An old internal asset path is an implementation detail, not a second design mode.

No formal art is required for M0–M4. Do not fabricate `.uasset` or `.umap` binaries. Missing SkeletalMesh / AnimBP / Niagara / material / sound is a presentation warning, not an M4 gameplay failure.

---

## 4. Codex working rules

Before a non-trivial change:

1. Read this file and the relevant `PROJECT_FRAMEWORK.md` sections.
2. Inspect existing implementation before creating abstractions.
3. Identify the authoritative owner of every changed state.
4. Check replication, travel, SaveGame and Blueprint impact.
5. Implement the smallest coherent change.
6. Compile/test in increments when UE is available.

If Unreal Engine is unavailable in the environment, do not claim a build passed. Report the exact build/test command that remains to run.

Never fabricate or hand-edit `.uasset` or `.umap` files. If editor-created assets are required, implement the native/config contract and provide exact asset setup steps.

Never edit third-party Fab/Marketplace assets in place. Create project-owned children/material instances under `Content/Reclaim`.

Do not use destructive Git commands, discard user changes, mass-delete content, or perform broad renames without explicit need.

Prefer Unreal built-ins for P0: Enhanced Input, GAS, Gameplay Tags, OnlineSubsystem Null/LAN, AI/Navigation, UMG, SaveGame.

---

## 5. Non-negotiable authority boundaries

### GameInstance / SessionSubsystem

Owns:

- Create / Find / Join / Destroy session;
- session delegates;
- lightweight cross-map session metadata.

Does not own world Actor pointers or mission combat truth.

### Lobby GameMode

Server-only authority for:

- role reservation;
- Ready;
- Start eligibility;
- travel initiation.

### Lobby GameState

Replicated read model for:

- role occupation;
- player list;
- Ready summary;
- lobby-visible session state.

### PlayerState

Owns replicated data that must outlive Pawn replacement / travel as appropriate:

```text
PlayerSlot
SelectedRole
Ready
SubmittedLoadout summary
LifeState summary
Emergency Reinsertion count
run statistics
personal RunInventory component/read model
```

### PlayerController

Owns local UI, Enhanced Input routing, client request entry points and session/menu control.

Never use `GetPlayerController(0)` as universal gameplay context.

### Player Character

Replicated Adapter body with CharacterMovement, GAS/attributes, health/protection coordinator, Mutation Load, weapon/equipment and interaction capabilities.

Role Blueprint children stay thin and share common network logic.

---

## 6. Lobby role invariants

Roles:

```text
Vanguard
Ranger
Engineer
Warden
```

Mandatory rules:

1. A non-empty role has at most one Lobby owner.
2. Client sends only a request.
3. Server validates and commits.
4. Concurrent requests resolve in server processing order; only one succeeds.
5. `Ready=true` locks role switching.
6. Switching is atomic: validate target first, then release old + claim new in one server transaction.
7. Failed switch preserves old role.
8. Leaving Lobby releases role immediately.
9. Travel/Mission locks role selection.
10. `SelectedRole` persists through PlayerState for mission spawn.
11. Returning to Lobby preserves valid role but resets Ready.
12. All clients converge on the same reservation read model.

Never release the old role before confirming the target role is available.

---

## 7. Session/travel rules

```text
MainMenu
→ Lobby          Join allowed
→ Travelling     Join disabled
→ Mission        Join disabled
→ Result
→ Lobby Return   Join allowed
```

Use Host-driven server travel. Preserve required PlayerState data across travel. Do not store world Actor pointers in GameInstanceSubsystem.

Host leaving terminates the Demo session; clients return to menu/error flow. No Host Migration.

Do not partially implement Mid-Run Join.

---

## 8. Skill / GAS pattern

Use:

```text
Ability Definition DataAsset
        ↓
GameplayAbility execution family
        ↓
Montage / Notify / GameplayEffect / spawned Actor / GameplayCue
```

Responsibilities:

```text
DataAsset       configuration
GameplayAbility runtime lifecycle and server-side validation
MutationLoad    shared mutation ability cost/overload policy
Montage         animation timing
Notify          timing events
GameplayEffect  suitable attribute/state modification
GameplayCue     result presentation
Character       capabilities, not per-skill config
```

The eight role skills should mainly be configurations/subclasses of a small set of execution families. Do not create a bespoke native class solely because a cooldown, radius, label or VFX differs.

M4 role semantics:

```text
Vanguard  Muscle Burst / 肌纤维爆发
Vanguard  Keratin Hardening / 角质硬化

Ranger    Hunter Sense Scan / 猎感扫描
Ranger    Tendon Leap / 肌腱跃迁

Engineer  Support Drone / 支援无人机
Engineer  Engineering Overload / 工程过载

Warden    Regeneration Pulse / 再生脉冲
Warden    Stabilizer Field / 稳定剂场
```

The current implementation may reuse older execution actors/modes internally when their validated network lifecycle matches the new effect.

---

## 9. Mutation Load rules

Mutation Load is server authoritative.

Baseline:

```text
Range                 0–100
Safe                  0–69
High                  70–99
Overload              reaches 100
Overload consequence  mutation abilities temporarily locked
Gun consequence       none; firearms remain usable
Recovery              server-owned, timer/event based
```

Client UI may predict/display feedback but must not directly reduce or spend Mutation Load.

Warden Stabilizer Field may reduce mutation-load growth and improve recovery through server-owned effects/state.

Do not make permanent infection, forced transformation or long-term character-body mutation part of Demo P0.

---

## 10. Weapon/network combat rules

Weapon input may feel immediate locally, but legal fire cadence, ammo, hit result and damage are server authoritative.

P0 canonical weapons:

```text
Assault Rifle      Hitscan
Tactical Shotgun   deterministic multi-Hitscan
Grenade Launcher   server-spawned replicated projectile
```

Arc Gun is not P0 canonical content. If an old Arc Gun DataAsset remains for compatibility, it must stay dormant unless explicitly enabled later.

For hitscan:

1. owner may immediately play local muzzle/recoil/tracer cosmetics;
2. client sends a compact shot request/sequence;
3. server validates owner, life state, equipped weapon, ammo, fire interval, mission state and aim data;
4. server performs authoritative trace/spread;
5. server applies damage and updates ammo;
6. durable state replicates; result cues/presentation follow.

For shotgun, accepted spread must be deterministic. Do not trust a free-form client-provided seed as authority.

For projectiles, server spawns the damaging replicated projectile. Client-only prediction must never apply damage.

Do not synchronize firing state with per-frame Multicast.

---

## 11. Health / Protection / Downed / Revive / Reinsertion

Baseline:

```text
Health                 100
Protection             75
Emergency Reinsertion    1 per player per run
```

Existing internal `Shield` attribute/component names may remain if they are already serialized and validated; player-facing semantics are Protection / 防护.

If Health/Protection use GAS Attributes, the coordinator component is not a second independent numeric database.

Life-state flow remains server authoritative:

```text
Active
→ Downed
   ├─ Revive → Active
   └─ timeout → Critical/Destroyed
                ├─ reinsertion available → Reinserting → Active(new Pawn)
                └─ none → unavailable for combat
```

Narrative meaning of reinsertion is emergency medical recovery/redeployment, not robot rebuild or supernatural resurrection.

Global team wipe is evaluated by Mission GameMode/mission authority, not by one Pawn.

---

## 12. Run inventory, resources, organ rewards, save

### Team resources

Temporary mission resources are server-owned in Mission GameState:

```text
Scrap
Electronics
Chemical
Tissue
```

Any valid pickup updates one team pool replicated to all players.

### Personal run build

Organ adaptations, weapon modifiers and personal build state are personal. Prefer Owner Only replication for private RunInventory state.

A player's adaptation selection must never mutate another player's inventory.

### Deterministic organ candidate generation

Inputs include:

```text
RunSeed
PlayerSlot
SampleType
RewardIndex
candidate pool / role affinity / current build filters
```

Output is a legal unique candidate set. Server confirms the selected adaptation.

### Persistence

Demo uses local personal SaveGame outside the mission. The owning player writes local persistence only from server-confirmed settlement/research results.

---

## 13. Enemy / Director rules

P0 archetypes:

```text
Hound    Threat 1    fast group pressure, leap/flank
Spitter  Threat 2    ranged pressure/projectile
Brute    Threat 5    armor, charge, heavy melee
Elite    Threat 6–8  base behavior plus one advanced mutation trait
```

If the old Hound prototype is still stored at a `Crawler`-named asset path/class shell, keep the path for serialization safety and expose Hound in current data/UI.

Director scaling canonical values:

```text
1P  Threat 1.00  Health 1.00  Special cap 1
2P  Threat 1.55  Health 1.05  Special cap 2
3P  Threat 2.05  Health 1.08  Special cap 3
4P  Threat 2.45  Health 1.10  Special cap 4
```

Player count changes apply at the next wave/encounter refresh. Do not bulk-rewrite existing enemies' health during combat.

---

## 14. Gameplay Tags baseline

Keep existing runtime routing tags if already serialized, and add/use current semantic families for new work:

```text
Role.Vanguard
Role.Ranger
Role.Engineer
Role.Warden

Damage.Kinetic
Damage.Fire
Damage.Electric
Damage.Corrosive

Mutation.Thermal
Mutation.Electric
Mutation.Keratin
Mutation.Neural
Mutation.Regeneration
Mutation.Sensory

Build.Weapon
Build.Mutation
Build.Mobility
Build.Defense
Build.Device
Build.Survival

Enemy.Hound
Enemy.Armored
Enemy.Ranged
Enemy.Elite

State.Protected
State.Downed
State.Reviving
State.HighMutationLoad
State.Reinserting

Mission.Active
Mission.Complete
Mission.Failed
Mission.Extracting

Resource.Scrap
Resource.Electronics
Resource.Chemical
Resource.Tissue

Net.LocalOwner
Net.Authority
```

Do not use free-form string comparisons for gameplay categories already represented by tags/enums.

---

## 15. C++ and replication style

Follow Unreal coding conventions and these project rules:

- Forward-declare where legal; minimize header includes.
- Use `TObjectPtr<>` for reflected UObject references where appropriate.
- Prefer soft references for content that need not load eagerly.
- Use Gameplay Tags for extensible semantic categories, enums for closed mutually-exclusive states.
- Use `ReplicatedUsing` when clients need a presentation reaction.
- Validate Server RPC preconditions even without anti-cheat.
- Prefer timers/events over low-frequency Tick.
- Justify any Tick-based system.
- Keep Blueprint-facing API narrow.
- Never store unrelated tuning values in Character defaults.

Replication preferences:

```text
CharacterMovement        movement
Replicated property      durable state
RepNotify                state-driven presentation
OwnerOnly                private run inventory/build
Server RPC               client-owned request
short cue/Multicast      presentation event only when appropriate
```

Avoid:

```text
per-frame Multicast
client-authoritative damage/resources/mission progress/Mutation Load
client-generated authoritative drops
Unreliable RPC as sole carrier of durable critical state
replicating data easily derived from an existing authoritative replicated value
```

---

## 16. Milestone discipline

M0–M4 is current completed programming scope and must stay coherent after any canon update.

Future work is only:

```text
M5  Mission graybox / full run objective flow
M6  Organ Roguelite / Field Purifier / 10 adaptations
M7  resources, settlement, Bio Lab, Armory, SaveGame
M8  formal level/art/VFX/SFX integration
M9  networking stability, loss/latency, performance, Shipping
M10 Vertical Slice acceptance
```

Do not create extra bridge/intermediate milestones for the background change.

Do not pull M5+ mission/reward/meta systems into M4 merely to rename old placeholders.

---

## 17. Testing contract

The framework must support:

```text
Solo          1 Host
2P            1 Host + 1 Client
4P            1 Host + 3 Clients
Latency       ~100 ms simulated ping
Packet Loss   2–5%
Client Leave  one Client leaves during mission
Host Leave    Host leaves; clients return to menu/error
Travel Loop   Lobby → Mission → Lobby x10
Role Lock     concurrent same-role requests
4 Roles       all four unique roles
Mutation      server cost, replication, high-load and overload behavior
Director      canonical 1P/2P/3P/4P scaling
```

After M4 code/data changes, at minimum re-run:

```text
C++ Build
Reclaim.* automation tests where available
M4 asset/config validation script
1P / 2P / 4P PIE
Role Lock
Ability activation + Mutation Load
Director scaling
Lobby → Mission → Lobby
100 ms latency smoke test
```

Formal art is not an M4 gate.
