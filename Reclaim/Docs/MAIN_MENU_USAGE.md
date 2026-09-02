# MAIN_MENU_USAGE.md
# Project Reclaim — Main Menu 使用、装配边界与验证手册

> 文件位置：`Docs/MAIN_MENU_USAGE.md`  
> 引擎：Unreal Engine 5.6  
> 网络：Listen Server / Server Authoritative  
> 适用阶段：M0–M4 验证以及后续所有 Main Menu / Session 前端回归  
> 更新日期：2026-09-01

---

# 0. 本文档解决什么问题

本文档专门处理 `L_MainMenu`。

当前最需要避免的错误是：

```text
打开 L_MainMenu
→ World Settings 看起来没有任何配置
→ PIE 后 Server 0 / Client 1 都是黑屏
→ 误以为 M4 Gameplay 或多人代码坏了
```

这类现象首先属于：

```text
Main Menu Frontend / Map / UI 装配问题
```

而不是：

```text
AI
Weapon
Ability
Mission
Replication
```

的问题。

尤其要明确：

> `World Settings -> GameMode Override = None` 不代表这个地图“完全没有 GameMode”。

它真正表示：

```text
这张地图没有设置“地图级 GameMode Override”
→ Unreal 会继续使用 Project Settings 中的默认 GameMode
```

所以排查时必须同时看：

```text
L_MainMenu 的 World Settings
+
Project Settings -> Maps & Modes
```

如果继承到的默认 GameMode 没有 Main Menu 前端逻辑，而且地图本身又是空地图，那么最终看到黑色世界是正常结果。

---

# 1. 当前代码基线与 Main Menu 的关系

当前 M0–M4 Gameplay 装配脚本的职责主要是：

```text
DataAsset
Gameplay Blueprint thin shell
Role / Weapon / Ability / Enemy / AI 配置
Gameplay defaults
Gameplay-critical validation
```

当前原有 M0–M4 脚本明确不负责：

```text
重建 Widget Blueprint
创建/修改地图
作者化 Blueprint Event Graph
最终 UI
最终美术
```

因此：

```text
setup_m0_m4_assets_fix5.py PASS
```

并不能证明：

```text
L_MainMenu 已经配置
WBP_MainMenu 已经显示
Host 按钮已经接到 SessionSubsystem
Find / Join 已经可用
```

Main Menu 必须作为单独 Frontend Gate 验证。

---

# 2. 当前项目的 Main Menu P0 目标

Main Menu 最低 P0 功能：

```text
Host
Find / Join
Settings
Quit
Session Error / Status
```

建议内容路径：

```text
/Game/Reclaim/World/Maps/L_MainMenu
/Game/Reclaim/UI/Menu/WBP_MainMenu
/Game/Reclaim/UI/Menu/WBP_SessionBrowser
```

Main Menu 不是 Gameplay Map。

它不应该负责：

```text
角色 Spawn
Weapon
AI
Mission Director
Role Authority
Damage
Revive
```

它的职责只有：

```text
本地前端
Session 入口
进入 Lobby
退出游戏
```

---

# 3. Main Menu 推荐职责结构

推荐结构：

```text
L_MainMenu
    ↓
MainMenu / Frontend GameMode
    ↓
本地 PlayerController
    ↓
Create WBP_MainMenu
    ↓
Host / Find / Join / Settings / Quit
    ↓
UReclaimSessionSubsystem
```

网络与 UI 职责：

```text
Widget
→ 只显示状态并发送请求

PlayerController
→ 本地 UI / Menu 请求入口

UReclaimSessionSubsystem
→ Create / Find / Join / Destroy Session
→ OnlineSubsystem delegate 生命周期
```

不要让 Widget 自己实现一套 OnlineSubsystem 会话系统。

不要让 Level Blueprint 成为长期 Session Owner。

---

# 4. 关于 Main Menu GameMode

Main Menu 最好使用独立的 Frontend GameMode，而不是：

```text
BP_GM_Lobby
BP_GM_Mission
```

推荐项目内容名：

```text
BP_GM_MainMenu
```

它应该非常薄。

推荐职责：

```text
Player Controller Class
→ Main Menu 专用/兼容的 PlayerController

Default Pawn Class
→ None 或不生成 Gameplay Pawn

Gameplay Authority
→ 无
```

