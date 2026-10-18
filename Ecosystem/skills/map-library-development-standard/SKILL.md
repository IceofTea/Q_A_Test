---
name: map-library-development-standard
description: 'Use when developing or extending the C++ library behind the 解决方案脚本语言 (E_S_M-Project style: Element/Set/Map), wrapping functionality as 映射 (maps), designing new Map/Set/Element, splitting a user requirement into atomic 基本指令/映射, storing library information via 意向性对象 (质性/质料, Intentionality-Project), or reviewing 库开发 code. Triggers: 库映射开发标准, 映射开发, 封装映射, 元素集合, 意向性对象, E_S_M.'
---

# 库映射开发标准

本 skill 规定为解决方案脚本语言开发 C++ 库时的标准。脚本语言只关注**逻辑执行顺序**，一切可执行动作都必须是库中预先封装好的**映射（Map）**，一切被操作的对象都必须封装为**集合（Set）**与**元素（Element）**。

权威资料在 `E_S_M-Project/` 目录：

- `E_S_M/Element.md`、`E_S_M/Set.md`、`E_S_M/Map.md` —— 三大结构体定义
- `E_S_M/E_S_M_Construct_And_Use.md` —— 构造/析构/封装用法
- `ESM_Base/ESM_Base.md` 及其子文档 —— 已有映射与集合的清单（开发前先查这里）
- `Exception/Exception.md` 及其子文档 —— 异常码体系

## 1. 三大核心抽象

### Element（元素）—— 操作单位

```C++
struct Element {
    unsigned int Size;
    char* KeySpace;
    void operator&=(Element& Ele);   // 转交传递：旧元素被置空，目标应为空元素
    void operator=(const Element& Ele);   // 深拷贝
    void operator|=(const Element& Ele);  // 引用传递：需 ZeroElement 解除，否则重复释放
};
```

- 一切操作发生在具体元素上；脚本节点的参数本质都是元素。
- 内存纪律三原则：
  1. **简单结构**（只在 KeySpace 上申请一块内存，如 UInt 元素）：可用通用 `ReleaseElement` 释放，或依赖析构。
  2. **复杂结构**（内部还有子内存，如序偶、文件、检索器）：必须提供**专用释放映射**，禁止只用 `ReleaseElement`。
  3. **引用元素**（`|=` 产生）：内核不记录引用，使用完必须 `ZeroElement` 解除，再释放实体元素。

### Set（集合）—— 操作的发生环境

```C++
typedef bool (*SetTestFunction)(Element& Ele, Set& S);
struct Set {
    Element SetParameter;        // 泛化集合的特化参数
    SetTestFunction TestFunction; // 判断 Ele 是否属于 S
};
```

- 所有操作都在某个集合的语义内发生；元素「属于」哪个集合决定了它能用哪些映射。
- Set 运行期基本不变，不要设计依赖 Set 可变的逻辑。

### Map（映射）—— 唯一的操作形式

```C++
typedef bool (*MapFunction)(Element& Domain, Element& Range);
struct Map {
    Set* Domain;   // 定义域集合：约束入参
    Set* Range;    // 值域集合：约束出参
    MapFunction MapActive;
};
```

- **内核与脚本层只接受 Map，绝不直接接受 Set/Map 结构体**；要在内核中使用 Set/Map 结构，必须封装为元素（`Element(sizeof(Set), (char*)&SomeSet)`，配合宏 `GetSetElement()` / `GetMapElement()`）。
- 一个映射 = 定义域集合 + 值域集合 + 执行函数，返回 bool 表示执行状况。
- 映射的定义行（写进 md 文档的那行）是对外契约：

```C++
Map NewUIntElement(&NullElementSet, &NullElementSet, NewUIntElementFunction);
```

### ESM 设计模式总览（元素-集合-映射）

E_S_M-Project 是该范式的参考实现，新增任何库/模块都按此三元结构组织：

