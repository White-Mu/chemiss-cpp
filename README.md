# Chemiss 化学棋 - C++ 后端 + HTML 前端

一个基于化学原理的策略棋类游戏，灵感来源于 [Chemiss](https://github.com/Bil812/Chemiss)。

## 游戏简介

Chemiss（化学棋）是以原子成键、电子转移、放射性衰变等化学原理为核心的两人策略棋类游戏。

### 游戏目标
吃掉对方的 **氢王（H）** 棋子即可获胜。

### 核心机制
- **离子键吃子**：金属与非金属之间，电负性差 > 1.7 时可直接吃掉对方
- **共价键/金属键**：相邻棋子自动成键，键合状态的棋子可被第三方攻击
- **电性吸引**：带电棋子回合开始时强制相互吸引
- **金属给电子**：金属可向相邻己方非金属转移电子
- **锂核变**：Li 到达对方底线可核变为周期表任意元素
- **放射性衰变**：放射性同位素有几率衰变并发射 α/β/γ 射线
- **射线系统**：射线可诱发嬗变、裂变或眩晕

### 初始布局
```
黑方: O P Si F H C N S  (第0行)
      Na Na Li Li Li Li Na Na  (第1行)

白方: Na Na Li Li Li Li Na Na  (第6行)
      O P Si F H C N S  (第7行)
```

## 技术架构

| 层级 | 技术 |
|------|------|
| 后端 | C++17 + 自定义 WebSocket 服务器 |
| 前端 | HTML5 + CSS3 + Vanilla JavaScript |
| 通信 | WebSocket JSON 协议 |
| 构建 | CMake + MinGW |

## 快速开始

### 1. 克隆仓库
```bash
git clone <repository-url>
cd chemiss-cpp
```

### 2. 构建项目

**使用 MinGW (推荐)**:
```bash
cmake -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/mingw64/bin/g++.exe" -DCMAKE_MAKE_PROGRAM="C:/mingw64/bin/mingw32-make.exe"
cmake --build build
```

**使用 Visual Studio (MSVC)**:
```bash
cmake -B build
cmake --build build --config Release
```

### 3. 运行服务器
```bash
./build/chemiss-server.exe
# 默认端口 8080
```

### 4. 打开浏览器游玩
访问 `http://localhost:8080`

- **白方先行**
- 点击己方棋子查看合法移动（绿色高亮）
- 再次点击目标格子移动
- 点击敌方棋子尝试吃子

## 项目结构

```
chemiss-cpp/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp                    # 服务器入口
│   ├── game/
│   │   ├── element.h/.cpp          # 元素周期表数据
│   │   ├── piece.h/.cpp            # 棋子类
│   │   ├── board.h/.cpp            # 棋盘类
│   │   ├── rules.h/.cpp            # 游戏规则引擎
│   │   └── game_state.h/.cpp       # 游戏状态管理
│   ├── network/
│   │   ├── websocket_server.h/.cpp  # WebSocket/HTTP 服务器
│   │   └── SHA-1 + Base64 实现
│   └── utils/
│       ├── json_utils.h/.cpp       # JSON 序列化工具
│       └── ...
└── frontend/
    ├── index.html                    # 游戏主页面
    ├── css/
    │   └── style.css                 # 棋盘与棋子样式
    └── js/
        └── app.js                    # 前端逻辑与通信
```

## 已实现规则

- [x] 标准 8×8 棋盘与初始布局
- [x] 基于周期/阵营的移动方向修正
- [x] 离子键直接吃子（电负性差 > 1.7）
- [x] 共价键/金属键自动形成与解除
- [x] 键合状态下的条件吃子
- [x] 电性吸引（强制移动）
- [x] 金属给电子（阳离子/阴离子）
- [x] Li 核变（周期表选元素 + 同位素）
- [x] 放射性衰变系统（α/β/γ）
- [x] 射线效果（嬗变、裂变、眩晕）
- [x] 稀有气体规则（不参与键合、不可被吃）
- [x] 氢王胜负判定
- [x] 和棋系统

## 通信协议

客户端 ↔ 服务器通过 WebSocket 发送 JSON 消息：

| 消息类型 | 说明 | 示例 |
|---------|------|------|
| `get_state` | 请求完整状态 | `{"type":"get_state"}` |
| `move` | 移动棋子 | `{"type":"move","pieceId":1,"to":{"row":5,"col":3}}` |
| `legal_moves` | 请求合法移动 | `{"type":"legal_moves","pieceId":1}` |
| `transmute` | 核变 | `{"type":"transmute","pieceId":10,"element":"Fe","massNumber":56}` |
| `donate` | 给电子 | `{"type":"donate","donorId":20,"recipientId":15}` |
| `draw` | 请求和棋 | `{"type":"draw"}` |
| `accept_draw` | 同意和棋 | `{"type":"accept_draw"}` |
| `new_game` | 新游戏 | `{"type":"new_game"}` |

## 依赖

- C++17 兼容编译器（GCC, Clang, MSVC）
- CMake 3.16+
- 浏览器（Chrome, Firefox, Edge 等）

无需外部库！所有网络、加密（SHA-1/Base64）均为自实现。

## 许可证

本项目允许个人学习、研究、修改和二次开发。你可以自由地 Fork、修改和分发代码，但**禁止用于商业用途**（包括但不限于直接销售、嵌入商业产品、提供付费服务等）。

如果你基于此项目做出了有趣的改进，欢迎分享回来！

## 致谢

灵感来源于 [Bil812/Chemiss](https://github.com/Bil812/Chemiss)，原作为纯前端实现。本项目将其重构为 C++ 后端 + HTML 前端，以支持更复杂的计算和更好的可扩展性。
