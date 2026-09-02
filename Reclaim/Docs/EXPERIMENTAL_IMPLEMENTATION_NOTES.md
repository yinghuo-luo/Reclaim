# 实验实现登记

本文件记录当前为了打通 M0/Foundation 功能链路而保留的实验性实现。这里的内容不是长期架构承诺，后续对应系统完成后应替换或删除。

判定规则：

- 如果实现只用于功能占位、编辑器批量生成或调试展示，必须登记在这里。
- 后续修改涉及下表系统时，先读本文件；如果功能已经正式实现，必须替换或删除对应临时逻辑，并同步更新本文件、`M0_FOUNDATION_USAGE.md` 和 `PROJECT_FRAMEWORK.md`。
- 如果实现影响联机权威、存档、复制范围或公开类边界，替换前必须先核对 `AGENTS.md` 和 `PROJECT_FRAMEWORK.md`。
- 临时实现不得把客户端提升为权威，也不得把 Blueprint 变成第二套规则层。

## 当前实验项

| 功能 | 位置 | 当前用途 | 替换方向 |
| --- | --- | --- | --- |
| UMG 定时刷新 | `Source/Reclaim/UI/ReclaimUIWidgets.h` 中各 Widget 的 `RefreshInterval`/`FTimerHandle`；`Source/Reclaim/UI/ReclaimUIWidgets.cpp` 中 `StartRefreshTimer`、`ClearRefreshTimer`、`RefreshLobby`、`RefreshRoleCard`、`RefreshPlayerSlot`、`RefreshHUD`、`RefreshTeammateRow`、`RefreshRewardSelection`、`RefreshDebugText` | 临时用 Timer 轮询 GameState、PlayerState、组件和 DebugSubsystem，让占位 UI 能显示当前状态 | 改为事件驱动：RepNotify、GameState/PlayerState 委托、ASC 属性变化回调、MVVM 或明确的 UI ViewModel。UI 只展示复制读模型，不轮询权威数据 |
| 占位 Widget Blueprint | `Content/Reclaim/UI/Menu/WBP_MainMenu.uasset`、`WBP_SessionBrowser.uasset`；`Content/Reclaim/UI/Lobby/WBP_Lobby.uasset`、`WBP_RoleCard.uasset`、`WBP_PlayerSlot.uasset`；`Content/Reclaim/UI/HUD/WBP_HUD.uasset`、`WBP_TeammateRow.uasset`、`WBP_RewardSelection.uasset`、`WBP_NetDebug.uasset` | 由脚本创建的最小可绑定控件，用于调用原生 UI Widget 类并验证菜单、Lobby、HUD、奖励和调试链路。当前脚本运行会先删除旧 WBP，再按最新 `BindWidget` 契约重建，避免保留旧控件树或旧父类 | 用项目正式 UMG 资产替换布局和表现。仍应绑定原生 API，不在 Widget Blueprint 内重写角色预约、资源、任务阶段、伤害或奖励权威规则 |
| Widget 资产生成辅助 | `Tools/Editor/create_m0_foundation_assets.py` 的 `WIDGET_BLUEPRINT_SPECS`、`RECREATE_GENERATED_WIDGET_BLUEPRINTS`、`clean_obsolete_generated_assets`、`create_foundation_widgets`、`ensure_widget_blueprint_controls`；`Source/Reclaim/Editor/ReclaimPythonAssetSetupLibrary.h/.cpp` | 临时辅助 Python 在编辑器内清理旧占位 WBP、创建同名 Button/TextBlock/ProgressBar，并标记变量以满足原生 `UPROPERTY(meta=(BindWidget))` | 正式 UI 资产稳定后删除或收窄为一次性迁移工具。不要把它扩展成运行时 UI 系统 |
| 占位地图 | `Content/Reclaim/World/Maps/L_MainMenu.umap`、`Content/Reclaim/World/Maps/L_Lobby.umap`、`Content/Reclaim/World/Forest/L_Forest_Mission.umap`；脚本入口在 `Tools/Editor/create_m0_foundation_assets.py` 的 `create_and_configure_maps` | 最小地图和 PlayerStart，用于验证 Main Menu -> Lobby -> Mission 的 Listen Server travel | 后续由编辑器制作正式关卡。任务推进仍归 MissionController/GameMode/GameState，不能迁移到 Level Blueprint |
| 占位角色、敌人和设备蓝图 | `Content/Reclaim/Characters/Player/BP_PlayerRobot_*.uasset`、`Content/Reclaim/Characters/Enemies/BP_Enemy_*.uasset`、`Content/Reclaim/Devices/BP_Deployable_*.uasset`、`Content/Reclaim/Core/GameModes/BP_GM_*.uasset` | 只提供原生类子类、默认类引用和基础编辑器可见入口 | 后续替换为项目自有视觉/动画/设备表现子类。Blueprint 子类保持薄层，不拥有联机权威规则 |
| 技能执行族桩 | `Source/Reclaim/AbilitySystem/ReclaimGameplayAbility.*`；`Source/Reclaim/AbilitySystem/Abilities/ReclaimGenericCombatAbility.*`、`ReclaimMovementAbility.*`、`ReclaimDeployAbility.*`、`ReclaimScanAbility.*`、`ReclaimChannelAbility.*`；配置在 `Source/Reclaim/AbilitySystem/ReclaimAbilityDefinition.*` 和 `Content/Reclaim/Data/Abilities/DA_Ability_*.uasset` | 当前主要建立 AbilityDefinition -> GameplayAbility 的类型和数据通道，具体 Montage、Notify、GameplayEffect、GameplayCue 和技能生命周期还未完整实现 | 按执行语义补完少量 Ability family。数值和资产继续放 DataAsset，不为每个同生命周期技能新增重复 C++ 类 |
| 任务流程占位 | `Source/Reclaim/Missions/ReclaimMissionController.*` 的 `AdvanceObjectiveOnce`；`Source/Reclaim/GameModes/ReclaimMissionGameMode.*`；`Source/Reclaim/GameModes/ReclaimMissionGameState.*` | 当前只提供 MissionPhase、RunSeed、队伍资源和目标幂等推进的基础壳 | 补全 Landing -> 三节点 -> Root Nest -> Extraction -> Settlement 的服务器权威流程。奖励、资源和随机性继续由服务器稳定种子驱动 |
| AI Director/CombatBrain 基础决策 | `Source/Reclaim/AI/ReclaimEnemyDirector.*`、`ReclaimEnemyCombatBrain.*`、`ReclaimEnemyActionExecutorComponent.*`；配置在 `Content/Reclaim/Data/AI/DA_AI_DemoConfig.uasset` | 当前只验证 Threat Budget 缩放和简单 Approach/Retreat/UseSkill 意图 | 补全服务器刷怪点过滤、波次组合、Elite cap、离场刷新策略和 ActionExecutor 侧 UE 副作用。CombatBrain 仍只产出意图 |
| Debug UI 和 DebugSubsystem | `Source/Reclaim/Debug/ReclaimDebugSubsystem.*`；`Content/Reclaim/UI/HUD/WBP_NetDebug.uasset`；`Source/Reclaim/UI/ReclaimUIWidgets.cpp` 的 `UReclaimNetDebugWidget` | Development 阶段显示角色预约、RunSeed、MissionPhase 等排错信息 | 保留为开发钩子或迁移到受控调试面板。Shipping 不显示，也不参与玩法权威 |