1. **一切皆元素**：Element 是唯一数据载体，脚本参数、中间结果、甚至集合/映射结构体本身都封装为元素（`SetSet`/`MapSet` 集合 + `GetSetElement`/`GetMapElement` 宏），内核得以统一处理。
2. **类型即集合**：数据类型不是语言层概念，而是集合判定。每种数据 = 一个 `Set` + `TestFunction`；元素通过 `XxxSetTest` 才算属于该类型，也才能作为对应映射的合法参数。
3. **操作即映射**：所有动作都写成 `Map`。C++ 代码层的操作符/宏与内核映射一一对应，设计新库时双向核对：
   - `&=` 转交 ↔ `PutPairFirstElement` / `GetPairFirstElement` 等 Put/Get 系列
   - `|=` 引用 ↔ `QuoteElement`
   - `GetEleOrder` 下标解析 ↔ `UIntGetElementOrder` / `UIntPutElementOrder`（内核需按地址操作时）
   - 析构/归零 ↔ `ReleaseElement` / `ZeroElement`
   凡代码中出现操作符/宏，检查内核是否需要对应映射；每个内核映射也应能在代码中找到等价写法（便于案例文档展示等价关系）。
4. **静态元素填充**：`EmptyElement`、`EmptyMap` 是零值静态元素，专用于只操作定义域或值域单侧的映射占位。
5. **复合结构 = 元素序列/元素树**：复杂对象用 `ElementOrderSet`（宏 `GetEleOrder` 按下标解析、可嵌套）组织，不自定义黑盒结构（呼应原则 8）。

## 2. 开发原则

1. **功能必须封装成映射**：脚本语言节点的第一行永远是映射名。代码里能调用的普通函数，若要被脚本使用，必须包一层 `MapFunction(Element&, Element&)`。
2. **对象必须封装成集合**：新数据类型先定义 `Set XxxSet(参数, XxxSetTest)`，元素构造后能通过该集合的判定，才可参与相关映射。
3. **定义域/值域是硬契约**：入参集合写错，节点在图后推时无法唯一确定原子映射。设计映射时先写集合，再写函数。
4. **按功能族命名**：`NewX`（构造）、`ReleaseX`（释放）、`GetX`（取出，如 GetUIntElement/GetPairFirstElement）、`PutX`（存入，如 PutPairFirstElement）、`XFunction`（供 C++ 调用的函数体）与 `Map X`（供脚本/内核检索的映射符号）成对出现。
5. **保持映射无状态**：映射不在运行期大规模更改自身，状态只落在元素上。
6. **转交 vs 引用要显式**：转交（`&=`、Put/Get 系列）保持单实体；引用（`|=`、Quote）必须显式解除。
7. **释放必须配对**：每个 `NewX` 映射都要有对应的释放方案（通用 `ReleaseElement` 或专用 `ReleaseX`），并在文档中写明适用范围。
8. **可操作对象设计为元素序列/元素树**：库中那些在系统内核运行中**需要被脚本层直接操作其内部信息**的复合对象（如 ESM_Manage 项目中的 ElementEnvironmentSearcher），不要封装成黑盒整体元素，而应设计成**元素序列（序偶/ElementOrder，经宏 GetEleOrder 按下标解析）或元素树（序列嵌套）**的形式——序列的每个槽位是一个子元素，脚本层用一串 `GetEleOrder` 即可取出/替换元素下的全部信息，无需为每个内部字段发明一套专用映射。
   - **判定标准（设计前必问）**：该元素在系统内核运行过程中是否需要被拆开操作其内部信息？
     - **需要** → 必须按元素序列/元素树布局，槽位布局（下标含义、嵌套层次）写进集合与映射文档，使通用的解析宏即可覆盖操作需求。
     - **不需要**（内部信息只在某个专用映射内部消费，脚本层只关心整体）→ 保持简单/复杂结构的整体封装，只需提供够用的专用映射即可，避免过度暴露内部结构。
   - 槽位内的子元素在代码块结束时不会自动析构，序列式对象的释放需遍历槽位逐个释放（或提供专用释放映射）。
