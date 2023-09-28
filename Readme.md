# （CCL）Cubic Crystal Libraries README

## 简述

$\qquad$ Cubic Crystal Libraries通常被简写作CCL，其中文翻译通常是“立方晶”，如果你愿意叫其他名字也无所谓。它是一个跨平台的软件开发框架，包含一个内核以及众多的扩展模块。

$\qquad$它的第一开发者是中国人，所以说大部分注释都是由中文写成，恕不提供任何外语注释，不过第一开发者并没有使用拼音的习惯，所以说放心，里面所有的函数名、类名、变量名之类都是由有意义的英文词汇构造的。

## 定位

$\qquad$一个开源，高扩展性，自由选择的跨平台（游戏）开发框架。当然，你想要使用它开发实用软件也不在话下
$\qquad$此项目是第一开发者闲暇之余一拍脑袋想出来的点子。因为本人曾经想要写一个游戏却不愿意使用任何现成的游戏引擎，然后饱受复杂零碎的API困扰，更重要的是，高级API很难提供我真正想要的功能，所以说本人就此决定自己来做一个框架。这个框架可能具有成为游戏引擎的潜质。

## 环境

$\qquad$目前支持Window和Linux两大平台。此项目使用纯粹的C和C++混合编写而成，确保你的计算机具有编译C/C++的能力。

**必须具备的运行/编译环境**  
$\qquad$ 0.C标准库  
$\qquad$ 1.OpenGL  
$\qquad$ 2.Xlib（Linux）  
$\qquad$ 3.Visual studio集成环境（Windows编译）  
$\qquad$ 4.C/C++编译器（编译）  
$\qquad$ 5.bash程序（Linux编译）  
$\qquad\qquad$ 注：我认为所有Linux选手都不可能缺少bash这玩意  
$\qquad$ 6.cmake程序（编译）  
$\qquad$ 7.make程序（Linux编译）

**编译方法：**

$\qquad$ Windows上使用Visual Studio进行cmake编译，此IDE对cmake项目的支持很好。  
$\qquad$ Linux上确保你有bash执行器，然后进入项目根目录，执行：

~~~bash
bash ./AutoBuild.sh
~~~

$\qquad$然后你就能在

~~~bash
./build/bin/
~~~

里面找到二进制文件了

$\qquad$假如它提示你缺少 **cmake** 或者 **make** 或者**编译环境**等，请自行查阅相关软件的安装方法，此类问题通常可以在一条指令之内解决。

## 说明

$\qquad$当前此框架已具有基础的消息框架，渲染框架，有自己的一些好用的数据结构，能够在多平台为你带来稳定一致的体验。

$\qquad$详细的使用文档将在开发小有成就之时放到某个自建网站的网页中（现在还没写）。  
作者在此给予此库的简要使用方法和结构说明以及内含的思想：  
$\qquad$用户在使用时只需要将一个极小的静态库连接到自己的可执行文件中，这个静态库里面包含了此项目的思想精华：加载扩展组件。实际上，加载扩展组件是这个静态库唯一能够做到的事情，所有的有用功能都被打包成动态库文件等待加载。好处是你真的可以随意选择想要的组件，即使是缺失了这些组件程序也不会崩溃，只会什么都不做，并告诉你它找不到组件（笑。  
$\qquad$赞美模块化编程！把各种功能分开来写，打包成各自的模块，这样用户就能自己组装成刚刚好够用的成品了。如果你愿意，可以直接设计自己的模块，只要遵守CCL中十分简洁的模块调用协议，就能立刻将其并入CCL的模块生态（喜），无论自用还是发布出来造福他人都是极好的。  
$\qquad$这里要提醒一下，所有的“官方”的模块都是不提供编译时期的链接方式的，你只能使用CCL核心加载器来加载其中提供的功能。使用如下代码来加载 **CCLstd** 模块并使用其中的 **ShortSleep** 功能：

~~~C
//请将CCL核心的头文件加入你的编译环境
#include <CCL.h>

/*
* 下面这个头文件其实是一个清单，列出了cclstd加载后
* 所有可以从里面获取的功能。
* 这种清单由模块的作者负责编写。
*/
#include <cclstd.h>

//这个用来保存加载成功的模块，没错，它就是个ID
CCLMODID cclstd;

/*
* 名字可以随便起，但乱起名字的你最好能
* 记住这个功能函数原本的功能
*/
CCLModFunction shortsleep;