Main Menu 不需要生成玩家战斗角色。

如果是纯 UMG 全屏菜单：

```text
没有 Gameplay Pawn
没有 3D Camera
```

本身不是错误。

只要 `WBP_MainMenu` 正常 Add To Viewport，UI 就可以完全覆盖黑色世界背景。

因此：

```text
黑色世界背景
≠ Main Menu 失败

完全没有 UI
= Main Menu 失败
```

---

# 5. `GameMode Override = None` 应该如何理解

这是当前最容易误判的地方。

当 `L_MainMenu` 中看到：

```text
World Settings
-> GameMode Override = None
```

真实含义是：

```text
没有地图级 Override
→ 使用 Project Settings 的 Default GameMode
```

不是：

```text
UE 没有 GameMode
```

因此必须继续检查：

```text
Edit
-> Project Settings
-> Maps & Modes
-> Default GameMode
```

可能出现三种情况。

## 情况 A：Default GameMode 正好是 Main Menu Frontend GameMode

那么：

```text
GameMode Override = None
```

也可能正常运行。

但是这种方式容易让 Lobby / Mission 继承错误，因此当前项目更推荐：

```text
每张核心地图显式指定自己的 GameMode
```

## 情况 B：Default GameMode 是普通 GameModeBase

那么 Main Menu 不会自动创建 Widget。

结果通常是：

```text
空地图
+
没有 Pawn / Camera 或只有默认观察
+
没有 Widget
=
黑屏
```

## 情况 C：Default GameMode 是 Lobby / Mission GameMode

这是错误配置。

可能导致：

```text
Main Menu Spawn Gameplay Pawn
调用 Lobby/Mission 生命周期
错误创建 HUD
错误使用 PlayerState
Travel/Session 行为混乱
```

Main Menu 不应通过全局 Gameplay GameMode“碰巧运行”。

---

# 6. 当前永久装配原则

项目当前固定原则：

> 可以重复、固定、机械化的 UE 装配一律使用 Python。

因此下面这些如果属于项目固定结构：

```text
创建/确认 BP_GM_MainMenu
创建/确认 MainMenu PlayerController Blueprint
设置 L_MainMenu GameMode
设置固定 Widget Class 引用
配置固定默认属性
设置 Maps & Modes
固定 Blueprint component / class reference
```

都应该由：

```text
Tools/Editor/*.py
```

完成。

本文档只说明：

```text
应该是什么
如何验证
如何诊断
```

不把长期工程方案建立在大量手工点击上。

如果当前基线缺少 Main Menu 专用装配 Python：

```text
这叫“Main Menu 装配脚本缺口”
```

不能把几十个 Editor 点击步骤当成新的永久基线。

---

# 7. 允许的手工操作范围

手工 Editor 操作主要用于：

```text
确认最终配置值
查看 World Settings
查看 Class Defaults
运行 PIE
观察 Output Log
检查 Widget 是否出现
测试按钮
调整最终 UI 美术布局
```

临时排错时，可以手工改变某个值验证原因。

例如：

```text
临时将 L_MainMenu GameMode Override 指到已知 Frontend GameMode
```

如果这样立刻恢复 UI，说明问题就是地图/GameMode 装配。

但是：

```text
诊断成功
≠ 可以把手工修改当作正式工程补丁
```

正式修复仍应回到 Python 装配。

---

# 8. Main Menu PlayerController 的职责

当前架构要求：

```text
PlayerController
→ Own local UI
→ Input routing
→ Client request entry point
→ Session/menu control
```

推荐 Main Menu PlayerController 在本地 BeginPlay 时完成：

```text
Is Local Controller
    ↓
Create WBP_MainMenu
    ↓
Add To Viewport
    ↓
Show Mouse Cursor = true
    ↓
Set Input Mode UI Only
```

重要原则：

```text
只为 Local Controller 创建 UI
```

不要在服务器上给每个远端 Controller 创建本地 Widget。

不要使用：

```text
GetPlayerController(0)
```

作为所有多人上下文的通用解决方案。

Main Menu 本地 UI 应使用：

```text
Owning Player / Self
```

对应的本地 Controller。

---

# 9. WBP_MainMenu 最低结构

M4 验证阶段不要求最终 UI 美术。