9. **需要落盘/登记的信息用意向性对象存储**：库开发中凡是「要把信息记录到文件/系统中」的需求（配置、表单、日志、检索表……），不要为每类信息自造一套文件读写或容器管理，一律复用**意向性对象**（Intentionality-Project，参考文档 `Lib_Introduce/Intentionality-Project/`）：
   - 意向性对象 = **质性**（指向信息的意向）+ **质料**（实际存储信息的位置）。存入时更新质料，获取时直接读取质料。
   - 标准存取链（基础模块 `Intentionality.h` 九项映射按需选用）：
     1. `NewIntentionalityObject` 构造对象 → `AddIntentionalityObject`（定义域 SuperIndexSet）登记质性并分配地址；
     2. **存**：`PutIntentionalityObject`（序偶 <UIntSet 地址, StreamSet 信息>）把信息写入质料；
     3. **取**：`ReadIntentionalityObject`（引用，用后必须 `ZeroElement`）或 `GetIntentionalityObject`（转交所有权，用后自行释放/归还）；
     4. `SelectIntentionalityObject` 按超索引检索质性，`RemoveIntentionalityObject` / `ReleaseIntentionalityObject` 收尾。
   - **按超索引从总元素环境检索器直达**：意向性对象随知识库加载流程注册到总元素环境检索器（EES）后，运行期只需按其超索引信息从总元素环境检索器中读取到该意向性对象，再读取对应信息——因此库代码不必自建全局注册表。
   - **库开发的职责边界**：只开发「如何构造信息」与「如何使用信息」（信息的字段布局、语义、合法性约束）；意向性对象的登记/检索/存取指令是通用基础设施，直接调用——指明质性即可把信息存入质料，或从质料读出，不要重复实现存储机制。
   - 副作用纪律（写进文档）：`Add`/`Remove`/`Put`/`Release` 会改变对象内部结构，此前 `Read` 得到的引用随之失效；`Get` 转走质料后同地址再 Get 得到空元素。
10. **信息使用的职责边界：绑定 / 字典记录 / 意向性对象三类判定**。设计两个被操作元素之间的关系时（参照「总元素环境检索器与元素环境」的绑定范式），先回答「被操作元素是否需要提供自己的信息来帮助对方运行、信息是否相互独立」，据此三选一：
    - **绑定关系（需要协作运行）**：被绑定元素要提供自身信息参与绑定方的运行 → 绑定方结构**预留槽位**给被绑定元素；用 **Set/Put 映射完成绑定（设置）**，用 **Get 映射完成解绑**（槽位取空）；若只传入元素**引用**参与运行，则无需解绑，但引用方必须按引用纪律 `ZeroElement` 解除。槽位含义、绑定/解绑映射、释放时是否连带槽内元素写进文档。
    - **字典记录（简单字段登记）**：无符号整型、流类型（Stream）、布尔类型等简单信息的记录 → 直接把记录元素**构造到字典条目内部**（条目的槽位），随条目一起释放，不单独绑定/解绑，也不走意向性对象。
    - **意向性对象（复杂、多跳、相互独立）**：信息获取需要**多次查询**、且各条信息**相互独立、无需绑定** → 用意向性对象链式存储：每次查询经「意向性（质性 → 质料）」取质料；当质料元素为**超索引类型**时，以其超索引信息定位**下一层意向性对象**，逐层查询，直到最后一层意向性对象中「存储信息」那条质性的质料直接指明具体信息。库只负责各层信息的构造与语义，链式跳转由超索引 + 意向性对象通用指令完成。
    - 三类的共同边界：库代码只开发「信息如何构造、如何使用」；槽位预留、条目封装、质性/质料存取与超索引检索都是既有机制，**不得重复发明**第三套存储/协作方式。

## 3. 任务拆分：如何从需求确定基本指令

接到用户需求（如「计算 1+1」「把两个数装进序偶」「遍历序列」）时，按以下顺序拆解：

