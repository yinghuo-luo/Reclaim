# Reclaim C++ / TypeScript / Blueprint 架构

## 目标

本改造只借鉴 AtlasTs 的“职责分层”思想，不复制其 ARPG 业务逻辑。Reclaim 仍保持现有 M0–M4 网络权威、GAS、Weapon、AI、Animation 实现；PuerTS 被加入为策略/编排层。

数据流：

```text
Blueprint / DataAsset
  资产引用、动画/VFX、曲线、可视化配置
            │
            ▼
C++ authoritative core
  Replication / RPC / GAS / Damage / Weapon / AI executor / Mission state
            │ reflected UFUNCTION / UObject / delegates
            ▼
TypeScript policy layer
  规则组合、策略选择、流程编排、快速迭代
            │ command intent
            ▼
C++ / Blueprint validation + execution
```

## 三层边界

### C++：不可下放的稳定底座

保留在 C++：

- Replication、RPC、Server Authority 和反作弊/输入校验。
- CharacterMovement、Health/Protection、Downed/Revive。
- GAS Ability 激活与 GameplayEffect 实际执行。
- Weapon 弹药所有权、射速校验、Trace/Projectile、Damage。
- AI Perception、Navigation、EnemyActionExecutor、Director 的世界状态修改。
- PlayerState / GameState / Mission 的网络持久状态。
- SaveGame 与跨 Travel 状态。
- AnimInstance/AnimGraph 的每帧姿态计算。

### TypeScript：可快速迭代的 Policy / Orchestration

适合逐步迁入 TS：

- Mission/Encounter 的流程选择和阶段规则。
- Reward/Mutation 的候选构成与选择策略。
- AI 的“候选动作评分/组合”，但动作执行仍走 C++ executor。
- Weapon 的非权威策略组合，例如模式选择或表现参数请求；真正开火仍在 C++。
- Player 的上下文策略、可交互规则、客户端表现编排。
- UI ViewModel/界面流程编排（不保存权威游戏状态）。

不要让 TS 成为 replicated state owner，也不要在 TS 复制一份 C++/DataAsset 镜像数据。

### Blueprint / DataAsset：资产与表现配置

- 继续保存 SkeletalMesh、AnimSequence、Montage、Niagara、Sound、Curve、Widget、DataAsset 引用。
- `UReclaimScriptBindingComponent::ScriptConfig` 直接引用现有 DataAsset。
- TS 通过 PuerTS 反射读取这个 UObject，不生成额外 JSON 配置副本。
- Blueprint 用 `OnScriptCommand` 把 TS 的“意图”接入已有 C++ UFUNCTION；执行前由 C++/BP 做合法性校验。

## Runtime 设计

`UReclaimScriptRuntimeSubsystem` 是每个 `UGameInstance` 唯一的 VM owner：

```text
UGameInstance
└─ UReclaimScriptRuntimeSubsystem
   └─ puerts::FJsEnv("JavaScript")
      └─ Content/JavaScript/Reclaim/Bootstrap.js
```

Puerts Automatic Binding 关闭，避免 PuertsModule 自己再启动一个 VM。Reclaim 不使用 TS 继承 UE Character/Actor 作为主链，而使用反射桥接和 Delegate。

## Blueprint 接入范式

在需要脚本策略的 Blueprint 上添加 `Reclaim Script Binding Component`：

| 对象 | ScriptId | ExecutionPolicy | ScriptConfig |
|---|---|---|---|
| Player Character | `Player.Character` | `AuthorityOnly`（玩法）或客户端表现时 `NonAuthorityOnly` | 现有 Player/Role DataAsset |
| Enemy 基类 | `AI.Enemy` | `AuthorityOnly` | `EnemyDefinition`/相关 DataAsset |
| Weapon 基类 | `Weapon.Runtime` | `AuthorityOnly` | `ReclaimWeaponDefinition` |

现阶段这三个 TS Controller 是**空策略适配器**，不会改变原 Gameplay 行为。之后每迁一个规则，再把对应事件显式接入 TS；不要一次性重写 M0–M4。

## Event / Command 约定

C++/Blueprint -> TS：`SendScriptEvent(EventName, ContextObject, PayloadJson)`。

TS -> C++/Blueprint：`EmitScriptCommand(CommandName, ContextObject, PayloadJson)`。

建议命名：

```text
Lifecycle.BeginPlay
Lifecycle.EndPlay
Mission.PhaseChanged
AI.DecisionRequested
Weapon.ModeRequested
Player.ContextChanged
```

`PayloadJson` 只用于小型、瞬时、无 UObject 类型的 payload。大型配置和资产不要塞 JSON，直接把 UObject/DataAsset 放在 `ContextObject` 或 `ScriptConfig`。

## 网络规则

1. Gameplay Policy 默认 `AuthorityOnly`。
2. TS 只能提出 command intent；C++ 重新校验后执行。
3. Client TS 不直接修改生命、弹药、能力、任务阶段等权威状态。
4. TS 不自行维护“复制状态缓存”；需要状态时从 C++ reflected UObject 读取。
5. Dedicated Server/Listen Server 均只依赖同一套 C++ authority contract。

## 推荐迁移顺序

1. 先只启用 Runtime + Binding，不迁业务逻辑，验证 PIE / Listen Server / packaged build。
2. 迁 Mission/Reward 这种低频规则编排。
3. 迁 AI action scoring，但保留 C++ action executor。
4. 迁 Weapon/Player 上的非核心策略和表现编排。
5. 最后再评估是否需要更强的 TS 类型生成/热重载；不要把 TS Actor inheritance 变成项目默认模式。

## V8 94

本包不会下载或替换你已有的 PuerTS。应用脚本只检查：

- `Plugins/Puerts` 已存在。
- `Source/JsEnv/JsEnv.Build.cs` 包含 `V9_4_146_24`（即本项目约定的 V8 94 backend）。

如果你已经把对应 backend 放入 Puerts 的 `ThirdParty`，脚本不会触碰它。

## TypeScript 类型

基线桥接层故意只依赖 `puerts.argv`，因此即使尚未生成完整 UE `Typing/` 也可以编译。
等 C++ 首次成功编译后，可以使用 PuerTS 的类型声明生成功能生成 `Typing/`，再把具体 Policy 中的 `any` 逐步替换为 `UE.Reclaim...` 类型。不要为了类型生成而把 Runtime 改回 Automatic Binding。