最低只需要：

```text
WBP_MainMenu

Host
Find / Join
Settings
Quit
Status / Error Text
```

最好额外显示：

```text
当前 Session 状态
Find 状态
Join 状态
错误原因
```

这样异步网络错误不会表现成：

```text
按钮点了没反应
```

---

# 10. Main Menu Widget 的创建链必须真实存在

只有：

```text
/Game/Reclaim/UI/Menu/WBP_MainMenu
```

这个资产存在，完全不够。

必须存在运行时创建链：

```text
Local PlayerController BeginPlay
→ CreateWidget(WBP_MainMenu)
→ Owning Player = 当前本地 PlayerController
→ AddToViewport
```

并设置：

```text
Show Mouse Cursor = true
Input Mode = UI Only
```

或者根据 UI 设计使用：

```text
Game and UI
```

但 Main Menu 最常用的是：

```text
UI Only
```

如果 Widget 有焦点需求，还应指定：

```text
Widget To Focus = MainMenu Widget
```

---

# 11. Main Menu 黑屏的标准排查顺序

出现：

```text
L_MainMenu 运行后黑屏
```

严格按这个顺序排查。

## 11.1 确认正在运行的地图

必须确认：

```text
/Game/Reclaim/World/Maps/L_MainMenu
```

不是：

```text
未保存的新地图
Lobby
Mission
默认空地图
```

---

## 11.2 看 `GameMode Override`

打开：

```text
World Settings
-> Game Mode
-> GameMode Override
```

如果：

```text
None
```

继续看 Project Settings。

不要在这里直接下结论说“没有 GameMode”。

---

## 11.3 看全局 Default GameMode

打开：

```text
Edit
-> Project Settings
-> Maps & Modes
```

确认：

```text
Default GameMode
```

到底是什么。

如果不是 Frontend GameMode，那么 `L_MainMenu Override=None` 很可能就是黑屏原因之一。

---

## 11.4 确认有效 PlayerController Class

需要确认实际运行的 Main Menu GameMode 的：

```text
Player Controller Class
```

是负责 Main Menu UI 的 Controller。

如果继承到默认：

```text
APlayerController
```

又没有 Level Blueprint 创建 UI，就不会出现 Main Menu。

---

## 11.5 确认 BeginPlay 是否运行

可以临时使用：

```text
Print String
```

或 Output Log 验证。

如果 Main Menu Controller BeginPlay 根本没有执行：

```text
先查 GameMode / PlayerController 选择
```

不要继续查 Widget。

---

## 11.6 确认 `Is Local Controller`

Main Menu UI 只应在：

```text
Is Local Controller = true
```

的 Controller 上创建。

如果 Branch false：

```text
当前 Controller 不是该窗口本地 Controller
```

---

## 11.7 确认 Widget Class

检查：

```text
WBP_MainMenu Class != None
```

如果是软引用，确认资产路径有效并成功加载。

---

## 11.8 确认 `CreateWidget`

`CreateWidget` 必须：

```text
Class = WBP_MainMenu
Owning Player = 当前本地 PlayerController
```

返回值不能是：

```text
None
```

---

## 11.9 确认 `Add To Viewport`

Widget 创建成功不等于显示成功。

必须实际执行：

```text
Add To Viewport
```

---

## 11.10 确认 Widget Visibility

检查：

```text
Visible
Self Hit Test Invisible
Hit Test Invisible
```

等 Visibility 是否符合设计。

根 Widget 如果是：

```text
Collapsed
Hidden
```

整个菜单都不会显示。

---

## 11.11 确认鼠标与 Input Mode

至少：

```text
Show Mouse Cursor = true
Set Input Mode UI Only
```

如果 UI 可见但点不了：

优先检查：

```text
Input Mode
Focus
Visibility
Button Is Enabled
Mouse Cursor
```

而不是网络代码。

---

# 12. 你之前看到的“Server 0 / Client 1 两个黑窗口”代表什么

当 PIE 配置：

```text
Number of Players = 2
Net Mode = Play As Listen Server
```

UE 会直接创建：

```text
Server 0
Client 1
```

并建立 PIE 网络关系。

如果 `L_MainMenu` 没有 UI 装配：

```text
Server 0 黑
Client 1 也黑
```

完全符合预期。

