# PROJECT_FRAMEWORK.md — Project Reclaim / 净界计划 UE5.6 项目框架

> UE 5.6 · Windows PC · 1–4 人合作 PVE FPS/Roguelite · Listen Server
>
> 本文件定义当前工程的结构、权威边界、M0–M10 开发里程碑以及新背景下的数据/程序规范。M0–M4 已作为程序基线存在；本文件直接把当前背景纳入 M4，不设置额外过渡里程碑。

---

# 0. 当前项目基线

当前 Demo 目标：

```text
1–4 人合作
Listen Server
Host / Join / Lobby / Role Select / Ready / Start
Vanguard / Ranger / Engineer / Warden，职业不可重复
末世森林隔离区
适应者行动小队
写实实弹枪械为持续主输出
有限变异能力 + 战术设备
Mutation Load
功能器官样本 → 现场纯化三选一 → 个人 Build
撤离 → Bio Lab / Armory → 下一局
```

M0–M4 程序基线：

```text
M0  Session / Lobby / Role Lock / Travel / Authority
M1  Replicated Character movement / camera / HUD baseline
M2  authoritative rifle/shotgun combat / Health + Protection
M3  4 roles / 8 abilities / Downed / Revive / Reinsertion
M4  Enemy AI / Director / Threat scaling
```

当前没有正式美术资源。M0–M4 不要求补录正式角色、武器、敌人、动画、森林、VFX 或 SFX。

旧工程中已经存在并被 Blueprint/序列化引用的内部名称可以保留，例如：

```text
BP_PlayerRobot_*
UReclaimHealthShieldComponent
BP_Enemy_Crawler
DA_Enemy_Crawler
BP_KineticBarrier
旧 Reward/Modifier/Fabrication 类
```

这些名称不是第二套设计，只是兼容实现细节。玩家可见文本、DataAsset 的当前 ID/Tag、未来代码与文档使用新背景语义。

---

# 1. 固定技术决策

```text
Engine                 Unreal Engine 5.6
Platform               Windows PC
Primary language       C++ core + Blueprint content/presentation
Input                   Enhanced Input
Abilities/status        Gameplay Ability System + Gameplay Tags
Networking              Listen Server
Development sessions    OnlineSubsystem Null / LAN path
Authority               Server authoritative
Players                 1–4
Lobby role rule         4 roles; no duplicates
Travel                  Host-driven server travel; preserve PlayerState data
Mission join            Disabled
Persistence             Local SaveGame for personal meta progression
AI                      Server-only decisions
World generation        Hand-authored map; no runtime random terrain
Art                     no formal art required through M7 graybox/content phase
```

P0 不引入 Dedicated Server-only 假设。

---

# 2. 推荐工程目录

保持现有模块名 `Reclaim`，项目自有内容集中在 `Content/Reclaim`。

```text
Reclaim.uproject
AGENTS.md
PROJECT_FRAMEWORK.md
CURRENT_USAGE_M0_M4.md

Config/
├─ DefaultEngine.ini
├─ DefaultGame.ini
├─ DefaultInput.ini
└─ DefaultGameplayTags.ini

Source/Reclaim/
├─ Core/
├─ Network/
├─ GameModes/
├─ Player/
├─ Characters/
├─ Components/
├─ Mutation/
├─ AbilitySystem/
├─ Weapons/
├─ Devices/
├─ Roles/
├─ Missions/
├─ Rewards/
├─ Resources/
├─ AI/
├─ Animation/
├─ Save/
├─ UI/
└─ Debug/

Content/Reclaim/
├─ Core/Network/
├─ Core/GameModes/
├─ Characters/Player/
├─ Characters/Enemies/
├─ Weapons/
├─ Devices/
├─ Abilities/
├─ Mutation/
├─ Missions/
├─ Resources/
├─ Rewards/
├─ Save/
├─ Debug/
├─ World/Forest/
├─ UI/Menu/
├─ UI/Lobby/
├─ UI/HUD/
├─ UI/BioLab/
├─ VFX/
├─ Audio/
├─ Data/Roles/
├─ Data/Weapons/
├─ Data/Devices/
├─ Data/Abilities/
├─ Data/Enemies/
├─ Data/Adaptations/
├─ Data/Organs/
├─ Data/Missions/
└─ Data/AI/
```