**第 1 步：切成原子动作。** 把需求改写成动词列表：构造 → 赋值/计算 → 传递/存取 → 输出 → 释放。脚本节点 = 一个原子动作，节点之间只留真正的数据依赖。

**第 2 步：为每个动作确定数据类型 → 找到集合。** 用 Grep 在 `ESM_Base/`（及各模块索引 md）检索类型关键词（UInt、OrderedPair、Bool、Char、ElementOrder……），确认该类型属于哪个 Set，以及是否有解析宏（`GetUIntElement`、`GetEleOrder` 等）。

**第 3 步：检索现有映射，优先复用。** 在各模块索引 md 中按动词+类型检索（New/Release/Get/Put/Copy/Zero/Swap/Success/Add……），逐个打开映射 md 核对**定义域与值域**是否匹配你的参数。匹配即复用。

**第 4 步：缺口设计新映射。** 现有映射不满足时，按模板设计（先落 Set 再落 Map，遵循 ESM 设计模式）：
   - 定义域集合、值域集合（先确认已有 Set，没有就先建 Set + TestFunction）；
   - `MapFunction` 函数体语义（含失败返回值）；
   - 是否会产生/解除引用、属于简单还是复杂结构、释放方案；
   - 复合对象的**内部布局决策**（对应原则 8）：内核运行中需要拆开操作其内部信息吗？需要 → 布局为元素序列/元素树，定义槽位下标含义与嵌套层次，使通用解析宏即可完成操作；不需要 → 整体封装 + 专用映射；
   - 是否需要新异常码（查 `Exception/ExceptionCode/`，无则按异常文档标准新增）；
   - 是否需要把信息记录到文件/系统中？需要 → 不自造存储机制，按原则 9 用意向性对象（指明质性 → Put 存入质料 / Read·Get 读出），库只实现信息的构造与使用；
   - 元素间关系按**原则 10** 三选一：需协作运行 → 预留槽位 + Set/Put 绑定、Get 解绑（或仅传引用）；UInt/Stream/Bool 等简单字段登记 → 构造进字典条目随条目释放；多跳查询且信息相互独立 → 意向性对象链（质性→质料，超索引逐层定位下一层对象）。

**第 5 步：反向校验脚本可表达性。** 新映射必须能写成脚本节点的四段格式（映射名 / 定义域参数 / 值域参数 / 状态）。多入参要么用序偶封装（Put 系列 + 单序偶定义域），要么用 `()` 多参语法——二选一，由脚本层需求决定。

**第 6 步：落文档。** 每个新增的 Element/Set/Map/宏/异常码**必须**建独立 md，并更新所在模块索引与项目索引（写法遵循「文档编写标准」skill：`md-documentation-standard`）。

## 4. 新映射交付清单

- [ ] 定义域/值域集合存在且有 TestFunction，定义行与文档一致
- [ ] 属于 ESM 三元结构：有对应 Set 判定、有 `Map Xxx` 定义行、复合结构为元素序列/树而非黑盒
- [ ] C++ 侧同时提供 `Map Xxx` 符号与 `XxxFunction`
- [ ] 构造/释放配对；复杂结构有专用释放映射
- [ ] 复合对象已判定布局：需内核内操作内部信息的，按元素序列/元素树设计且槽位布局已写入文档，序列式对象释放覆盖所有槽位
- [ ] 引用路径有 `ZeroElement` 解除说明
- [ ] 落盘/登记类信息走意向性对象（质性+质料），未自造文件读写或全局注册表；Read 引用失效与 Get 转交所有权的纪律已写入文档
- [ ] 元素间关系已按原则 10 判定归属（绑定/字典记录/意向性对象链），绑定有 Set/Put 与 Get 配对或引用解除说明，字典条目释放连带记录元素，意向性对象链的各层质性语义写入文档
- [ ] 有可运行的案例代码 + 逐句解释（含等价关系、风险提示）
- [ ] 已在实现 .cpp 内的 `Code_Test` 命名空间编写 `TotalTest` 总测试映射（正常/异常/清理路径均覆盖）
- [ ] 模块索引 md 已加链接；分类归属正确
- [ ] 脚本侧可表达为四段式节点