int main(int argc, char** argv)
/*argv是必不可少的，
* 你需要提供可执行文件在工作目录中的
* 路径才能确保正确寻址
*/
{
    int ret = 0;
    char* path = nullptr;
    //CCL在使用之前必须初始化，否则罢工！！
    if (!CCLModuleLoaderInit())
    {
        printf("%s\n", CCLCurrentErr_EXModule());
        ret = -1;
        goto End;
    }
    //使用此函数来根据名字自动生成加载路径
    ccl_gluing_path(argv[0], "CCL_std", &path);
    //你可以看看这个路径长啥样
    printf("%s\n", path);
    cclstd = CCLEXLoad(path);
    delete path;
    if (!cclstd)
    {
        printf("error: %s\n", CCLCurrentErr_EXModule());
        ret = 1;
        goto End;
    }

    //把加载器想象成一个服务器，现在你要向其发送请求
    CCL_MOD_REQUEST_STRUCTURE req;

    /*
    * 告知加载器你想要的功能函数的国籍、身份证号，
    * 然后加载器就会过去开门查水表（划掉）
    * 这些东西在对应的清单里面会有
    * 自行查阅 cclstd.h ，里面会有详细注释
    */
    CCLModGetFn(cclstd, CCLSERV_SHORT_SLEEP, &req);
    if (req.func)
        shortsleep = req.func;

    //一秒钟的优质睡眠，CCLstd的休眠以微秒为单位
    if (shortsleep)
        shortsleep((void*)1000);

End:
    printf("return value: %d\n", ret);
    printf("press enter to exit\n");
    getchar();
    return ret;
}
~~~

$\qquad$上述示例阐明了CCL核心加载器的工作流程，下面简要阐述CCL模块的一些特征：  
$\qquad$首先模块都将被打包成完完全全的动态库，在CMakeLists.txt里面是这样描述的：

~~~cmake
add_library("CCL_std" SHARED ${CCL_Std_Files})
~~~

$\qquad$在模块源码中，你必须含有下面这个函数

~~~C
CCLMODAPI const char* CCLEXmain(void** argv, int argc)
~~~

$\qquad$当加载器读取这个模块的库文件时，将首先寻找上述负责初始化的函数，如果没找到，那么加载器将判定这个模块是无效的并返回错误。你可以使用错误查询函数来查看错误。值得注意的是，**函数符号格式必须采用C语言标准** ，否则加载一定会失败，你可以在 **C++** 函数前面加入 **extern "C"** 来解决这个问题。  
$\qquad$当加载器找到这个函数后，会给它传递一些信息，而这个函数将自主完成后续所有的加载工作，详见CCLstd的源码中cclstd.c文件，这份源码很简单。之后此初始化函数将向加载函数返回该模组真实的名字然后结束。假如初始化函数没有返回名称（一个字符串），加载器将判定加载故障，并停止加载给出错误，假如模块中实现了用于释放模块时善后的函数

~~~C
CCLMODAPI void CCLEXexit()
~~~

那么这个函数将在加载失败后给出错误前被执行。这个函数同样要求 **函数符号格式必须采用C语言标准** 。你可以在此函数中给出你的内存和设备资源释放方案。

## 结构

此项目的目录结构为（可以看出有很多东西还没开始写）：
CCL  
└─parts  
$\qquad$├─cclaudio  
$\qquad$├─cclcore  
$\qquad$│$\qquad$├─include  
$\qquad$│$\qquad$└─src  
$\qquad$├─cclcompressor  
$\qquad$│$\qquad$├─include  
$\qquad$│$\qquad$└─src  
$\qquad$├─cclgraphic  
$\qquad$│$\qquad$├─include  
$\qquad$│$\qquad$└─src  
$\qquad$├─cclinet  
$\qquad$│$\qquad$├─include  
$\qquad$│$\qquad$└─src  
$\qquad$├─cclstd  
$\qquad$│$\qquad$├─include  
$\qquad$│$\qquad$└─src  
$\qquad$└─demos  
$\qquad\qquad$├─include  
$\qquad\qquad$│$\qquad$├─demo1  
$\qquad\qquad$│$\qquad$├─demo2  
$\qquad\qquad$│$\qquad$└─demo3  
$\qquad\qquad$└─src  
$\qquad\qquad\qquad$├─demo1  
$\qquad\qquad\qquad$├─demo2  
$\qquad\qquad\qquad$└─demo3

## 一些牢骚（倒叙）