Fab / Marketplace 原始资产放在独立目录，不直接修改其原始内容。

---

# 3. 核心对象职责

| 对象 | 权威 / 职责 |
|---|---|
| `UReclaimSessionSubsystem` | Create / Find / Join / Destroy Session；跨图会话入口信息 |
| `AReclaimLobbyGameMode` | 服务器角色占用、Ready、Start、Travel |
| `AReclaimLobbyGameState` | 复制玩家列表、角色占用表与 Lobby 读模型 |
| `AReclaimMissionGameMode` | 服务器出生、失败、Reinsertion、结算、Return Lobby |
| `AReclaimMissionGameState` | Mission Phase、Run Seed、团队资源、Threat、Extraction |
| `AReclaimPlayerState` | PlayerSlot、SelectedRole、Ready、Loadout、生命状态摘要、统计 |
| `AReclaimPlayerController` | 本地 UI、输入路由、客户端请求入口 |
| `AReclaimPlayerCharacter` | Replicated Character；武器、生命/防护、Mutation Load、互动 |
| `UReclaimMutationLoadComponent` | 服务器变异负荷、恢复、阈值、过载、能力许可 |
| `UReclaimRunInventoryComponent` | 个人 Build；Owner Only 为优先方向 |
| `AReclaimEnemyDirector` | 服务器 Threat Budget / spawn composition |
| `AReclaimEnemyAIController` | 服务器敌人 AI 外层循环 |
| `AReclaimMissionController` | M5 起服务器任务目标链 |
| Field Purifier / Reward service | M6 起服务器器官候选与个人选择 |

关键网络状态不放在 Level Blueprint。

---

# 4. 核心枚举 / 状态

保留既有稳定枚举时，不为世界观改名做破坏性迁移。

推荐闭合状态：

```text
EReclaimRole:
None / Vanguard / Ranger / Engineer / Warden

EReclaimSessionPhase:
MainMenu / Lobby / Travelling / Mission / Result / ReturningLobby

EReclaimPlayerLifeState:
Active / Downed / Destroyed / Redeploying
```

如果现有代码仍使用 `Destroyed` / `Redeploying`，玩家可见文本映射为“重度失能 / 应急重新投入”。

M5 Mission phase 应直接使用当前任务语义：

```text
None
Insertion
Activity1
Activity2
Activity3
BreedingDen
ExtractionUnlocked
Extracting
Succeeded
Failed
```

若现有序列化 enum 已有 `Landing/Node1/Node2/Node3/RootNest`，M5 可继续使用这些内部枚举值并在显示层映射，除非有安全迁移需求。

---

# 5. Lobby / Travel 不变量

角色池：

```text
Vanguard
Ranger
Engineer
Warden
```

规则：

1. 同一 Lobby 同一角色最多 1 人；
2. Client 只发送角色请求；
3. Server 原子验证和提交；
4. `Ready=true` 后禁止切换；
5. 切换时先验证目标空闲，再释放旧角色；
6. 离队立即释放；
7. Travelling / Mission 锁定；
8. SelectedRole 经 PlayerState 跨 Travel；
9. Mission 根据 RoleDefinition.PawnClass 生成角色；
10. 返回 Lobby 重置 Ready。

Session flow：

```text
Main Menu
→ Lobby
→ Travelling
→ Mission
→ Result
→ Lobby
```

Mission 不允许 Mid-Run Join。

---

# 6. 玩家基础状态

当前玩家基础数值：

```text
Health           100
Protection        75
Mutation Load    0–100
Emergency Reinsertion 1 / Run
```

内部已验证的 `Shield` attribute/component 名称可以继续承载 `Protection` 数值；新 HUD 和文案显示“防护”。

Mutation Load：

```text
0–69    Safe
70–99   High
100     Overload
```

过载时：

- 暂时禁止继续激活变异能力；
- Mutation Load 由服务器恢复；
- 不阻止普通枪械开火；
- 不触发永久感染、不可控变身或长期惩罚。

目标战斗构成为约：

```text
70% 写实枪械持续战斗
30% 职业变异能力 / 设备 / 适应 Build
```

---

# 7. M4 四职业能力语义

当前 M4 直接使用以下能力语义。