这不能证明：

```text
Session Host/Join 已经失败
```

因为这时根本还没有通过 Main Menu 用户流程测试 Session。

同样，它也不能证明：

```text
Session Host/Join 已经成功
```

因为 Client 1 是 Editor 自动创建和连接的。

所以：

```text
2P Play As Listen Server
```

不是 Main Menu Session Browser 的验收方式。

---

# 13. Main Menu UI 的第一阶段测试方式

首先只验证：

```text
L_MainMenu 能不能正常显示
```

使用：

```text
Number of Players = 1
Net Mode = Standalone
```

验收：

- Main Menu 出现；
- 鼠标出现；
- 鼠标能移动；
- Button hover 正常；
- Button click 正常；
- Status/Error Text 可更新；
- 不生成 Gameplay Pawn 也没关系；
- 背景是黑色/灰色临时世界也没关系。

只要全屏 UMG 正常显示，Main Menu 前端就算通过第一层。

---

# 14. Host 的正确职责

Host 按钮不能永久写成：

```text
OpenLevel(L_Lobby)
```

因为这样绕过了：

```text
CreateSession
Session Settings
OnlineSubsystem
Joinability
Session lifecycle
```

正确语义：

```text
Host Button
    ↓
本地 Menu / PlayerController request
    ↓
UReclaimSessionSubsystem
    ↓
Create Host Session
    ↓
CreateSession 成功回调
    ↓
Host 进入 L_Lobby
```

这是：

```text
Session
→ Lobby
```

而不是：

```text
Widget
→ 直接 OpenLevel
```

---

# 15. `UReclaimSessionSubsystem`

项目框架约定：

```text
UReclaimSessionSubsystem : UGameInstanceSubsystem
```

职责：

```text
Create Session
Find Sessions
Join Session
Destroy / Leave Session
管理 OnlineSubsystem async delegate
缓存搜索结果
```

当前架构中语义上应具备：

```text
Host Session
Find Sessions
Join selected Session
Leave Session
Destroy Session
Has Active Session
```

具体函数名必须以当前代码为准。

不要仅因为框架文档曾经写过：

```text
HostSession
FindSessions
JoinSessionByIndex
```

就假设 Blueprint 中一定存在同名节点。

如果当前 C++ 没有：

```text
BlueprintCallable
BlueprintAssignable
或 PlayerController wrapper
```

那么 Widget 中就看不到这些调用入口。

这时正确做法是：

```text
补当前代码/API 暴露
```

而不是：

```text
Widget 内重新写 OnlineSubsystem
```

---

# 16. Host 成功后应该发生什么

概念流程：

```text
CreateSession request
→ Async CreateSession result
→ Success
→ Host travel to Lobby as Listen Server
```

最终进入：

```text
/Game/Reclaim/World/Maps/L_Lobby
```

并由：

```text
BP_GM_Lobby
```

接管 Lobby 权威逻辑。

如果点击 Host 后：

```text
直接加载 L_Lobby
```

但 Session Browser 中其他客户端完全找不到它，要怀疑：

```text
只是 OpenLevel 成功
Session 根本没创建
```

---

# 17. Find / Join 的正确职责

Find / Join 概念：

```text
Find / Join Button
    ↓
SessionSubsystem.Find
    ↓
异步结果
    ↓
WBP_SessionBrowser
    ↓
显示搜索结果
    ↓
用户选择某条结果
    ↓
Join selected result
    ↓
Resolve connect string
    ↓
ClientTravel
    ↓
进入 Host Lobby
```

不要：

```text
Find 按钮直接打开 L_Lobby
```

也不要：

```text
Join 按钮用固定 IP / OpenLevel
```

来冒充完整 Session 流程。

---

# 18. WBP_SessionBrowser 的最低功能

建议最低显示：

```text
Host / Session 名称
当前玩家数
最大玩家数
Ping
Session Phase
Join 按钮
Refresh
Back
Error Text
```

Mission 阶段的会话：

```text
不应该作为可加入 Lobby 的 Session 展示
```

项目当前设计：

```text
Mid-Run Join = 不做
```

---

# 19. Session Error 一定要显示

Main Menu 必须有可见错误反馈。

至少覆盖：