"Cubic Crystal Library"  
“立方晶”  
简记为：CCL  
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-  
和GCL很像但是，GCL是纯粹随机找的字母组合，而这个是单词缩写。  
从意味上讲，这个没有那么抽象。  
源头自然是GCL，但是青出于蓝胜于蓝，更好的迭代也是CCL的目的。  
这是一次对于“引擎”的尝试，不是游戏引擎，而是能在日常编写软件  
中也能方便使用的“助力”。  
//////////  
希望是非常强大的核动力助力引擎(^^  
CCL的很多功能和服务是在GCL中实验过很多遍之后搬过来的，技术上  
相对更成熟一些。  
至少要跨Windows和Linux两大平台吧，是个很大的长期工程呢（q_q  
//  
//  
-2023-5-16-Adore

**0.0.6.1-alpha** 加入TCP网络模块（socket）  
网络编程是很复杂的类别，甚至复杂度超过了多线程（因为网络任何时都有可能受到干扰，不如本地环境一般稳定）  
。这种异步不是靠时延就能解决的，必须想尽办法用机制来确保稳定。  
从今天开始，CCL就不再是一个简单的模块群了，它将逐渐具有高级实用价值。
-2023-9-28-Adore

**0.0.6.0-alpha** LZ77算法和动态数组  
动态数组最大简单，但压缩算法真的很变态。。。并不是原理难，而是实现难（工程师的真实瞬间），亚字节操作  
让检查内容十分艰难，只有把压缩和解压全部完成之后才能比较直观地debug，不过历经千辛万苦还是成了  
-2023-9-20-Adore

**0.0.5.3-alpha** Windows改姓mesa了（雾  
Windows中D2D和D3D是真的活少事多，很难和OpenGL统一起来，所以说干脆直接改用OpenGL。  
毕竟之前是有在Linux铺过路的，所以说这次直接改一点点接口就能上阵，状态很完美。  
DirectX在现在处于弃用的状态。  
-2023-9-4-Adore

**v0.0.5.2-alpha** Linux图形框架构建出来了  
这下轮到Windows重写了，direct2D和OpenGL的坐标系很难统一，而且OpenGL到3D可以一步到位，所以说现在还是决定  
在Windows平台使用OpenGL。当然窗口管理系统还是使用已经构建起来的Win32框架。  
-2023-9-3-Adore

**v0.0.5.1-alpha** 窗口管理部分创建（Linux）  
Xlib自带的绘图不是很好用，在Linux上的2D以及3D图像渲染模块计划使用OpenGL一步到位了。  
-2023-8-31-Adore

**v0.0.5.0-alpha** 基础gui管理部分基本完善（仅限Windows）  
增加了更多2D实体的支持，修正了屎山中一些多线程问题，现在运行更加稳定了。  
很遗憾direct2D并不支持带有透明通道的位图绘制，这种功能只有等到加入direct3D之后  
才能支持了，这已经是下下个版本的事情了。目前暂时不拓展图像方面的支持，先加入网络支  
持以及Linux方面的基础功能。  
-2023-8-9-Adore

**v0.0.4.4-alpha** 完善功能（仅限Windows）  
增加了更多2D实体的支持，在core中增加了一些便捷封装。  
下一次版本号变动就是较大变动，STEP变动到5，但仍然是alpha，目前有骨缺肉。  
-2023-8-4-Adore

**v0.0.4.3-alpha** 2D图形实体管理架构完成（仅限Windows）  
中间遇到了各种莫名其妙的问题，吐血ing。。。  
数据结构加入了遍历功能，其中二叉树默认为中序遍历，线性表从前往后依次遍历。  
-2023-8-4-Adore

**v0.0.4.2-alpha** 基本视窗框架完成了（仅限Windows）  
摸鱼了很长时间，现在完成窗口管理的主要结构，还没有添加任何功能，暂时不能实用。  
此结构是为后面的图像绘制打基础的，被一并包含在graphic组件里面。  
-2023-8-2-Adore

**v0.0.4.1-alpha** Linux也支持了  
并没有更改任何功能结构，只是拓展了平台支持，实际上到这一步才算勉强达到GCL的水准了。  
要记住Autobuild.sh绝对不能使用带标志UTF8（UTF8 with BOM），否则上面的标志行会导致脚本无法执行。  
-2023-6-22-Adore

**v0.0.4.0-alpha** 添加多线程支持  
实际上是将GCL内化的std部分搬到外部模块里面去了。  
-2023-6-22-Adore

**v0.0.3.3-alpha** 版本号继承自GCL的末期版本  
CCL的组织方式发生了变化，所有的可选功能全部放到外部模块里面，需要用户自己选择加载。  
核心部分尽可能的小（只要确保有完全加载模块的能力即可），作者并不喜欢一个“大而丑”的东西。  
///////////  
这种方法有一些好处，封装好的文件可以立刻通用（C的符号格式），无需再次编译链接，热加载，  
缺失非关程序键模块时不会自动崩溃，在用户愿意的情况下可以继续运行。  
-2023-6-21-Adore