## 7.1 Vanguard / 先锋

### 肌纤维爆发 / Muscle Burst

- 方向高速位移；
- Server 验证状态、Mutation Load、碰撞和最终位置；
- 可以撞开 Hound 等可推动目标；
- 复用现有 movement ability 生命周期。

### 角质硬化 / Keratin Hardening

- 短时正面/近距离减伤与防护强化；
- Server 维护持续时间、吸收/减伤和 Mutation Load；
- 当前 M4 可复用已经验证的 Barrier Actor / collision 生命周期作为内部实现；
- 不要求能量护盾美术。

## 7.2 Ranger / 猎手

### 猎感扫描 / Hunter Sense Scan

- Server 范围查询；
- 标记高威胁目标/弱点/特殊样本方向；
- 团队可见 Mark；
- Current M4 只需保证共享标记与网络权威，弱点骨骼命中属于后续美术/HitZone 接入。

### 肌腱跃迁 / Tendon Leap

- Client 提交方向/目标意图；
- Server 验证距离、状态与可达性；
- 服务器执行最终移动；
- 增加 Mutation Load。

## 7.3 Engineer / 工程师

### 支援无人机 / Support Drone

- Server Spawn 复制 Actor；
- 修复 Protection、友方设备或任务设施；
- Demo 不要求复杂独立寻路；
- 增加 Mutation Load。

### 工程过载 / Engineering Overload

- Server 对已部署设备施加短时强化；
- 提高攻击/耐久/效率或恢复；
- 复制强化状态；
- 增加 Mutation Load。

## 7.4 Warden / 守望者

### 再生脉冲 / Regeneration Pulse

- Server 范围治疗自身和附近队友少量 Health；
- 对近距离变异生物造成短硬直；
- M4 可复用旧 Area Combat/Pulse 查询生命周期，但 Data 不再把它定义为净化电击主伤害能力；
- 增加 Mutation Load。

### 稳定剂场 / Stabilizer Field

- Server Spawn 场 Actor；
- Server 维护 overlap；
- 降低队友 Mutation Load 增长；
- 提高 Mutation Load / Protection 恢复效率；
- 增加施放者 Mutation Load。

---

# 8. M4 Mutation Load 程序接口

新增等价于：

```text
UReclaimMutationLoadComponent
```

职责：

```text
CurrentMutationLoad
safe/high/max thresholds
server-only spend/reduce/reset
server recovery timer
overload lock
replication / RepNotify
ability activation query
```

基础约束：

- Client 不直接写 CurrentMutationLoad；
- Ability 在服务器执行前检查并消耗；
- 达到 100 时进入过载；
- 恢复到安全阈值附近才解锁；
- 停止施法后延迟恢复；
- Warden Stabilizer Field 的效果通过已有 tag/overlap 状态作用于该组件；
- 不使用每帧 Multicast。

M4 允许使用执行家族的固定基础负荷成本；M6 `DA_Adaptation` 再加入器官能力自己的数据化 Cost。

---

# 9. M2 武器框架在新背景中的定义

M4 仍保留已验证的服务器权威武器实现。

P0：

```text
Assault Rifle       Hitscan
Tactical Shotgun    deterministic multi-Hitscan
Grenade Launcher    replicated projectile
```

不把所有枪改成 Projectile。

Hitscan：

1. 本地可立即播放枪口/后坐力表现；
2. Server RPC 接收紧凑射击请求；
3. Server 验证 owner / LifeState / Ammo / FireInterval / MissionState / aim；
4. Server trace；
5. Server damage；
6. Ammo / durable state replication。

Shotgun 散布必须由服务器可复现 Seed 决定。

Grenade Launcher：

- 服务器 Spawn Projectile；
- Projectile Replicate；
- 服务器碰撞 / 爆炸 / Damage；
- Client 只做表现。

旧 `ArcGun` DataAsset/类如果仍在工程中，不删除，但不属于当前 P0 默认 Loadout/内容范围。

---

# 10. 战术设备

P0：

```text
Auto Turret
Portable Protection Device
```

为了兼容旧资产，`DA_Device_ShieldGenerator` / `BP_Deployable_ShieldGenerator` 路径可以继续存在，但当前 Display/DeviceId 语义应是 Portable Protection / 便携防护装置。

