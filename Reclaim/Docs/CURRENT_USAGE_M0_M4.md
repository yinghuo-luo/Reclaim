# Project Reclaim 当前操作手册 — M0–M4

> 引擎：Unreal Engine 5.6 / Windows PC  
> 网络方式：LAN / Listen Server  
> 入口地图：`L_MainMenu`  
> 适用项目：当前 Reclaim0 基线  
> 更新日期：2026-09-01

本文只说明当前工程从 MainMenu 开始的操作方式，以及 MainMenu、Lobby、Mission 的手动配置关系。所有配置直接在 Unreal Editor、Blueprint、地图 World Settings 和 C++ 默认值中完成。

## 1. 当前地图与资产路径

| 用途 | 内容路径 | 作用 |
|---|---|---|
| MainMenu 地图 | `/Game/Reclaim/World/Maps/L_MainMenu` | 游戏启动后的前端入口 |
| MainMenu GameMode | `/Game/Reclaim/Core/GameModes/BP_GM_MainMenu` | 创建 MainMenu PlayerController |
| MainMenu PlayerController | `/Game/Reclaim/UI/Menu/BP_PC_MainMenu` | 创建并显示 MainMenu Widget |
| MainMenu Widget | `/Game/Reclaim/UI/Menu/WBP_MainMenu` | Host、Find、Settings、Quit |
| Lobby 地图 | `/Game/Reclaim/World/Maps/L_Lobby` | Host 创建会话后的大厅 |
| Lobby GameMode | `/Game/Reclaim/Core/GameModes/BP_GM_Lobby` | 大厅玩家、职业和 Ready 规则 |
| Lobby Widget | `/Game/Reclaim/UI/Lobby/WBP_Lobby` | 选择职业、Ready、Start Mission |
| Mission 地图 | `/Game/Reclaim/World/Forest/L_Forest_Mission` | M0–M4 Gameplay 场景 |
| Mission GameMode | `/Game/Reclaim/Core/GameModes/BP_GM_Mission` | 根据玩家职业生成 Pawn |

当前主流程是：

```text
L_MainMenu
→ Host
→ 创建 LAN GameSession
→ L_Lobby
→ 选择职业并 Ready
→ Host Start Mission
→ L_Forest_Mission
```

## 2. 打开项目后的入口配置

### 2.1 Project Settings

打开 `Project Settings → Maps & Modes`，使用以下地图：

```text
Editor Startup Map = /Game/Reclaim/World/Maps/L_MainMenu
Game Default Map  = /Game/Reclaim/World/Maps/L_MainMenu
```

当前网络配置位于 `Config/DefaultEngine.ini`：

```ini
[OnlineSubsystem]
DefaultPlatformService=Null

[OnlineSubsystemNull]
bEnabled=true
```

这表示当前使用本机 LAN Session，不是 Steam、EOS 或其他线上服务。C++ 会在使用 `OnlineSubsystemNull` 时强制使用 LAN 查询；因此即使 `bLANSession` 在某个 Blueprint 实例中被误保存为 `false`，当前本地验证仍会走 LAN Session。

### 2.2 L_MainMenu 的地图配置

打开：

```text
/Game/Reclaim/World/Maps/L_MainMenu
```

在 `World Settings` 中设置：

```text
GameMode Override = BP_GM_MainMenu
```

`BP_GM_MainMenu` 的 `Class Defaults` 使用：

```text
Player Controller Class = BP_PC_MainMenu
Default Pawn Class      = None
HUD Class               = None
```

MainMenu 不生成战斗 Pawn。前端界面由 `BP_PC_MainMenu` 创建。

### 2.3 BP_PC_MainMenu 的配置

打开：

```text
/Game/Reclaim/UI/Menu/BP_PC_MainMenu
```

确认其父类为：

```text
ReclaimMainMenuPlayerController
```

在 `Class Defaults` 中设置：