```text
Create Session Failed
Find Session Failed
No Session Found
Join Session Failed
Session Full
Invalid Result
Connection Failed
Session Already Active
Host Session Lost
```

异步网络系统如果没有错误 UI，非常容易表现成：

```text
按钮没有反应
```

这会让排错难度大幅增加。

---

# 20. Settings

M4 验证阶段不需要完整 Settings 系统。

允许：

```text
Settings 按钮
→ 打开简单占位 Settings Widget
```

或者：

```text
显示“Settings pending”
```

但不建议保留完全没有任何反馈的死按钮。

---

# 21. Quit

Standalone / Game 运行下：

```text
Quit
→ Quit Game
```

PIE 中 Editor 对 Quit 的表现可能与最终打包游戏不同。

因此 Quit 的最终行为要在：

```text
Standalone / Development build
```

再确认一次。

---

# 22. 真正的 Host / Join 测试

要验证 Session，不要使用：

```text
2P Play As Listen Server
```

作为最终证据。

使用两个独立实例。

实例 A：

```text
启动 L_MainMenu
→ Host
→ CreateSession
→ L_Lobby
```

实例 B：

```text
启动 L_MainMenu
→ Find
→ Session Browser 看见 A
→ Join
→ 进入 A 的 L_Lobby
```

通过标准：

- B 是通过 Session Search 找到 A；
- B 是通过 Join Session 进入；
- 不是 Editor 预连接；
- 两边进入同一个 Lobby；
- Lobby 玩家数正确；
- Session metadata 正确；
- Mission 阶段不可加入。

---

# 23. Main Menu 与 Lobby 的边界

Main Menu 只管：

```text
Session
Frontend navigation
```

进入 Lobby 后：

```text
Role
Ready
PlayerSlot
Start Mission
```

全部由 Lobby 系统负责。

不要让 `WBP_MainMenu` 修改：

```text
SelectedRole
Ready
RoleReservations
MissionPhase
```

也不要让 Main Menu GameMode 保存这些 Gameplay 状态。

---

# 24. Main Menu 与 PlayerState 的边界

Main Menu 通常不需要为了 UI 强行使用完整 Gameplay PlayerState。

除非当前 `ReclaimPlayerController` 明确依赖某个自定义 PlayerState，否则 Frontend 应保持最小依赖。

Session 状态应该属于：

```text
GameInstanceSubsystem
```

而不是：

```text
PlayerState
```

因为 Session 生命周期跨地图。

---

# 25. Main Menu 与 Level Blueprint 的边界

临时排错可以：

```text
Level Blueprint
→ Create WBP_MainMenu
→ AddToViewport
```

如果这样一做菜单马上出现，可以证明：

```text
Widget 本身没坏
问题在 GameMode / PlayerController / Frontend 创建链
```

但是正式架构不应把 Main Menu 长期建立在 Level Blueprint。

正式应回到：

```text
PlayerController
或专用 Frontend 管理层
```

原因：

```text
UI 是本地玩家职责
Level Blueprint 不是 Session Owner
不利于复用与 Travel
```

---

# 26. Main Menu 与 Camera 的关系

纯 UMG Main Menu：

```text
不要求 Gameplay Camera
```

如果 Widget 覆盖整个窗口：

```text
世界背景是黑的也没关系
```

所以：

```text
没有 Camera
```

并不是“菜单 UI 不显示”的根因。

如果以后做 3D Main Menu 场景：

```text
CameraActor
背景环境
角色展示
```

都属于表现层。

不要让它们成为 Host / Join 的前置依赖。

---

# 27. Main Menu 与 Default Pawn 的关系

纯 Frontend 推荐：

```text
Default Pawn Class = None
```

或使用完全无 Gameplay Authority 的 Frontend Pawn / Spectator。

不要为了让屏幕“不黑”而生成：

```text
BP_PlayerRobot_Vanguard
BP_PlayerRobot_Ranger
...
```

作为 Main Menu Pawn。

玩家战斗 Pawn 应在 Mission 按：

```text
PlayerState.SelectedRole
```

服务器权威生成。

---

# 28. Main Menu 黑屏时不要做的错误修复

不要：

```text
把 BP_GM_Mission 设成 L_MainMenu GameMode
```

不要：

```text
把 BP_GM_Lobby 设成 L_MainMenu GameMode
```