设备由 Server Spawn；碰撞、耐久、生命周期、伤害/效果由 Server 决定。

正式炮塔/防护装置模型与 VFX 不属于 M4。

---

# 11. M4 敌人与 Director

## 11.1 敌人

| 当前语义 | Threat | 核心行为 | 器官方向 |
|---|---:|---|---|
| Hound | 1 | 高速群体、扑击、侧翼 | 肌腱 / 感官 |
| Spitter | 2 | 保持距离、远程分泌物/腐蚀投射 | 腐蚀腺体 |
| Brute | 5 | 骨甲、防御、冲撞、重击 | 角质 / 高密度骨组织 |
| Elite | 6–8 | 基础行为 + 单一高级异变 | 高品质器官 |

现有 `DA_Enemy_Crawler` / `BP_Enemy_Crawler` 路径可继续承载 Hound 数据，不进行破坏性资产重命名。

## 11.2 Director canonical scaling

```text
Effective players   Threat scalar   Health scalar   Special cap
1                   1.00            1.00            1
2                   1.55            1.05            2
3                   2.05            1.08            3
4                   2.45            1.10            4
```

规则：

- Director 仅服务器运行；
- Budget/composition 是主要难度轴；
- 不用大幅提高普通敌人生命值；
- 人数变化在下一次 Encounter Refresh / Spawn Wave 生效；
- Spawn 需要最小玩家距离、NavMesh、合理遮挡；
- 高强度时延迟下一波；
- Brute / Elite 同时数量受 cap 限制。

---

# 12. Gameplay Tags

新内容优先使用：

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

旧 Ability routing tags 若已被输入/执行链使用，不删除；DataAsset 可同时拥有旧 routing tag 与新 semantic tag。

---

# 13. 网络权威矩阵

| 系统 | 服务器最终决定 | 客户端 |
|---|---|---|
| 移动 | 最终位置/校正 | 输入、相机、预测表现 |
| 武器 | 合法射击、命中、伤害、Ammo | 输入、本地枪口/后坐力 |
| 职业能力 | Cooldown、Mutation Load、范围/命中、持续状态 | 输入、HUD、VFX/SFX |
| Health/Protection | 数值、Downed、Revive、Reinsertion | HUD、反馈 |
| Mutation Load | 当前值、增长/恢复、阈值、过载 | HUD/过载表现 |
| AI | 感知、目标、动作、伤害 | 复制位置/动画 |
| Director | Budget、组合、Spawn | Debug 显示 |
| Mission | 阶段、计数、完成/失败、Extraction | 目标 UI |
| 资源 | 生成、拾取、Team pool | HUD |
| 器官候选 | Seed、SampleType、候选、选择确认 | 个人选择 UI |
| Personal Build | 服务器确认 Run state | Owner Only 读模型 |
| Meta Save | server-confirmed settlement/research payload | 本地 SaveGame |

客户端不得直接修改服务器 Gameplay 真相。

---

# 14. RPC / Replication

Client 请求：

```text
Fire
Reload
ActivateAbility
Interact
SelectAdaptation
Ready
```

服务器执行前按系统验证：

```text
Owner
LifeState
Distance
MissionPhase
Cooldown
Ammo
Mutation Load
target validity
role/state legality
```

原则：

- 高频连续状态优先 Replicated Property / CharacterMovement；
- 不在 Tick 高频 Multicast 玩家/AI；
- Projectile 由 Server Spawn；
- Mission/Drop/Organ candidate 使用 Server RunSeed；
- Personal Build 优先 Owner Only；
- Team Resources / Mission State 放 GameState；
- 不依赖 `GetPlayerController(0)` 作为通用 Gameplay context；
- 不使用本地时间作为服务器 RNG 真相。

---

# 15. M4 DataAsset 状态

M4 只配置/验证当前已经存在并影响 M0–M4 Gameplay 的数据：

```text
DA_Role_*
DA_Ability_*     8 个职业能力
DA_Weapon_AssaultRifle
DA_Weapon_HeavyShotgun
DA_Weapon_GrenadeLauncher
DA_Device_AutoTurret
DA_Device_ShieldGenerator  # current semantic = Portable Protection
DA_Enemy_Crawler           # current semantic = Hound
DA_Enemy_Spitter
DA_Enemy_Brute
DA_Enemy_Elite
DA_AI_DemoConfig
```