```text
Main Menu Widget Class = WBP_MainMenu
Show Mouse Cursor      = true
Enable Click Events    = true
Enable Mouse Over Events = true
```

### 2.4 WBP_MainMenu 的配置

打开：

```text
/Game/Reclaim/UI/Menu/WBP_MainMenu
```

其父类应为：

```text
ReclaimMainMenuWidget
```

在 `Class Defaults` 中使用：

| 属性 | 当前值 | 说明 |
|---|---|---|
| `LobbyMap` | `/Game/Reclaim/World/Maps/L_Lobby` | Host 成功后进入的地图 |
| `HostMaxPlayers` | `4` | Host 最大玩家数，范围 1–4 |
| `bLANSession` | `true` | 使用 LAN Session |
| `SessionBrowserWidgetClass` | `/Game/Reclaim/UI/Menu/WBP_SessionBrowser` | Find 后显示的 Session Browser；为空时使用当前默认路径 |

Widget Tree 中必须保留以下变量名，因为 C++ 会按这些名字绑定按钮和文字：

```text
HostButton
FindButton
SettingsButton
QuitButton
StatusText
```

按钮文字和布局可以继续调整，但不要修改上面的变量名。

## 3. MainMenu 的手动装配

当前固定配置直接在 Unreal Editor 的 Blueprint Class Defaults 和地图 World Settings 中完成，不依赖外部自动化文件。按下面顺序检查并保存资产。

1. 打开 `/Game/Reclaim/UI/Menu/WBP_MainMenu`，确认父类为 `ReclaimMainMenuWidget`。
2. 在 `WBP_MainMenu` 的 Class Defaults 中设置：

   ```text
   LobbyMap = /Game/Reclaim/World/Maps/L_Lobby
   HostMaxPlayers = 4
   bLANSession = true
   SessionBrowserWidgetClass = /Game/Reclaim/UI/Menu/WBP_SessionBrowser
   ```

   `SessionBrowserWidgetClass` 为空时，C++ 会使用当前默认路径；建议直接填写上面的 Widget。
3. 在 Widget Tree 中保留以下控件变量名。可以调整文字、颜色和布局，但不要重命名：

   ```text
   HostButton
   FindButton
   SettingsButton
   QuitButton
   StatusText
   ```

4. 打开 `/Game/Reclaim/UI/Menu/BP_PC_MainMenu`，确认父类为 `ReclaimMainMenuPlayerController`，并设置：

   ```text
   MainMenuWidgetClass = /Game/Reclaim/UI/Menu/WBP_MainMenu
   Show Mouse Cursor = true
   Enable Click Events = true
   Enable Mouse Over Events = true
   ```

5. 打开 `/Game/Reclaim/Core/GameModes/BP_GM_MainMenu`，设置：

   ```text
   PlayerControllerClass = /Game/Reclaim/UI/Menu/BP_PC_MainMenu
   DefaultPawnClass = None
   HUD Class = None
   ```

6. 打开 `/Game/Reclaim/World/Maps/L_MainMenu`，在 World Settings 中设置：

   ```text
   GameMode Override = /Game/Reclaim/Core/GameModes/BP_GM_MainMenu
   ```

保存以上四个资产后，MainMenu 的 Host、Find、Settings 和 Quit 操作即可使用。

## 4. 从 MainMenu 开始的实际操作

### 4.1 进入 MainMenu

1. 先停止 Unreal Editor 中正在运行的 PIE/Multi-PIE。
2. 启动第一个独立游戏进程，并进入 `L_MainMenu`。验证时使用 Development 包生成的游戏 `.exe`，不要从 Unreal Editor 的 Multi-PIE 窗口代替。
3. 再启动第二个独立游戏进程，进入同一个 `L_MainMenu`。两个窗口必须来自两个独立的游戏进程。
4. 等待两个 MainMenu 都出现，再使用鼠标操作按钮。MainMenu PlayerController 已设置为 UI 输入模式并显示鼠标。

