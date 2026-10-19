# CMDSleep

***

映射 CMDSleep 接受〈无符号整型集合（UIntSet）〉为定义域，〈空元素集合（NullElementSet）〉为值域，定义如下：

```C++
Map CMDSleep(&UIntSet, &NullElementSet, CMDSleepFunction);
```

毫秒级阻塞等待（CMDSleep）按定义域 UInt 毫秒数挂起当前轮询——等价代码 Sleep(ms)。

***

## 案例：

```text
CMDSleep
UInt3000A
EmptyElement
TraUntractable
```

等待 3000ms，.pi 登记 UInt3000A UInt 3000 Unlimited Common。风险：阻塞轮询线程，不宜在长链高频使用。