以下旧内容即使已经存在，也不作为 M4 canonical 配置目标：

```text
Arc Gun
旧 10 个电子/动能 Modifier
旧 Fabrication Pool
旧 Infected Nest Mission data
旧 PlayerRobot animation DataAsset
```

不要删除它们；后续 M6/M7/M8 直接按新背景的数据系统实现/替代。

---

# 16. M4 完成判定

M4 在新背景下完成需要：

```text
UE5.6 C++ Build PASS
M4 asset configuration matches the current C++/Blueprint baseline
1P / 2P / 4P role spawn and gameplay path
Role Lock
Assault Rifle / Shotgun server damage
4 roles / 8 abilities server-authoritative
Mutation Load replication and spend/recovery/overload
Downed / Revive / Reinsertion
Hound / Spitter / Brute / Elite behavior prototypes
Director exact canonical scaling
100 ms latency smoke test
Lobby → Mission → Lobby regression
```

允许存在：

```text
Skeletal Mesh unassigned
Anim Class unassigned
Niagara/SFX/material missing
placeholder UI
graybox map
```

正式美术缺失不阻塞 M4。

---

# 17. M5 — Mission 灰盒

M5 直接实现新背景任务，不做旧“感染节点/根巢”语义的并行版本。

主流程：

```text
Insertion
→ Activity Point 1
→ Activity Point 2
→ Activity Point 3
→ Breeding Den
→ Extraction Unlocked
→ optional Apex Hunt
→ Extraction
→ Result
```

P0 实现：

- 3 个变异活动点/繁殖节点；
- 每个阶段的服务器 objective actor/controller；
- 主巢 / Breeding Den；
- 最终高强度 Elite encounter，不做正式 Boss；
- Apex Hunt 作为一次可选高风险精英目标；
- Extraction A/B 每局启用其一；
- Success / Failed；
- 15–20 分钟完整多人流程；
- Mission phase/counters 写入 GameState；
- 同一目标服务器只完成一次；
- Client Leave 后下一次 Director refresh 调整人数。

地图主体手工制作；不运行时随机生成地形。

---

# 18. M6 — Organ Roguelite

M6 实现：

```text
Organ Sample
Field Purifier
per-player deterministic three-choice
Owner-only personal adaptation inventory/build
10 core adaptations
Mutation Build
```

## 18.1 Organ source

推荐 `DA_OrganSource`：

```text
SampleType
EnemySource
CandidatePool
ResearchUnlockId
Quality
```

来源：

- 活动点；
- Elite；
- 高风险事件；
- Apex Hunt。

样本作为 Encounter Reward；每名玩家独立生成候选。

稳定候选 Seed：

```text
RunSeed
+ PlayerSlot
+ SampleType
+ RewardIndex
```

未选择玩家不阻塞任务推进。

## 18.2 10 个核心强化

```text
热腺附着        Thermal / weapon
热腺扩散        Thermal / mutation
导电组织        Electric
神经反馈        Electric / Neural
角质反击        Keratin / Defense
猎血反应        Neural / Mobility
再生回路        Regeneration / Survival
捕食本能        Sensory / weak-point
破甲弹改装      Weapon
负荷缓冲        Mutation / Survival
```

实现优先使用事件/Effect/Tag，不在 Character Tick 写 10 个 one-off 分支。

## 18.3 Data Assets

新增：

```text
DA_Adaptation
DA_OrganSource
Data/Adaptations/
Data/Organs/
```

`DA_Adaptation` 建议字段：

```text
ID
OrganSource
Trigger
Effect
Values
MutationLoadCost
StackRule
ConflictRule
RoleAffinity
ReplicationScope
Tags
```

---

# 19. M7 — 资源、结算与家园 UI

M7 直接使用新资源：

```text
Scrap
Electronics
Chemical
Tissue
High-quality Organ / Research Data
```

不再把 Alloy / Crystal / Biopolymer / Anomaly 作为玩家可见 canonical 经济。

## 19.1 Team Resources

Mission GameState：

```text
TeamResources
```