不要：

```text
为了看到东西 Spawn 一个战斗 Pawn
```

不要：

```text
Host 按钮直接 OpenLevel(L_Lobby)
```

不要：

```text
Widget 自己重写 OnlineSubsystem
```

不要：

```text
看到 GameMode Override=None 就断言 UE 没有 GameMode
```

不要：

```text
用 2P Multi-PIE 证明 Find/Join 正常
```

不要：

```text
把 Main Menu 黑屏当作允许忽略的美术问题
```

---

# 29. 常见故障：UI 完全不显示

现象：

```text
窗口黑
没有按钮
没有鼠标
```

排查优先级：

```text
1. Map
2. Effective GameMode
3. PlayerController Class
4. PlayerController BeginPlay
5. Local Controller
6. Widget Class
7. CreateWidget
8. AddToViewport
9. Visibility
10. Input Mode
```

---

# 30. 常见故障：菜单显示但鼠标不见

检查：

```text
Show Mouse Cursor = true
Input Mode UI Only
Viewport Focus
```

如果鼠标被锁进 Gameplay：

```text
检查是否仍在 Game Only
```

---

# 31. 常见故障：鼠标出现但按钮点不了

检查：

```text
Widget Visibility
Button IsEnabled
Root panel hit-test
Input Mode
Widget Focus
是否有全屏透明 Widget 覆盖
```

---

# 32. 常见故障：Host 点了没反应

依次确认：

```text
OnClicked
→ Menu/PlayerController function
→ SessionSubsystem 获取成功
→ CreateSession request 发出
→ Async delegate 回调
→ Success / Failure 都有 UI
```

如果只有：

```text
按钮 → OpenLevel
```

这不是完整 Host。

---

# 33. 常见故障：Host 能进 Lobby，但 Client 找不到

这种现象首先怀疑：

```text
Host 只是 OpenLevel
没有真正 CreateSession
```

继续检查：

```text
OnlineSubsystem
LAN flag
Session Settings
Session metadata
CreateSession success callback
Host 是否仍处于 Lobby joinable phase
```

---

# 34. 常见故障：Find 永远 0 个结果

确认：

```text
两个独立实例
相同 OnlineSubsystem
相同 LAN / Non-LAN 选择
Host 已 CreateSession
Host Session 没有立即销毁
Session Phase = Lobby
搜索结果 MaxResults > 0
```

开发阶段使用 Null / LAN 时，还要确认两个实例处于可发现的本地开发环境。

---

# 35. 常见故障：Join 后没进 Host Lobby

检查：

```text
JoinSession result
Resolved Connect String
ClientTravel
Session result index
Host 是否已经 Travel
Host 是否进入 Mission
```

Mission 当前禁止 Mid-Run Join。

---

# 36. 常见故障：从 Main Menu 进 Mission 后不能移动

如果某条调试流程暂时直接经过 UI → Gameplay，检查：

```text
Input Mode 是否仍然是 UI Only
Show Mouse Cursor 是否按 Gameplay 需求关闭
Enhanced Input Mapping Context 是否恢复
Controller 是否 Possess Pawn
```

Main Menu 的：

```text
UI Only
```

不能永久残留到 Mission。

---

# 37. PIE 测试矩阵

| 测试目标 | 配置 | 是否能证明 Session 正常 |
|---|---|---|
| Main Menu UI 是否显示 | 1P Standalone | 否，只证明 Frontend |
| Host 按钮能发起 CreateSession | 1P Standalone / 独立实例 | 部分 |
| Find / Join | 两个独立实例 | 是 |
| Lobby replication | 2P Play As Listen Server | 否，只证明 Gameplay replication |
| Mission replication | 2P / 4P Play As Listen Server | 否，只证明 Gameplay replication |
| Session Browser | 两个独立实例 | 是 |

---

# 38. Main Menu 最低验证流程

严格按以下顺序。

## Gate A：纯 UI

```text
L_MainMenu
1 Player
Standalone
```

通过：

- [ ] Menu 可见。
- [ ] 鼠标可见。
- [ ] Host 可点击。
- [ ] Find / Join 可点击。
- [ ] Settings 有反馈。
- [ ] Quit 有反馈。
- [ ] Error/Status Text 可见。

## Gate B：Host

