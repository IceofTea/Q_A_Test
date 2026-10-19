# CMDFlushOutput

***

映射 CMDFlushOutput 接受〈空元素（NullElement）〉为定义域，〈空元素集合（NullElementSet）〉为值域，定义如下：

```C++
Map CMDFlushOutput(&NullElement, &NullElementSet, CMDFlushOutputFunction);
```

输出缓冲落盘（CMDFlushOutput）把 stdout 缓冲内容立即写出——等价代码 flush(stdout)，代码可用 fflush，虚拟机中只能用该映射。

***

## 案例：

```text
CMDFlushOutput
EmptyElement
EmptyElement
TraUntractable
```

与 CMDPutString 配对：Put 只写缓冲，Flush 才保证控制台/文件即时可见。
