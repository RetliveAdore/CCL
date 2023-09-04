#ifndef _INCLUDE_CCLGRAPHIC_H_
#define _INCLUDE_CCLGRAPHIC_H_

#include <CCL.h>

#define CCLGRAPHIC_MODNAME "CCLGraphic"

//CCL的窗口系统抹平平台差异，窗口是图像的基础
//窗体结构是CCL内部托管的，用户获得的是托管ID
typedef CCLUINT64 CCLWINDOW;
typedef CCLUINT32 CCLWINDOWSTYLE;

//窗口消息传递结构如下，实际上是将不同平台的消息转换成CCL的消息完成统一
typedef struct ccl_msg
{
	CCLUINT64 windowID;
	CCLINT64 x;      //用于指示窗口坐标、鼠标坐标或窗口大小等
	CCLINT64 y;
	CCLINT8 keycode; //ASC字符不需要转换，其余查找宏
	CCLUINT8 status; //见下方宏定义
}CCL_MSG, *PCCL_MSG;
#define CCL_S_UP    0x01
#define CCL_S_DOWN  0x02
#define CCL_S_MOVE  0x04
#define CCL_S_LEFT  0x10
#define CCL_S_RIGHT 0x20
#define CCL_S_OTHER 0xff

/*
* 键盘对应的键码
*/

#ifdef CCL_WINDOWS
#  define CCL_ESC    0x1b
#  define CCL_UP     0x26
#  define CCL_DOWN   0x28
#  define CCL_LEFT   0x25
#  define CCL_RIGHT  0x27
#  define CCL_SPACE  0x20
#  define CCL_TAB    0x09
#  define CCL_DELETE 0x2e
#  define CCL_CTRL   0x11
#  define CCL_SHIFT  0x10
#  define CCL_ALT    0x12
#  define CCL_ENTER  0x0d
#  define CCL_BACK   0x08
#elif defined CCL_LINUX
#  define CCL_ESC    0x09
#  define CCL_UP     0x6f
#  define CCL_DOWN   0x74
#  define CCL_LEFT   0x71
#  define CCL_RIGHT  0x72
#  define CCL_SPACE  0x41
#  define CCL_TAB    0x17
#  define CCL_DELETE 0x77
#  define CCL_CTRL   0x25
#  define CCL_SHIFT  0x32
#  define CCL_ALT    0x40
#  define CCL_ENTER  0x24
#  define CCL_BACK   0x16
#endif

//关于窗体的操作使用一个服务入口即可
#define CCLSERV_WINDOW 10

/*
* CCLWINDOW服务的ID
*/

#define CCLWINDOW_CREATE  0x01
#define CCLWINDOW_DESTROY 0x02
#define CCLWINDOW_MOVE    0x03
#define CCLWINDOW_TITLE   0x04
//设置渲染帧率上限，CCL的渲染工作由另外一个独立线程来处理
#define CCLWINDOW_FPS     0x05
#define CCLWINDOW_CBK     0x06
#define CCLWINDOW_SIZELMT 0x07
#define CCLWINDOW_ITEM    0x08

//传入此值选择默认的尺寸处理
#define CCL_WINDOWSIZE_DEFAULT 0x80000000

//标准回调函数格式
//如果用户想要打断默认的操作，就返回CCLTRUE
typedef CCLBOOL (*cclWindowCallback)(PCCL_MSG msg);

//各种回调的编号

#define CCL_QUIT_CB   0
#define CCL_PAINT_CB  1
#define CCL_MOVE_CB   2
#define CCL_SIZE_CB   3
#define CCL_KEY_CB    4
#define CCL_MOUSE_CB  5
#define CCL_FOCUS_CB  6
typedef struct ccl_window_req
{
	CCLWINDOW id;
	CCLUINT8 servid;
	CCLUINT8 cbk_id;
	CCLUINT8 style;
	CCLBOOL status;  //用于控制是否开启尺寸限制
	CCLUINT32 fps;
	const char* title;
	CCLUINT64 w, h, x, y;  //当用于限制尺寸时，w、h用作指示最大x、y
	cclWindowCallback cbk;
	void* item;
}CCL_WINDOW_REQ;

//用于决定现在是否可以退出
//使用示例：onquit((void*)&cclbool)
#define CCLSERV_ONQUIT 12

#define CCLSERV_MSGBOX 20

#define CCL_MSGBOXTYPE_OK        0x00
#define CCL_MSGBOXTYPE_OKCANCEL  0x01
#define CCL_MSGBOXTYPE_YNCANCEL  0x02

#define CCL_MSGBOX_STAT_OK     0x01
#define CCL_MSGBOX_STAT_CANCEL 0x02
#define CCL_MSGBOX_STAT_YES    0x03
#define CCL_MSGBOX_STAT_NO     0x04
typedef struct ccl_msgbox_req
{
	const char* title;
	const char* text;
	CCLUINT8 type;
	CCLUINT8 status; //执行后返回用户的选择
}CCL_MSGBOX_REQ;

//可以向窗口对象中增减图形/图像实体
//或者进行其他服务等

struct ccl_point_size
{
	float x, y;
	float w, h;
};
struct ccl_point_point
{
	float x1, y1;
	float x2, y2;
};
struct ccl_range
{
	CCLUINT32 x1, y1;
	CCLUINT32 x2, y2;
};
struct ccl_center_r
{
	float x_r, y_r;
	float r_x, r_y;
};
struct ccl_colorf
{
	float r, g, b, a;
};

typedef struct ccl_2d_item
{
	/*
	* 创建成功之后window的修改不会生效
	* 创建成功之后type的修改不会生效
	* 创建成功之后level的修改不会生效
	* 创建成功之后data的修改不会生效
	*/
	//
	CCLWINDOW window;
	CCLUINT8 type;
	CCLINT64 level;  //层级可理解为图层，越小越靠下
	CCLBOOL change;  //会更新实体(CCLTRUE)
	//更新后自动重置为CCLFALSE

	CCLBOOL lock;  //加锁后不渲染此实体

	//设置为CCLTRUE告诉组件移除这个实体
	CCLBOOL remove;  //当变为CCLFALSE时移除完成

	//如果是简单图形，那就无需使用
	//如果是位图之类的，就把数据的头指针放在这里
	void* data;
	struct ccl_range range;  //用于指明要绘制的位图的区域
	                               //或者用于在创建时指明位图的大小

	union  //控制图形实体的位置和大小
	{
		struct ccl_point_size ps;
		struct ccl_point_point pp;
		struct ccl_center_r cr;
	};
	struct ccl_colorf color;
	float rad;  //旋转角度（逆时针）
	float stroke;
}CCL2D_ITEM, *PCCL2DITEM;
#define CCL_2D_LINE   (CCLUINT8)0x01
#define CCL_2D_RECT   (CCLUINT8)0x02
#define CCL_2D_ELIPSE (CCLUINT8)0x03
#define CCL_2D_BITMAP (CCLUINT8)0x04

#define CCL_2D_FILLED (CCLUINT8)0x10

#endif
