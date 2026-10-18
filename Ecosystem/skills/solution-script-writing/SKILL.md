---
name: solution-script-writing
description: Use when writing, editing, or reviewing 解决方案脚本语言 files (*.lan) and their parameter tables (*.pi). Covers graph replacement syntax, [] parallel subgraphs, node states, parameter table format, mapping lookup via md docs (e.g. NewUIntElement.md, UIntAdd.md), and how to pick maps from user requirements.
---

# 解决方案脚本语言编写指南

本 skill 指导如何编写问答流图的解决方案脚本（`.lan` 主文件 + `.pi` 参数表文件）。
权威语法文档位于项目内：`Solution_Language_Syntax.md`（索引）、`Language_Syntax.md`（主语言）、`Parameter_Syntax.md`（参数表）。
动手前应先阅读这三个文件核对规则。

## 1. 文件结构

一个解决方案由两个文件构成：

- 主文件（`*.lan`）：描述问答流图的替换规则——替换前图与替换后图。
- 参数表文件（`*.pi`）：登记主文件中出现的每个参数的完整信息。

主文件语法只写参数名；参数的其余信息一律在参数表中查表获得。

## 2. 图块（花括号）

主文件顶层由并列的花括号对组成，每两个花括号构成一组「前图 / 后图」：

```text
{
    // 替换前问答流图
}

{
    // 替换后问答流图
}
```

- 前图为空 `{ }` 时表示从零构造整张图（引导/bootstrap 型方案）。
- 后图描述替换后的节点与边结构。前图中的节点需要被匹配，后图中的节点在替换后进入后推执行。

## 3. 节点

每个节点固定四段、按顺序书写、每段独占一行：

```text
Map Information
( Domain Parameter )
( Range Parameter )
Node State
```

- 映射检索信息：映射名，需能在智能体环境的公共检索、私有检索或静态库检索中唯一确定为一个原子映射。
- 定义域 / 值域参数：写参数名，用小括号包裹多个兄弟参数（参数以序列树形式组织，括号内并列表示兄弟）。
- 节点状态：固定关键字之一。

示例（定义域为两个参数，值域一个参数）：

```text
UIntAdd
( UIntA UIntB )
UIntC
TraUntractable
```

## 4. 节点状态

| 状态 | 含义 |
|---|---|
| `Tractable` | 可执行：前置节点全部完成、映射与参数均唯一确定后即可分配轮询机启动线程 |
| `Pending` | 等待：前置已完成但仍需等待外部条件 |
| `Untractable` | 不可执行：必须等前置节点解决完成；前置完成后只进 Pending |
| `TraUntractable` | 特殊不可执行：前置节点全部完成后**直接**转为可执行，不经等待，可加速图后推、减少询问次数 |

用法要点：

- 一条链上无外部条件等待的节点用 `TraUntractable`，让执行流最快后推。
- 整个图中通常只有入口节点（无前置节点者）设为 `Tractable`。
- 受外部条件调控的节点才用 `Pending` / `Untractable`。

## 5. 并行子图（方括号 `[]`）

用方括号对表示节点间存在并行关系。具有并行关系的子图与**前置节点全连接**、与**后置节点全连接**。

### 5.1 何时使用 `[]`

- 多个节点之间没有数据依赖（互不需要对方的输出），且都只依赖同一组前置节点。
- 希望这些节点同时被分配执行、并行后推时。

反例（不要用 `[]`）：节点 B 需要节点 A 的输出作为定义域参数——这是串行依赖，直接顺序书写即可。

### 5.2 硬性规则：`[]` 块内首尾必须是节点

**每个 `[]` 代码块的第一个元素和最后一个元素必须是普通节点（映射+参数+状态），不允许出现嵌套的 `[]` 并行块。**

并行块只能在与节点同级的层次并列，不能嵌套：

```text
// 正确：三个并行块与前后节点同级，块内首尾都是节点
    BeforeNode
    ...
    TraUntractable

    [

        WorkA
        EmptyElement
        ResultA
        TraUntractable

    ]
    [

        WorkB
        EmptyElement
        ResultB
        TraUntractable

    ][

        WorkC
        EmptyElement
        ResultC
        TraUntractable

    ]

    AfterNode
    ...
    TraUntractable
```

```text
// 错误：块内嵌套了 []，禁止
    [
        [
            InnerNode
            ...
        ]
    ]
```

### 5.3 多层并行的表达方式

需要「不同阶段各自并行」时，拆成多组同级的 `[]` 序列，组与组之间用普通节点或组间先后关系区分（见第 6 节实例的收尾部分）。

## 6. 完整案例解读

`SolutionBootstrap.lan` 是一个 bootstrap 方案：前图为空，后图构造「建元素 → 自增 → 求和 → 释放 → 输出」的流水线。

```text
{
}
{
    [
        NewUIntElement
        EmptyElement
        UIntA
        Tractable

        UIntSuccessor
        UIntA
        UIntA
        TraUntractable

    ][

        NewUIntElement
        EmptyElement
        UIntB
        Tractable

        UIntSuccessor
        UIntB
        UIntB
        TraUntractable

    ][
        NewUIntElement
        EmptyElement
        UIntC
        Tractable
    ]

    UIntAdd
    ( UIntA UIntB )
    UIntC
    TraUntractable

    [

        ReleaseElement
        EmptyElement
        UIntA
        TraUntractable

    ]
    [

        ReleaseElement
        EmptyElement
        UIntB
        TraUntractable

    ][

        ReleaseElement
        EmptyElement
        UIntC
        TraUntractable

    ]

    CMDPutHelloWorld
    EmptyElement
    EmptyElement
    TraUntractable
}
```

阅读要点：