## 5. 常见错误

- 直接把 Set/Map 结构体当参数传进内核方法（应封装为元素）
- 用通用 `ReleaseElement` 释放复杂结构（泄漏子内存）
- 引用（`|=`）后不 `ZeroElement`（重复释放）
- 转交目标不是空元素（覆盖既有实体，造成泄漏）
- 映射定义域写成「希望的类型」而非集合实际约束（脚本节点无法通过归属判定）
- 复合对象在内核运行中需被逐字段操作，却封装成黑盒整体元素（脚本层取不到内部信息，只能不断新增专用映射）；反之把不该暴露的内部结构做成元素序列（破坏封装、释放负担转嫁给调用方）
- 新增映射后忘了更新索引 md（二次开发检索不到）
- 需要记录信息到文件时自造存储/注册机制（应复用意向性对象的质性-质料存取与总元素环境检索器的超索引检索）；或对意向性对象 Read 出的引用在 Add/Remove/Put/Release 之后继续使用（引用已失效）

## 6. 测试标准：Code_Test 命名空间与 TotalTest 总测试映射

每个含映射实现的 .cpp 都应附带可运行的自测试。原库体例（见 `E_S_M/h.cpp/ESM_Base_Test.cpp`、`KRS_Base/h.cpp/ZeroKRS.cpp` 等 10 余处）：在 .cpp 的 `namespace Q_A` 内声明 `Code_Test` 命名空间，其中编写**总测试映射**（原库多在对应 .h 中一并导出声明；仅本文件使用时可只在 .cpp 内声明，不污染头文件）：

```C++
namespace Q_A {
    namespace Code_Test {
        bool XxxTotalTestFunction(Element& Domain, Element& Range);
        Map XxxTotalTest(&NullElementSet, &NullElementSet, XxxTotalTestFunction);
    }
}
```

**TotalTest 的测试思路**（直调各 `XxxFunction`，不经 Map 对象；定义域/值域固定用 `NullElementSet` 占位）：

1. **BoolState 累计判定**：`bool BoolState = true;`；每个用例后 `if (不满足预期) { BoolState = false; }`，顺序执行到底再 `return BoolState;`——一次运行即暴露全部断言点，不在首个失败处中断。
2. **正常路径**：New → 集合归属判定（`XxxSetTest`）+ 参数默认值断言 → 写入类映射（Login/Put/Set/Copy）→ 读回校验（Get/Read，或 `GetEleOrder` 直取 + `ElementEqualFunction`）→ 排序/整理类映射后断言**不变量**（升序用 `SuperIndexLessFunction` 相邻比对、End 计数、空位分布）。
3. **异常路径用 try/catch**：对「重复登记」「未登记登出」等抛 Element 异常的映射，写成 `try { XxxFunction(...); BoolState = false; /* 未抛出即失败 */ } catch (Element&) { }`，并复核异常前后状态未被破坏。
4. **序列化往返**：ToData → 断言流首部计数 uint → 新建空对象 → DataTo → 逐项 `ElementEqualFunction` 比对 + 集合归属复验。
5. **清理路径最后执行**：Logout/Defragment/Release 放在末尾；Release 之后再做集合判定（应为假）；临时元素依赖析构或显式 `ReleaseElement` 收尾，不留悬垂引用。
6. **用例顺序即状态机**：按「构造 → 登录 → 异常 → 登出 → 复登 → 多键排序 → 碎片整理 → 序列化往返 → 释放」推进，每步在注释中标注期望布局（如 `[空,10,20,30]`），使布局演进可精确断言；测试内部可用 `static` 辅助函数封装结构寻址（如按逻辑下标取登记项），不导出到头文件。
