---
name: md-doc-search
description: 'Use when searching for documentation/markdown files (映射文档, md 文档, API 说明, 术语含义, 库用法) that may not exist in the current project. First search the document library at Q_A_Test-master\Doc\MD (including its subfolders); if no near-matching document is found there, ask the user instead of guessing. Triggers: 查文档, 找文档, 文档在哪, md 文档, 文档库.'
---

# md 文档检索标准

当任务涉及查找映射/集合/函数/术语的说明文档，且当前项目目录内没有（或不全）时，按下述顺序检索，不要凭记忆猜测含义。

## 固定文档库

**`Q_A_Test-master\Doc\MD`**（本工作区内，随 git 仓库同步，800+ 篇）

这是优先检索的文档库根目录，其下还有子目录（如 `Lib_Introduce`、`TextualMaterial`）和入口索引 `MainMarkDown.md`。

> 备注：原始 skill 写死的 `D:\AGI\Q_A\AI0001\Q_A\Doc\MD` 是另一台机器上的路径；本机文档库已内置在 `Q_A_Test-master` 仓库内，优先用它。若工作区搬到那台机器，两个路径都试一下。

## 检索流程

1. **先看项目内**：当前工作目录及其文档目录中是否已有对应 md（例如 `*.md` 映射文档、语法手册）。
2. **再查固定文档库**：
   - 先读入口索引 `Q_A_Test-master\Doc\MD\MainMarkDown.md`，从目录链接定位相关文档；
   - 用 Grep/Glob 在该目录下（含子目录）检索关键词（中英文都试：实体名、动词、术语、缩写）；
   - 命中后打开**最接近**的文档，确认其定义域/值域/语义与当前任务匹配。
3. **未命中则询问用户**：若该目录下也找不到近似的文档（注意"近似"——名字不完全一致但讲同一主题的也算命中），**必须向用户提问**，让用户确认术语/实体的含义或指明文档位置，**禁止凭猜测直接编写代码或解释**。

## 规则

- 检索范围仅限上述固定路径与当前项目，不要去检索其他无关目录。
- 命中文档后，引用其定义行/语义来编写代码，并在回复中注明依据的文档名。
- 未命中且用户给出新含义后，若该含义值得沉淀，可建议用户将其补充为该文档库中的 md 文档。
