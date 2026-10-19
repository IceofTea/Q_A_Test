// qacmd_plugin.cpp —— 内核直接插件示例：CMD 输出/等待/缓冲映射
// 契约见 app\docs\内核直接插件标准.md；映射纪律见 map-library-development-standard skill
// 编译：cl /LD /EHsc qacmd_plugin.cpp /Fe:qacmd_plugin.dll
#include <cstdio>
#include <windows.h>

// ---------- 与内核同构的最小 ESM 头（实际链接时用内核 Lib.h.cpp 的类型） ----------
struct Element { unsigned int Size; char* KeySpace; };
struct Set;
typedef bool (*SetTestFunction)(Element& Ele, Set& S);
typedef bool (*MapFunction)(Element& Domain, Element& Range);
struct Set { Element SetParameter; SetTestFunction TestFunction; };
struct Map { Set* Domain; Set* Range; MapFunction MapActive;
    Map(Set* d, Set* r, MapFunction f) : Domain(d), Range(r), MapActive(f) {} };

// ---------- 内核侧既有集合（由符号表按语义名绑定，插件内声明为外部引用） ----------
// NullElementSet / NullElement / UIntSet / StreamSet 由内核提供，插件不重复定义

// ---------- 本插件集合：QACMDSet（标记"本插件构造的元素"用于归属判定） ----------
extern "C" __declspec(dllexport) bool QACMDSetTest(Element& Ele, Set& S) {
    // 简化判定：KeySpace 前 4 字节为魔数 'QCMD'
    return Ele.Size >= 4 && *(unsigned int*)Ele.KeySpace == 0x444D4351u; // 'QCMD'
}
extern "C" __declspec(dllexport) Set QACMDSet = { {0, nullptr}, QACMDSetTest };

// ---------- 映射 1：CMDPutString —— 定义域 StreamSet，值域 NullElement ----------
// 把定义域流元素中的字符串写到 stdout，不换行不 flush（复刻 buf.obj 行为的关键原语）
extern "C" __declspec(dllexport) bool CMDPutStringFunction(Element& Domain, Element& Range) {
    if (!Domain.KeySpace || Domain.Size == 0) return false;
    fwrite(Domain.KeySpace, 1, Domain.Size, stdout); // 无换行、无 flush
    return true;
}

// ---------- 映射 2：CMDSleep —— 定义域 UIntSet(毫秒)，值域 NullElement ----------
extern "C" __declspec(dllexport) bool CMDSleepFunction(Element& Domain, Element& Range) {
    if (Domain.Size < sizeof(unsigned int)) return false;
    Sleep(*(unsigned int*)Domain.KeySpace);
    return true;
}

// ---------- 映射 3：CMDFlushOutput —— 定义域 NullElement，值域 NullElementSet ----------
extern "C" __declspec(dllexport) bool CMDFlushOutputFunction(Element& Domain, Element& Range) {
    return fflush(stdout) == 0;
}

// ---------- 符号表内嵌（插件自描述，内核可导出段或直接读 .sym 文件） ----------
// 行格式：类别 导出名 内核语义名 定义域集合 值域集合 备注
static const char* kSymbolTable =
    "Set     QACMDSet              QACMDSet         -            -              插件元素集合\r\n"
    "Map     CMDPutStringFunction  CMDPutString     StreamSet     NullElementSet 不换行不flush\r\n"
    "Map     CMDSleepFunction      CMDSleep         UIntSet       NullElementSet 毫秒阻塞\r\n"
    "Map     CMDFlushOutputFunction CMDFlushOutput  NullElement   NullElementSet fflush等价\r\n"
    "Element QACMDVersion          QACMDVersionInfo -             -              版本信息\r\n";

extern "C" __declspec(dllexport) const char* QACMDSymbolTable() { return kSymbolTable; }

// ---------- 版本信息元素（符号表 Element 条目指向它） ----------
static char kVersionBlob[] = {'Q','C','M','D', 0,1,0,0}; // 魔数 + v0.1.0.0
extern "C" __declspec(dllexport) Element QACMDVersion = { 8, kVersionBlob };

// ---------- 生命周期 ----------
// Init：登记本插件意向性对象（运行计数器质性→质料）；Shutdown：Release 收尾
extern "C" __declspec(dllexport) bool QACMDPluginInit(void* kernel_host) {
    // kernel_host：内核装载时传入的只读宿主句柄（意向性对象 Add/Put 接口表）
    // 意向性知识库 .kb 由内核在调用本函数前已装载，此处只做运行期挂接
    (void)kernel_host;
    return true;
}
extern "C" __declspec(dllexport) void QACMDPluginShutdown() {
    fflush(stdout); // 兜底：未 flush 的输出在卸载前落盘
}

// ---------- 自测（Code_Test 体例，TotalTest：正常+异常+清理路径） ----------
#ifdef QACMD_SELFTEST
namespace Code_Test {
    bool QACMDTotalTestFunction(Element& D, Element& R) {
        bool ok = true;
        Element s{6, (char*)"hello\n"};
        if (!CMDPutStringFunction(s, R)) ok = false;          // 正常：输出
        Element bad{0, nullptr};
        if (CMDPutStringFunction(bad, R)) ok = false;         // 异常：空域应失败
        Element ms{4, (char*)new unsigned int(10)};
        if (!CMDSleepFunction(ms, R)) ok = false;             // 正常：睡10ms
        if (!CMDFlushOutputFunction(D, R)) ok = false;        // 清理：flush
        delete (unsigned int*)ms.KeySpace;                    // 临时元素收尾
        return ok;
    }
    Map QACMDTotalTest(nullptr, nullptr, QACMDTotalTestFunction);
}
#endif