- 方括号内节点顺序书写构成子图内的先后依赖（如 `NewUIntElement` → `UIntSuccessor`）。
- 三个并行建元素分支同时依赖前图（空），故入口节点均可 `Tractable`。
- `UIntAdd` 是三个并行分支的共同后继，吃全部三条分支的输出。
- 释放三元素用一组并行块，之后接输出节点；再需要并行输出时再接一组新的并行块。

## 7. 参数表（`*.pi`）

参数表按行记录，每行五个字段，以空白分隔：

```text
参数名称  类型名称  基础信息  语境名称  参数状态
```

示例：

```text
EmptyElement  EmptyElement  EmptyElement  Unlimited  Common
UIntA String UIntA Unlimited Login
UIntB String UIntB Unlimited Login
UIntC String UIntC Unlimited Login
```

字段说明：

- **参数名称**：数据库主键，必须与主文件中出现的参数名逐字符一致（无空格/制表符）。
- **类型名称**：仅四个关键字 `String` / `UInt` / `SuperIndex` / `Bool`。注意该类型只用于信息生成，**不是**映射运行时的真实数据类型，因此写 `String` 不影响 `UIntSuccessor` 等数值映射的执行。
- **基础信息**：由类型决定格式的基础数据，用于信息生成。
- **语境名称**：须在语境表中可检索。特殊语境 `Unlimited` 表示不需要语境信息，由语境字典统一分配——可为同名或不同语境，编写时优先使用 `Unlimited` 以免遗漏语境表登记。
- **参数状态**：`Login`（优先注册）/ `Logout`（优先注销）/ `Common`（不注册不注销）/ `LoginOut`（注册并注销）。需要在环境中被后续方案检索使用的参数（如计算用的 UIntA/UIntB/UIntC）设 `Login`；空元素这类一次性占位参数设 `Common`。

**字段顺序易错点**：语境名称（含特殊关键字 `Unlimited`）固定在**第 4 位**，参数状态固定在第 5 位。不要把 `Unlimited` 写到第 3 位——第 3 位是基础信息。写完后按「名称 / 类型 / 基础信息 / 语境 / 状态」逐位核对。

### 7.1 `EmptyElement` 特殊参数

`EmptyElement` 是**空元素**，是脚本中最常用的占位/辅助参数：

- 作为 `NewUIntElement`、`ReleaseElement` 等映射的定义域（这类映射的定义域恒为 NullElementSet/NullElement），表示「不需要真实输入」。
- 作为 `CMDPutHelloWorld` 等输出类映射的定义域与值域。
- 元素被释放后归零，与 `EmptyElement` 等价——因此释放映射的定义域也写 `EmptyElement`。
- 参数表中固定写法：`EmptyElement  EmptyElement  EmptyElement  Unlimited  Common`（第 4 位是语境 `Unlimited`，第 5 位是状态 `Common`）。
- `EmptyElement` 在同一目录的参数表中登记一次即可，所有方案共用；其余参数才按方案登记。

### 7.2 新增参数的步骤

1. 在 `.lan` 中确定参数名（无空格的字符串）。
2. 在 `.pi` 追加一行 `参数名 类型名 基础信息 语境名 参数状态`。
3. 若该参数需被其他方案后续检索，状态写 `Login`。

## 8. 通过文档查找映射

### 8.1 已知映射时：查它的参数要求

每个映射有独立的 md 文档（如 `NewUIntElement.md`、`ReleaseElement.md`、`UIntAdd.md`、`UIntSuccess.md`）。文档中的 `Map ...(...)` 定义行给出定义域集合与值域集合：

| 文档 | 映射 | 定义域 | 值域 | 行为 |
|---|---|---|---|---|
| NewUIntElement.md | NewUIntElement | NullElementSet | NullElementSet | 在值域空元素上构造 UInt 元素，初值 0 |
| UIntSuccess.md | UIntSuccessor | UIntSet | UIntSet | 值域 = 定义域的后继值（+1） |
| UIntAdd.md | UIntAdd | OrderedPairSet | UIntSet | 值域 = 序偶两元素之和 |
| ReleaseElement.md | ReleaseElement | NullElement | UniversalSet | 释放值域元素内存并归零 |

写节点前必须打开对应 md 核对：

1. 定义域需要几个参数、是否要写成 `( P1 P2 )` 序偶形式。
2. 值域集合是否与传入参数名匹配（如 UIntAdd 值域必须是 UInt 元素）。
3. 是否有注意事项（如 UIntAdd 的值域应避免与定义域元素重合）。

### 8.2 未知映射时：按需求检索

用户提出需求（如「输出 helloworld」「创建一个整数」「相加」「释放」）时：

1. 用 Grep/Glob 在当前目录的 `*.md` 中检索需求关键词（中英文都试：输出/hello/print、加/add/sum、后继/successor/increment、释放/release/free……）。
2. 从命中的映射文档中读取 `Map` 定义行，筛出定义域/值域与手头参数集合匹配的映射。
3. 若文档中没有符合要求的映射，明确告知用户缺该映射，并询问是用现有映射组合实现还是需要新增映射。
4. 确定映射后按 8.1 核对参数要求，再写节点。

### 8.3 编写顺序

1. 从用户需求抽出节点序列与依赖关系。
2. 无依赖的节点归入同级 `[]` 并行块（首尾必须是节点，禁止嵌套）。
3. 每个节点查映射文档确定定义域/值域参数个数与写法。
4. 入口节点 `Tractable`，其余无条件等待的节点 `TraUntractable`。
5. 主文件中出现的每个参数名（除已在参数表中如 `EmptyElement`）在 `.pi` 中登记一行。
6. 自查：并行块首尾是节点、无嵌套 `[]`、每个释放/创建配对、参数表覆盖全部参数名。