## 本次已清理的旧内容

| 内容 | 原位置 | 清理原因 |
| --- | --- | --- |
| 误生成的角色命名敌人蓝图 | `Content/Reclaim/Characters/Enemies/DA_Role_Engineer.uasset`、`DA_Role_Ranger.uasset`、`DA_Role_Vanguard.uasset`、`DA_Role_Warden.uasset` | 文件名像角色 DataAsset，但实际父类是 `ReclaimEnemyCharacter`。当前有效角色 DataAsset 位于 `Content/Reclaim/Data/Roles/` |
| 旧 Lobby redirector 地图 | `Content/Reclaim/World/L_Lobby.umap` | 该文件是指向 `Content/Reclaim/World/Maps/L_Lobby.umap` 的 redirector，当前配置和脚本均使用新路径 |
| 一次性 Unreal Python 探针 | `Tools/Editor/_inspect_unreal_python.py` | 只用于探索 Unreal Python API，无运行时或资产生成引用 |
| Python 字节码缓存 | `Tools/Editor/__pycache__/create_m0_foundation_assets.cpython-313.pyc` | 生成缓存，不属于工程源数据 |
| 模板项目名 redirect | `Config/DefaultEngine.ini` 的 `TP_Blank` `ActiveGameNameRedirects` | 内容资产未引用 `TP_Blank`，保留会让旧模板迁移逻辑继续污染配置 |
| 旧 Widget Blueprint 占位生成物 | `Content/Reclaim/UI/Menu/WBP_MainMenu.uasset`、`WBP_SessionBrowser.uasset`；`Content/Reclaim/UI/Lobby/WBP_Lobby.uasset`、`WBP_RoleCard.uasset`、`WBP_PlayerSlot.uasset`；`Content/Reclaim/UI/HUD/WBP_HUD.uasset`、`WBP_TeammateRow.uasset`、`WBP_RewardSelection.uasset`、`WBP_NetDebug.uasset` | 本次脚本改为运行前直接删除并按当前 C++ `BindWidget` 控件清单重建，避免旧父类、旧控件树和旧 Event Graph 逻辑残留 |
| 武器开火验证桩 | `Source/Reclaim/Components/ReclaimWeaponComponent.*`、`Source/Reclaim/Weapons/ReclaimWeaponBase.*`、`ReclaimHitscanWeapon.*`、`ReclaimProjectileWeapon.*` | 已升级为 M2 服务器权威 combat path：server validation、ammo/reload、deterministic hitscan spread、damage through AttributeSet、short presentation hooks；不再只是消耗弹药的临时桩 |

