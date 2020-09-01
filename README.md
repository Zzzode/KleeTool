# KleeTool

该工具主要用于生成可被klee执行分析的llvm ir，依赖于dataflow分析生成的json调用链以及源程序的llvm中间代码。

## Build

1. 在项目根目录创建`build`文件夹

```shell
KleeTool/$ mkdir build
KleeTool/$ cd build
KleeTool/build/$ cmake ..
KleeTool/build/$ make -j4
```

2. ue shell script
```shell script
KleeTool/$ sudo chmod u+x build.sh
KleeTool/$ ./build
```

即可完成源码编译，二进制可执行文件存在`KleeTool/bin/`路径中

## Tutorial

首先需要建立一个数据集文件夹，路径按自己喜好，比如我是放在`～/Dataset/KleeTest`中

```shell
～/Dataset/$ mkdir KleeTest
～/Dataset/$ cd KleeTest
```

接着将源码，llvm中间代码以及json放入同一个文件夹，文件夹名字和源码文件名字保持一致。
比如我有三个文件分别为`test1.c, test1.ll, test1.json`，那么我新建一个文件夹`test1`

```shell
～/Dataset/KleeTest/$ mkdir test1
```

以此类推，将所有的数据集放入KleeTest中，此时KleeTest文件夹下应该有多个文件夹，分别存放不同的源码文件

最后，运行KleeTool工具，参数为存放所有数据集文件夹的路径，例如这里是KleeTest文件夹
```shell
～/Dataset/KleeTest/$ .../KleeTool/bin/KleeTool ～/Dataset/KleeTest/
```

## Others

~~当前工具对EOS平台wasm反编译出来的ll文件有bug，可能无法生成可用的ll文件，bug修复中。~~

bug已修复