服务器拾取验证后更新，全队 HUD 看到同一临时数量。

## 19.2 Settlement

成功撤离：

- snapshot TeamResources；
- 计算有效参与者；
- 发出 server-confirmed settlement payload；
- 每个有效本地玩家把收益写入个人 SaveGame。

失败与离队规则按 Demo 文档执行。

## 19.3 Bio Lab

UI 页面：

- 显示个人资源/研究样本；
- 使用带回器官/研究数据解锁新的 Adaptation candidate pool；
- 不做 3D 家园。

## 19.4 Armory

UI 页面：

- 基础枪械；
- Grenade Launcher；
- Device；
- 已解锁 weapon/adaptation modifier；
- 下一局 initial config。

## 19.5 SaveGame

个人本地 SaveGame：

```text
SchemaVersion
PermanentResources
ResearchedOrgans
UnlockedAdaptations
NextRunLoadout
OptionalLocalStats
```

Host 可单独保存区域 Threat / Control 读模型用于展示。

---

# 20. M8 — 正式关卡与美术

此阶段第一次集中导入正式美术。

范围：

```text
realistic PBR forest
rocks/roads
abandoned ranger/modern facility
4 Adapter role visuals from one coherent equipment family
first-person arms
realistic Assault Rifle / Shotgun / Grenade Launcher
Hound / Spitter / Brute / Elite visuals
small amount of nest props
VFX / SFX
finalized HUD presentation
```

视觉原则：

- 正常森林与废弃设施占画面主体；
- 变异危险通过尸骸、巢材、抓痕、足迹、卵囊、分泌物、局部雾提示；
- 不制作整图生物污染覆盖；
- 普通动物轮廓保持可识别，再叠加角质、腺体、骨甲、肌肉、感官异常；
- Fab 同类别优先同作者/同系列；
- Fab 原始资产不直接修改。

弱点系统若要使用骨骼/部位命中，在此阶段接入正式 HitZone/Bone mapping；M4 的 prepared weak-point data 不是完整部位命中系统。

---

# 21. M9 — 网络稳定、性能与打磨

覆盖：

```text
100 ms Ping
2–5% Packet Loss
Client Leave
Host Leave
Lobby → Mission → Lobby x10
4P peak enemies
Replication correctness
performance
Shipping / Development build
```

要求：

- Shooting / Ability / Mutation Load / Revive / Field Purifier 最终状态可收敛；
- AI / Director / Turret 不在 Client 重复权威决策；
- no P0 soft-lock；
- 1080p / 60 FPS 作为默认开发目标；
- build 不依赖 editor-only test object。

---

# 22. M10 — Vertical Slice 验收

新玩家无需开发者干预完成连续 2 次 Run：

```text
Host / Join
4-role unique selection
Ready / Start
Mission
3 activity points
Organ Sample
Field Purifier three-choice
personal adaptation build
Breeding Den
optional Apex Hunt
Extraction
Result
Bio Lab / Armory
next-run changed pool/loadout
second Run
```

1P、2P、4P 都走同一套多人 Gameplay 路径。

---

# 23. 多人测试矩阵

| 场景 | 配置 | 必须验证 |
|---|---|---|
| Solo | 1 Host | 全流程同多人路径；1 次 Emergency Reinsertion；Mutation Load |
| 2P | Host + Client | shooting / abilities / Revive / Mutation / resource / organ choice |
| 4P | Host + 3 Clients | role uniqueness / Director / build isolation / extraction |
| Latency | 100 ms | shooting / movement ability / Revive / organ choice |
| Packet Loss | 2–5% | Mission / Mutation / resources 最终收敛 |
| Client Leave | Mission 中离开 | Pawn 清理；下一 wave 降 budget |
| Host Leave | Mission | Session 终止；Client 返回菜单/error |
| Travel Loop | x10 | 无重复 PlayerState/reward/session residue |
| Role Lock | 并发同角色 | 仅服务器先处理成功者占用 |
| Build Isolation | 4 人连续选 3 次 | candidate / choice / RunInventory 不串号 |

---

# 24. Debug 工具

保留/扩展：

```text
Net Debug HUD
Role Debug
RunSeed Debug
Threat Debug
Replication Debug
Force Downed
Mission Skip
Give Resource
Give Organ
Mutation Debug
```