```text
独立实例 A
```

通过：

- [ ] 点击 Host。
- [ ] 发起真实 CreateSession。
- [ ] 成功回调。
- [ ] 进入 `L_Lobby`。
- [ ] 当前 Session 仍为 Active。
- [ ] Lobby 为 Joinable。

## Gate C：Find / Join

```text
独立实例 B
```

通过：

- [ ] 点击 Find。
- [ ] Session Browser 打开。
- [ ] 能看到实例 A。
- [ ] 玩家数正确。
- [ ] Ping/状态合理。
- [ ] 点击 Join。
- [ ] JoinSession Success。
- [ ] ClientTravel 到 Host Lobby。
- [ ] A/B 在同一 Lobby。

## Gate D：进入 Mission

在 Lobby：

```text
Role
→ Ready
→ Start
```

通过：

- [ ] Host ServerTravel。
- [ ] 所有 Client 到 Mission。
- [ ] Mission 不再出现在 Joinable Session 列表。
- [ ] Main Menu Widget 没有残留在 Mission Viewport。
- [ ] Gameplay Input 正常。

---

# 39. Main Menu 验收失败如何归类

## 可以归类为“表现未完成”

例如：

```text
背景还是黑色
按钮很丑
没有最终字体
没有正式音效
没有动画
没有背景场景
```

前提是：

```text
Menu 功能可用
```

## 不能归类为“表现未完成”

例如：

```text
完全没有 UI
Host 没有 CreateSession
Find 无实现
Join 无实现
按钮点了没回调
地图 GameMode 错
PlayerController 根本没创建 UI
```

这些属于：

```text
Frontend / Session 基础功能未完成
```

---

# 40. 当前 Main Menu Gate 的 PASS 定义

只有以下全部满足，才能标记：

```text
MAIN MENU GATE: PASSED
```

- [ ] `L_MainMenu` 能作为游戏前端启动。
- [ ] Effective GameMode 明确且正确。
- [ ] Main Menu 使用正确 PlayerController。
- [ ] `WBP_MainMenu` 由本地前端创建。
- [ ] `WBP_MainMenu` Add To Viewport。
- [ ] Mouse Cursor 正常。
- [ ] Input Mode 正常。
- [ ] Host 不绕过 Session。
- [ ] Host 能真实 CreateSession。
- [ ] Host 成功进入 Lobby。
- [ ] Find 能真实执行 Session Search。
- [ ] Session Browser 显示结果。
- [ ] Join 能真实 JoinSession。
- [ ] Client 能进入 Host Lobby。
- [ ] Session failure 有可见错误。
- [ ] Mission Session 不允许 Mid-Run Join。
- [ ] Main Menu UI 不泄漏到 Mission。
- [ ] Mission Gameplay Input 不受 Main Menu UI Only 残留影响。

---

# 41. 当前黑屏问题的最终判定规则

如果你看到：

```text
L_MainMenu
GameMode Override = None
Server 0 黑屏
Client 1 黑屏
```

正确判断顺序是：

```text
1. None = 没有地图级 Override，不代表没有 GameMode
2. 查 Project Settings Default GameMode
3. 查 Effective PlayerController
4. 查 Main Menu Widget 创建链
5. 查 AddToViewport
6. 查 InputMode/Mouse
7. 再谈 Session
```

并且：

```text
Server 0 / Client 1
```

只说明你启动的是：

```text
Multi-PIE Listen Server
```

不能替代：

```text
MainMenu → Host / Find / Join
```

的真实前端验证。

---

# 42. 后续修改规则

以后如果 Main Menu 的程序结构发生变化，本文件必须同步更新。

如果只是：

```text
修改 Main Menu 使用/验证说明
```

直接替换：

```text
Docs/MAIN_MENU_USAGE.md
```

不生成：

```text
MAIN_MENU_USAGE_v2.md
MAIN_MENU_USAGE_fix.md
MAIN_MENU_USAGE_new.md
```

如果需要补 Main Menu 固定 Editor 装配：

```text
提供 Tools/Editor 下的 Python 文件
```

如果需要修改：

```text
.h
.cpp
Build.cs
Config
```

提供只包含改动文件的增量 ZIP，并保持原始相对路径。

删除旧文件时，在交付回复中明确列出删除路径。