## 新增临时实现登记要求

新增临时代码或资产时，在本文件追加一行并写清：

- 功能名称。
- 代码和资产位置。
- 当前为什么需要它。
- 后续由哪个正式系统替换。
- 是否触碰服务器权威、复制、SaveGame 或 Blueprint 公共接口。

## 2026-08-29 M4 update

The previous AI Director/CombatBrain placeholder row is superseded for M4 native code. `Source/Reclaim/AI/ReclaimEnemyDirector.*`, `ReclaimEnemyCombatBrain.*`, `ReclaimEnemyActionExecutorComponent.*`, `ReclaimEnemySpawnPoint.*`, and `Source/Reclaim/Characters/ReclaimEnemyCharacter.*` now implement the server-only combat sandbox AI path, deterministic scaling/composition solver, spawn-point validation contract, public replicated enemy debug state, enemy targetable/pushable/purifiable contracts, and server-confirmed enemy death notification. Remaining temporary/content work is limited to project-owned visual enemy Blueprint presentation, final animations/VFX/SFX, and final map spawn placement. M4 gameplay DataAsset values are now maintained by `Tools/Editor/update_m4_gate_assets.py` instead of requiring the destructive full M0/UI regeneration path.

The previous generic ability-family placeholder row is superseded for the M3/M4 gameplay gate. `ReclaimMovementAbility`, `ReclaimScanAbility`, `ReclaimDeployAbility`, and `ReclaimGenericCombatAbility` execute the current server-side P0 contracts for Dash push, Tactical Scan marking, Kinetic Barrier replicated collision/absorption actor, Repair Drone replicated repair-tick actor, Overclock on owned deployables, Purification Pulse damage/stagger, and Stability Field replicated overlap/buff actor. Final mesh/animation/Montage/Niagara/material/audio presentation remains content work and must not become a second gameplay-authority path.


## 2026-08-29 M4 final-gate asset workflow

`Tools/Editor/update_m4_gate_assets.py` is the narrow gameplay-only migration entry point for the M4 gate. It reuses `create_m0_foundation_assets.py` canonical DataAsset/Blueprint specs but intentionally does not recreate Widget Blueprints or maps and does not assign missing art assets. `Tools/Editor/validate_m0_m4_assets.py` validates gameplay-critical Role/Ability/Enemy/Deployable mappings and reports missing art separately.

For M4, `UReclaimEnemyDefinition::BaseEnemyDefinition` is not treated as active runtime inheritance. The generated Crawler/Spitter/Brute/Elite definitions are explicit standalone gameplay configurations; Elite adds its current affix through `EliteAffix`. WeakPoint values remain prepared data until a real mesh/bone/hit-zone path exists.