Session 验证不要使用 Unreal Editor 的 `Number of Players=2 + NetMode=Standalone` Multi-PIE。该方式会生成 `UEDPIE_0`、`UEDPIE_1` 和不同的 `OnlineSubsystem :Context_*`，不是两个可互相发现的独立游戏进程。

### 4.2 Host

点击：

```text
Host
```

当前流程：

```text
HostButton
→ UReclaimSessionSubsystem.HostSession(HostMaxPlayers, bLANSession)
→ 创建 GameSession
→ 进入 L_Lobby
```

Host 使用 Listen Server。Host 是大厅中的服务器玩家，也是之后可以点击 `Start Mission` 的玩家。进入 Lobby 后不要关闭 Host 进程。

如果上一次运行留下了同名 `GameSession`，再次点击 Host 时，当前 Session 逻辑会先清理已有会话，再创建新的会话。创建期间只点击一次 Host。

### 4.3 Find

点击：

```text
Find
```

当前按钮会执行：

```text
FindSessions(MaxResults=50, bIsLAN=true)
```

并将可加入的 LAN Session 数量写入 MainMenu 的 `StatusText`。

点击 Find 后会同时打开 Session Browser。搜索完成后，Session Browser 会显示当前可加入的 LAN Session，并可直接进行 Join。

独立的 Session Browser 资产是：

```text
/Game/Reclaim/UI/Menu/WBP_SessionBrowser
```

Session Browser 按以下按钮操作：

| Widget 控件 | 操作 |
|---|---|
| `RefreshButton` | 搜索最多 50 个 LAN Session |
| `JoinFirstButton` | 加入当前选中的可加入 Session；未选择时加入第一个可加入 Session |
| `BackButton` | 关闭 Session Browser |
| `SessionListText` | 显示可加入 Session |
| `StatusText` | 显示搜索或 Join 状态 |

Join 成功后，客户端会自动进入 Host 所在的 Lobby。

### 4.4 Settings

当前 Settings 仅保留按钮入口，点击后会提示 M0 尚未配置 Settings 页面。不要把它当成已经存在的分辨率、音频或键位设置界面。

### 4.5 Quit

点击：

```text
Quit
```

关闭当前游戏实例。

## 5. Host 进入 Lobby 后的操作

### 5.1 Lobby 配置关系

打开：

```text
/Game/Reclaim/World/Maps/L_Lobby
```

在 `World Settings` 中设置：

```text
GameMode Override = BP_GM_Lobby
```

`BP_GM_Lobby` 的父类为：

```text
ReclaimLobbyGameMode
```

其 `MissionMap` 设置为：

```text
/Game/Reclaim/World/Forest/L_Forest_Mission
```

在 Details 面板中建议使用资产选择器选择 `L_Forest_Mission`。如果手动粘贴成
`/Game/Reclaim/World/Forest/L_Forest_Mission.L_Forest_Mission`，C++ 会自动取其 Long Package Name；最终 Server Travel 使用的仍必须是：

```text
/Game/Reclaim/World/Forest/L_Forest_Mission
```

大厅 Widget：

```text
/Game/Reclaim/UI/Lobby/WBP_Lobby
```

其父类为：

```text
ReclaimLobbyWidget
```

当前 Lobby PlayerController 会在检测到 `L_Lobby` 后创建 `WBP_Lobby` 并添加到屏幕，不需要在 Lobby Level Blueprint 中另外创建一套 Widget。

### 5.2 选择职业

进入 Lobby 后，选择一个未被其他玩家占用的职业：

```text
Vanguard
Ranger
Engineer
Warden
```

每个职业同时只能被一名玩家占用。玩家 Ready 后不能继续切换职业；需要先取消 Ready，再选择其他职业。

### 5.3 Ready

完成职业选择后点击：

```text
Ready
```

Start Mission 的前置条件是：

- 每个已连接玩家都选择了职业；
- 每个已连接玩家都处于 Ready；
- 职业没有重复；
- 玩家数量为 1–4 人。