Mutation Debug 至少能：

```text
set current load
force high load
force overload
show recovery state/source
```

---

# 25. 禁止项

- 禁止 Client 直接修改 Health / Protection / Mutation Load / Resource / Objective / permanent research；
- 禁止 Tick 高频 Multicast；
- 禁止把 Mission authority 集中在 Level Blueprint；
- 禁止通用 Gameplay 依赖 `GetPlayerController(0)`；
- 禁止服务器 RNG 只依赖本地时间；
- 禁止 Client 先生成权威 Drop 再让 Server 接受；
- 禁止为新背景重写已验证的 Session/Lobby/Weapon/AI 链路；
- 禁止为了没有美术而伪造 `.uasset/.umap`；
- 禁止把旧内部资源名当作玩家可见 canonical 内容继续扩展；
- 禁止新增中间桥接里程碑。

---

# 26. 资产配置策略（不使用 Python）

从当前基线开始，项目以后不新增、不恢复、不依赖任何 Python 文件或脚本作为资产装配入口。项目不启用 Python Editor Script Plugin，也不以外部自动化程序替代工程资产的正常保存与版本控制。

所有固定配置必须通过以下方式完成并随项目文件保存：

- Unreal Editor 中的 Blueprint Class Defaults；
- 地图的 World Settings；
- C++ 构造函数和默认属性；
- 已纳入版本控制的 `.uasset`、`.umap` 和 `Config/*.ini`。

## 26.1 MainMenu 手动配置

在 Unreal Editor 中按以下顺序装配 MainMenu：

1. 打开 `WBP_MainMenu`，确认父类为 `ReclaimMainMenuWidget`，并保留 `HostButton`、`FindButton`、`SettingsButton`、`QuitButton`、`StatusText` 这些控件变量名。
2. 在 `WBP_MainMenu` 的 Class Defaults 中设置 `LobbyMap=/Game/Reclaim/World/Maps/L_Lobby`、`HostMaxPlayers=4`、`bLANSession=true`，并将 `SessionBrowserWidgetClass` 指向 `/Game/Reclaim/UI/Menu/WBP_SessionBrowser`。
3. 打开 `BP_PC_MainMenu`，确认父类为 `ReclaimMainMenuPlayerController`，将 `MainMenuWidgetClass` 指向 `WBP_MainMenu`，并启用鼠标显示、点击事件和悬停事件。
4. 打开 `BP_GM_MainMenu`，将 `PlayerControllerClass` 指向 `BP_PC_MainMenu`，将 `DefaultPawnClass` 设为 `None`，并清除不需要的 HUD。
5. 打开 `L_MainMenu` 的 World Settings，将 `GameMode Override` 设为 `BP_GM_MainMenu`。

## 26.2 Lobby 与 Mission 手动配置

- `L_Lobby` 的 World Settings 使用 `BP_GM_Lobby`。
- `BP_GM_Lobby.MissionMap` 指向 `/Game/Reclaim/World/Forest/L_Forest_Mission`。
- `WBP_Lobby` 的父类为 `ReclaimLobbyWidget`，并保留职业、Ready、Start Mission 及状态文字控件的现有变量名。
- `L_Forest_Mission` 的 World Settings 使用 `BP_GM_Mission`。
- 如果 `SessionBrowserWidgetClass` 留空，C++ 只使用 `ReclaimUIWidgets.cpp` 中的默认 Session Browser 路径；优先直接在 `WBP_MainMenu` 中填写该属性。

## 26.3 Mutation Load 手动配置

在 `BP_PlayerRobot_Base` 的 Components 面板中保留一个 `ReclaimMutationLoadComponent`。Vanguard、Ranger、Engineer、Warden 等角色 Blueprint 继承该配置；如果子类存在重复的本地组件，手动在 Components/SCS 中删除重复项，避免同一 Pawn 拥有多个 Mutation Load 组件。

`Source/Reclaim/Editor/ReclaimPythonAssetSetupLibrary.*` 原本只为旧的 Python 装配入口提供编辑器辅助，现已移除。以后新增资产、调整父类、默认引用、Widget 控件和地图 GameMode 时，直接在 Unreal Editor 或 C++ 中完成，并提交修改后的工程文件。
