---
name: kernel-direct-plugin-development
description: 'Use when designing or implementing kernel direct plugins (直接插件) for Q_A_Test: deciding which 意向性对象 (intentionality objects) to construct and their 意向性行为, designing the DLL public interface surface (Element/Set/Map exports), and generating the 符号表 (symbol table) the kernel uses to import them. Triggers: 直接插件, 内核插件, 符号表, 插件开发, DLL 接口, qaplugin.'
---

# 内核直接插件开发 skill

直接插件三件套契约见 `app\docs\内核直接插件标准.md`，本 skill 教的是**设计决策流程**：拿到插件功能需求后，如何反推出该造什么、暴露什么、登记什么。映射/集合/元素本身的实现纪律看 `map-library-development-standard`，文档落法看 `md-documentation-standard`。

## 决策流程（五步）

### 第 1 步：功能 → 原子映射清单

把插件功能切成原子动作（同 solution-script-writing 第 3 步），每个动作对应一个 `Map`。判定每个映射的定义域/值域集合是否已存在（先查 `Doc/MD` 索引），缺则先设计 Set + TestFunction。CMD 复刻案例：输出字符串→`CMDPutString`、毫秒等待→`CMDSleep`、缓冲落盘→`CMDFlushOutput`。

### 第 2 步：判断意向性对象实例

凡"插件需要记住/登记/落盘的信息"（配置、计数、会话状态、命令表……）一律走**意向性对象**，不自建文件读写或全局表：

1. 列出信息清单 → 每条信息判定其**质性**（信息的意向/标识，须可超索引检索）与**质料**（信息实体）。
2. 需要多次查询且相互独立的信息 → 链式意向性对象（超索引逐层定位，见 `map-library-development-standard` 原则 10）。
3. **意向性行为**：对象登记后能被谁以什么方式消费——`Put`（写质料）/`Read`（读引用，用后 `ZeroElement`）/`Get`（转交所有权）三选一写进设计；`Add` 登记时机（Init 时 or 惰性首次使用）。
4. 产出：`.kb` 字典条目清单 + 对象存储文件内容规划（`FstreamWriteIntentionalityObject` 格式）。

### 第 3 步：设计 DLL 公共接口面

只暴露**脚本/内核真正会用的**最小面：

- 每个原子映射 → `extern "C"` 导出 `XxxFunction`（`bool (Element&, Element&)` 签名）+ 配套 `Map Xxx` 符号。
- 每个新集合 → 导出 `XxxSetTest` + `Set Xxx` 符号。
- 复合对象需脚本层拆内部信息 → 按元素序列/元素树布局（原则 8），槽位表写进文档；否则整体封装。
- 生命周期：`PluginInit`/`PluginShutdown` 两个入口；导出表注明每个元素的释放责任方。
- **不要**导出内部辅助函数；接口面每加一个符号都要能回答"谁在什么场景调用它"。

### 第 4 步：由接口面生成符号表

逐符号填表项：`条目类别(Element/Set/Map) 导出名 内核语义名 定义域集合 值域集合 备注`。

- Map 必须填齐两个集合名；集合必须已被符号表或内核既有库定义。
- 语义名 = `.lan` 节点第一段的检索名，唯一不冲突（查 `server/doc-index.js` 或 `Doc/MD` 索引避重名）。
- 顺序即注册顺序：Set 先于引用它的 Map。

### 第 5 步：验证

- 符号表 vs DLL 导出逐条比对（`dumpbin /exports`）。
- `.lan` 侧写出使用本插件的示例脚本，四段式节点能通过语义名唯一检索到映射。
- 知识库样例文件可被 `FstreamReadIntentionalityObject` 系列格式解读。

## 交付清单

- [ ] 三件套齐全：`.kb/`（字典+对象文件）、`.dll`、`*.sym` 符号表
- [ ] 每个 Map 的定义域/值域集合可追溯；New/Release 配对
- [ ] 意向性对象清单：质性语义、质料布局、意向性行为（Put/Read/Get）已写文档
- [ ] 符号表条目顺序合法（Set 先于 Map）、语义名无冲突
- [ ] 插件每个映射/对象按 md-documentation-standard 落 md 并更新索引
- [ ] 示例 `.lan` 演示插件映射用法

## 常见错误

- 把宿主插件（`qaplugin.hpp` CommandDef 体系）当直接插件：前者经 HTTP 桥给工作区用，后者要进内核映射检索域，三件套缺一不可
- 为插件自建注册表/文件存储：应走意向性对象 + EES 超索引
- 导出符号不带 `extern "C"`：GetProcAddress 找不到修饰名
- 符号表里 Map 先于它引用的 Set：注册时集合尚未定义
- 意向性对象 Read 出的引用在 Put/Release 后继续使用：引用已失效