一名 Host 可以独自选择职业、Ready 并开始任务，不要求必须有四名玩家。

### 5.4 Start Mission

只有 Listen Server 的 Host 可以点击：

```text
Start Mission
```

点击后，所有已连接玩家从 Lobby 一起进入：

```text
/Game/Reclaim/World/Forest/L_Forest_Mission
```

Mission GameMode 会根据 Lobby 中保存的 `SelectedRole` 生成对应职业 Pawn，不要在 MainMenu 或客户端本地手工指定职业 Pawn。

## 6. Mission 中的当前操作键位

当前基础键位来自 `Config/DefaultInput.ini`：

| 操作 | 键位 |
|---|---|
| 前进 | `W` |
| 后退 | `S` |
| 左移 | `A` |
| 右移 | `D` |
| 视角转动 | 鼠标移动 |
| 跳跃 | `Space` |
| 开火 | 鼠标左键 |
| 换弹 | `R` |
| 切换武器 | `1` |
| 职业技能 1 | `Q` |
| 职业技能 2 | `E` |
| 交互 | `F` |

Mission 中使用的是 Gameplay 输入，不再使用 MainMenu 的 UI 输入模式。

## 7. MainMenu 相关资产的手动配置清单

按下面顺序配置即可：

### MainMenu

```text
L_MainMenu.WorldSettings.GameModeOverride = BP_GM_MainMenu
BP_GM_MainMenu.PlayerControllerClass = BP_PC_MainMenu
BP_GM_MainMenu.DefaultPawnClass = None
BP_PC_MainMenu.MainMenuWidgetClass = WBP_MainMenu
WBP_MainMenu.LobbyMap = L_Lobby
WBP_MainMenu.HostMaxPlayers = 4
WBP_MainMenu.bLANSession = true
WBP_MainMenu.SessionBrowserWidgetClass = WBP_SessionBrowser
```

### Lobby

```text
L_Lobby.WorldSettings.GameModeOverride = BP_GM_Lobby
BP_GM_Lobby.MissionMap = L_Forest_Mission
BP_GM_Lobby.PlayerControllerClass = ReclaimPlayerController
WBP_Lobby.ParentClass = ReclaimLobbyWidget
```

### Mission

```text
L_Forest_Mission.WorldSettings.GameModeOverride = BP_GM_Mission
BP_GM_Mission.ParentClass = ReclaimMissionGameMode
```

## 8. Widget 变量名约定

不要修改下面这些名字。它们是 C++ `BindWidget` 和按钮事件绑定使用的名字。

### WBP_MainMenu

```text
HostButton
FindButton
SettingsButton
QuitButton
StatusText
```

### WBP_SessionBrowser

```text
RefreshButton
JoinFirstButton
BackButton
SessionListText
StatusText
```

### WBP_Lobby

```text
VanguardButton
RangerButton
EngineerButton
WardenButton
ReadyButton
StartMissionButton
LobbyStatusText
PlayerSlotsText
RoleReservationsText
LastRoleResultText
ReadyButtonText
StartMissionButtonText
```

## 9. 资产移动或重命名时的处理

当前 C++、Blueprint 和地图使用上面的稳定资产路径。移动资产时，同时更新：

1. `Config/DefaultEngine.ini` 中的默认地图；
2. `L_MainMenu` 的 GameMode Override；
3. `BP_PC_MainMenu.MainMenuWidgetClass`；
4. `WBP_MainMenu.LobbyMap`；
5. `BP_GM_Lobby.MissionMap`；
6. `WBP_MainMenu.SessionBrowserWidgetClass`；
7. 如果 `SessionBrowserWidgetClass` 保持为空，还要同步更新 `Source/Reclaim/UI/ReclaimUIWidgets.cpp` 中的默认 Widget 路径。

如果只是替换按钮样式、文字、颜色、材质或背景，不需要修改这些路径和变量名。
