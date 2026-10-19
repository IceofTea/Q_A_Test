# CMDPutString

***

映射 CMDPutString 接受〈流元素集合（StreamSet）〉为定义域，〈空元素集合（NullElementSet）〉为值域，定义如下：

```C++
Map CMDPutString(&StreamSet, &NullElementSet, CMDPutStringFunction);
```

控制台字符串输出（CMDPutString）把定义域流元素内容原样写入 stdout：不换行、不 flush——等价代码 write(KeySpace,1,Size,stdout)，代码可用 fwrite，虚拟机中只能用该映射。

***

## 案例：

```text
CMDPutString
TextA
EmptyElement
Tractable
```

输出参数 TextA 对应 .pi 中 String A-no-newline；与 CMDFlushOutput 配对使用才保证落盘。风险：定义域须为已构造的流元素，Size=0 或空 KeySpace 时返回 false。
