#ifndef _INCLUDE_CCLGL_HPP_
#define _INCLUDE_CCLGL_HPP_

#include "cclgraphicinner.h"

typedef struct ccl_2d_item_inner
{
    CCL2D_ITEM* pItem;
    CCL2D_ITEM old;
}CCL_2D_ITEM_INNER, *PCCL_2D_ITEM_INNER;

#ifdef CCL_LINUX
#include <GL/glx.h>
#include <GL/glu.h>

class ccl_gl
{
public:
    ccl_gl(Display* pDisplay, XVisualInfo* vi, Window win);
    ~ccl_gl();
    // 禁止拷贝
    ccl_gl& operator=(const ccl_gl&) = delete;
    ccl_gl(const ccl_gl&) = delete;
public:
    void AddItem(PCCL2DITEM pItem);
    void PaintAll();
    void Resize(CCLUINT32 x, CCLUINT32 y);
private:
    Display* dpy;
    Window w;

    //OpenGL的环境
    GLXContext context;

    CCLUINT32 _w, _h;
    CCLUINT32 CurrentID = 1;
    CCLBOOL _lock;

    //
    CCLDataStructure available;  //queue
    CCLDataStructure toremove;

    //这里会用比较繁杂的数据结构嵌套来存储图像实体的先后关系及层级关系
	//层级数字越小越先绘制，同层级可能会随机绘制
	CCLDataStructure _tree;

    static void PaintItems(void* data, void* user, CCLUINT64 idThis);

	friend void paint(void* data, void* user, CCLUINT64 idThis);
};

#elif defined CCL_WINDOWS //
#include <Windows.h>
#include <gl/GLU.h>
#include <corecrt_math_defines.h>
#pragma comment(lib, "opengl32.lib")

class ccl_gl
{
public:
    ccl_gl(HDC hDc);
    ~ccl_gl();
    // 禁止拷贝
    ccl_gl& operator=(const ccl_gl&) = delete;
    ccl_gl(const ccl_gl&) = delete;
public:
    void AddItem(PCCL2DITEM pItem);
    void PaintAll();
    void Resize(CCLUINT32 x, CCLUINT32 y);
private:
    HDC _hDc;
    //OpenGL的环境
    HGLRC _hRc;

    CCLUINT32 _w, _h;
    CCLUINT32 CurrentID = 1;
    CCLBOOL _lock;

    //
    CCLDataStructure available;  //queue
    CCLDataStructure toremove;

    //这里会用比较繁杂的数据结构嵌套来存储图像实体的先后关系及层级关系
    //层级数字越小越先绘制，同层级可能会随机绘制
    CCLDataStructure _tree;

    static void PaintItems(void* data, void* user, CCLUINT64 idThis);

    friend void paint(void* data, void* user, CCLUINT64 idThis);
};

#endif

#endif