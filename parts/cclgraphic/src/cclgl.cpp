#include "cclgl.hpp"
#include <math.h>

//一些常数
static const CCLUINT32 ratio = 4;
static const CCLUINT32 n1 = 80;
static const CCLUINT32 n2 = 8;
static const CCLUINT32 nc = 3;

void DrawRect(float x1, float y1, float x2, float y2, float level, float stroke, ccl_colorf* pColor)
{
    float tmp;
    if (stroke < 0)
        stroke = -stroke;
    if (x1 > x2)
    {
        tmp = x2;
        x2 = x1;
        x1 = tmp;
    }
    if (y1 > y2)
    {
        tmp = y2;
        y2 = y1;
        y1 = tmp;
    }
    level = -level;
    stroke /= 2 * ratio;

    glColor4f(pColor->r, pColor->g, pColor->b, pColor->a);
    glBegin(GL_QUADS);
    glVertex3f(x1 + stroke, y1 + stroke, level);
    glVertex3f(x1 - stroke, y1 - stroke, level);
    glVertex3f(x1 - stroke, y2 + stroke, level);
    glVertex3f(x1 + stroke, y2 - stroke, level);

    glVertex3f(x1 + stroke, y1 + stroke, level);
    glVertex3f(x1 - stroke, y1 - stroke, level);
    glVertex3f(x2 + stroke, y1 - stroke, level);
    glVertex3f(x2 - stroke, y1 + stroke, level);

    glVertex3f(x2 + stroke, y2 + stroke, level);
    glVertex3f(x2 - stroke, y2 - stroke, level);
    glVertex3f(x1 + stroke, y2 - stroke, level);
    glVertex3f(x1 - stroke, y2 + stroke, level);

    glVertex3f(x2 + stroke, y2 + stroke, level);
    glVertex3f(x2 - stroke, y2 - stroke, level);
    glVertex3f(x2 - stroke, y1 + stroke, level);
    glVertex3f(x2 + stroke, y1 - stroke, level);
    glEnd();
}

void DrawElipse(float x, float y, float rx, float ry, float level, float stroke, ccl_colorf* pColor)
{
    if (rx < 0)
        rx = -rx;
    if (ry < 0)
        ry = -ry;
    if (stroke < 0)
        stroke = -stroke;
    level = -level;
    int n = rx * nc + ry * nc;
    if (n > n1)
        n = n1;
    else if (n < n2)
        n = n2;

    glColor4f(pColor->r, pColor->g, pColor->b, pColor->a);
    glBegin(GL_QUADS);
    float lr_x = rx + stroke / ratio, sr_x = rx - stroke / ratio;
    float lr_y = ry + stroke / ratio, sr_y = ry - stroke / ratio;
    float tx_1 = x, tx_2 = x;
    float ty_1 = lr_y + y, ty_2 = sr_y + y;
    for (int i = 0; i <= n; i++)
    {
        glVertex3f(tx_1, ty_1, level);
        glVertex3f(tx_2, ty_2, level);
        tx_1 = sin(i * M_PI * 2 / n) * sr_x + x;
        tx_2 = sin(i * M_PI * 2 / n) * lr_x + x;
        ty_1 = cos(i * M_PI * 2 / n) * sr_y + ry;
        ty_2 = cos(i * M_PI * 2 / n) * lr_y + ry;
        glVertex3f(tx_2, ty_2, level);
        glVertex3f(tx_1, ty_1, level);
    }
    glEnd();
}

void FillRect(float x1, float y1, float x2, float y2, float level, ccl_colorf* pColor)
{
    level = -level;
    glColor4f(pColor->r, pColor->g, pColor->b, pColor->a);
    glBegin(GL_QUADS);
    glVertex3f(x1, y1, level);
    glVertex3f(x1, y2, level);
    glVertex3f(x2, y2, level);
    glVertex3f(x2, y1, level);
    glEnd();
}

void FillElipse(float x, float y, float rx, float ry, float level, ccl_colorf* pColor)
{
    if (rx < 0)
        rx = -rx;
    if (ry < 0)
        ry = -ry;
    level = -level;
    int n = rx * nc + ry * nc;
    if (n > n1)
        n = n1;
    else if (n < n2)
        n = n2;

    glColor4f(pColor->r, pColor->g, pColor->b, pColor->a);
    glBegin(GL_POLYGON);
    for (int i = 0; i < n; i++)
    {
        glVertex3f(sin(i * M_PI * 2 / n) * rx + x, cos(i * M_PI * 2 / n) * ry + y, level);
    }
    glEnd();
}

void DrawPoint(float x, float y, float level, float stroke, ccl_colorf* pColor)
{
    if (stroke < 0)
        stroke = -stroke;
    FillElipse(x, y, stroke / nc, stroke / nc, level, pColor);
}

void DrawLine(float x1, float y1, float x2, float y2, float level, float stroke, ccl_colorf* pColor)
{
    if (stroke < 0)
        stroke = -stroke;
    level = -level;
    stroke /= 2 * ratio;
    float r = -(M_PI_2 - atan((y2 - y1) / (x2 - x1)));
    float dx = cos(r) * stroke, dy = sin(r) * stroke;
    glColor4f(pColor->r, pColor->g, pColor->b, pColor->a);
    glBegin(GL_QUADS);
    glVertex3f(x1 + dx, y1 + dy, level);
    glVertex3f(x1 - dx, y1 - dy, level);
    glVertex3f(x2 - dx, y2 - dy, level);
    glVertex3f(x2 + dx, y2 + dy, level);
    glEnd();
}

void DrawDemo()
{
    ccl_colorf color;
    color.r = 0.3f;
    color.g = 0.5f;
    color.b = 0.2f;
    color.a = 0.5f;
    DrawElipse(20.0f, 30.0f, 30.0f, 20.0f, 1.0f, 3.0f, &color);
    color.r = 0.7f;
    color.g = 0.2f;
    color.b = 0.5f;
    color.a = 0.2;
    FillElipse(0.0f, 0.0f, 50.0f, 50.0f, 3.0f, &color);
    color.r = 0.5f;
    color.g = 0.9f;
    color.b = 1.0f;
    color.a = 0.3;
    DrawPoint(40.0f, 30.0f, 1.0f, 3.0f, &color);
    DrawRect(30.0f, -10.0f, 20.0f, 50.0f, 3.0f, 5.0f, &color);
    color.r = 1.0f;
    color.g = 1.0f;
    color.b = 1.0f;
    color.a = 0.4;
    FillRect(40.0f, -20.0f, 10.0f, 30.0f, 0.0f, &color);
    color.r = 0.2f;
    color.g = 0.4f;
    DrawLine(-10.0, -70.0, 50.0, 80.0, -1.0, 8, &color);
}

void copy_ccl2d_item(PCCL2DITEM pItem, PCCL_2D_ITEM_INNER pInner)
{
	pInner->old.ps.x = pItem->ps.x;
	pInner->old.ps.y = pItem->ps.y;
	pInner->old.ps.w = pItem->ps.w;
	pInner->old.ps.h = pItem->ps.h;
	pInner->old.color.r = pItem->color.r;
	pInner->old.color.g = pItem->color.g;
	pInner->old.color.b = pItem->color.b;
	pInner->old.color.a = pItem->color.a;
	pInner->old.range.x1 = pItem->range.x1;
	pInner->old.range.x2 = pItem->range.x2;
	pInner->old.range.y1 = pItem->range.y1;
	pInner->old.range.y2 = pItem->range.y2;
	pInner->old.rad = pItem->rad;
	pInner->old.stroke = pItem->stroke;
}

void InitGL()
{
    glShadeModel(GL_SMOOTH);
    //需要透明度的时候不要启用depth，否则某些情况下画面会被裁掉
    //不过如果按照从后往前的顺序来，就不会有问题
    glDisable(GL_DEPTH_TEST);
    //glDisable(GL_DEPTH);
    //启用透明度通道
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POLYGON_SMOOTH);

    //清屏黑
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

void clear_2(void* data)
{
	CCL_2D_ITEM_INNER*  pInner = (CCL_2D_ITEM_INNER*)data;
	delete (CCL_2D_ITEM_INNER*)data;
}

void clear_1(void* data)
{
	CCLDestroyDataStructure(&data, clear_2);
}

#ifdef CCL_LINUX

//查询线宽范围（PS:虚拟机里面只支持1-1的线宽）
//static float range[2];

ccl_gl::ccl_gl(Display *pDisplay, XVisualInfo *vi, Window win)
{
    dpy = pDisplay;
    w = win;
    context = glXCreateContext(dpy, vi, nullptr, GL_TRUE);
    glXMakeCurrent(dpy, w, context);
    /*
     * 创建环境完毕
     */
    InitGL();

    //查询
    //glGetFloatv(GL_LINE_WIDTH_RANGE, range);

    _tree = CCLCreateTreeMap();
    available = CCLCreateLinearTable();
    toremove = CCLCreateLinearTable();
}

ccl_gl::~ccl_gl()
{
    CCLDestroyDataStructure(&_tree, clear_1);
	CCLDestroyDataStructure(&available, nullptr);
	CCLDestroyDataStructure(&toremove, nullptr);
    glXMakeCurrent(dpy, None, NULL);
    glXDestroyContext(dpy, context);
}

void ccl_gl::PaintAll()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CCLDataStructureEnum(_tree, ccl_gl::PaintItems, this);
    while (CCLDataStructureSize(toremove))
    {
        CCLDataStructure tmp = CCLTreeMapGet(_tree, (CCLINT64)CCLLinearTableOut(toremove));
        CCLDestroyDataStructure(&tmp, nullptr);
    }
    //DrawDemo();
    glXSwapBuffers(dpy, w);
}

#elif defined CCL_WINDOWS

//像素格式
static PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        16,
        0,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
};
/*
* 
*/

void SetupPixelFormat(HDC hDc)
{
    int nPixelFormat;
    nPixelFormat = ChoosePixelFormat(hDc, &pfd);
    if (!SetPixelFormat(hDc, nPixelFormat, &pfd))
        printf("Failed SetPixelFormat!\n");
}

ccl_gl::ccl_gl(HDC hDc)
{
    _hDc = hDc;
    SetupPixelFormat(_hDc);
    _hRc = wglCreateContext(_hDc);
    wglMakeCurrent(_hDc, _hRc);
    /*
     * 创建环境完毕
     */
    InitGL();

    //查询
    //glGetFloatv(GL_LINE_WIDTH_RANGE, range);

    _tree = CCLCreateTreeMap();
    available = CCLCreateLinearTable();
    toremove = CCLCreateLinearTable();
}

ccl_gl::~ccl_gl()
{
    CCLDestroyDataStructure(&_tree, clear_1);
    CCLDestroyDataStructure(&available, nullptr);
    CCLDestroyDataStructure(&toremove, nullptr);
    wglMakeCurrent(_hDc, NULL);
    wglDeleteContext(_hRc);
}

void ccl_gl::PaintAll()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CCLDataStructureEnum(_tree, ccl_gl::PaintItems, this);
    while (CCLDataStructureSize(toremove))
    {
        CCLDataStructure tmp = CCLTreeMapGet(_tree, (CCLINT64)CCLLinearTableOut(toremove));
        CCLDestroyDataStructure(&tmp, nullptr);
    }
    //DrawDemo();
    SwapBuffers(_hDc);
}

#endif

void ccl_gl::AddItem(PCCL2DITEM pItem)
{
    CCLDataStructure pTree = (CCLDataStructure*)CCLTreeMapSeek(_tree, pItem->level);
    CCLUINT64 id = (CCLUINT64)CCLLinearTableOut(available);
    PCCL_2D_ITEM_INNER pInner = new CCL_2D_ITEM_INNER;
    copy_ccl2d_item(pItem, pInner);
    pInner->pItem = pItem;
    pInner->old.type = pItem->type;
    pInner->old.data = pItem->data;
    pInner->old.level = pItem->level;
    pItem->change = CCLFALSE;
    pItem->remove = CCLFALSE;
    if (!id)
    {
        id = CurrentID;
        CurrentID++;
    }
    if (!pTree)
    {
        pTree = CCLCreateTreeMap();
        CCLTreeMapPut(_tree, pTree, pItem->level);
    }
    CCLTreeMapPut(pTree, pInner, id);
}

void ccl_gl::Resize(CCLUINT32 x, CCLUINT32 y)
{
    glViewport(0, 0, x, y);
    glLoadIdentity();
    if (x > y)
    {
        glOrtho(-100.0f * x / y, 100.0f * x / y, -100.0f, 100.0f, 10.0f, -1000.0f);
    }
    else
    {
        glOrtho(-100.0f, 100.0f, -100.0f * y / x, 100.0f * y / x, 10.0f, -1000.0f);
    }
}

void paint(void* data, void* user, CCLUINT64 idThis)
{
    PCCL_2D_ITEM_INNER pInner = (PCCL_2D_ITEM_INNER)data;
    ccl_gl* pThis = (ccl_gl*)user;
    if (pInner->pItem->remove)
    {
        CCLLinearTableIn(pThis->toremove, (void*)idThis);
        pInner->pItem->remove = CCLFALSE;
        return;
    }
    if (pInner->pItem->change)
    {
        copy_ccl2d_item(pInner->pItem, pInner);
        pInner->pItem->change = CCLFALSE;
    }
    if (pInner->pItem->lock)
        return;
    if ((pInner->old.type & 0x0f) == CCL_2D_LINE)
    {
        DrawLine(
            pInner->old.pp.x1, pInner->old.pp.y1,
            pInner->old.pp.x2, pInner->old.pp.y2,
            0.0f, pInner->old.stroke, &(pInner->old.color)
        );
    }
    else if (pInner->old.type & CCL_2D_BITMAP)
    {
    }
    else if (pInner->old.type & CCL_2D_FILLED)
    {
        switch (pInner->old.type & 0x0f)
        {
        case CCL_2D_RECT:
            FillRect(
                pInner->old.pp.x1, pInner->old.pp.y1,
                pInner->old.pp.x2, pInner->old.pp.y2,
                0.0f, &(pInner->old.color)
            );
            break;
        case CCL_2D_ELIPSE:
            FillElipse(
                pInner->old.cr.x_r, pInner->old.cr.y_r,
                pInner->old.cr.r_x, pInner->old.cr.r_y,
                0.0f, &(pInner->old.color)
            );
            break;
        default:
            break;
        }
    }
    else
    {
        switch (pInner->old.type & 0x0f)
        {
        case CCL_2D_RECT:
            DrawRect(
                pInner->old.pp.x1, pInner->old.pp.y1,
                pInner->old.pp.x2, pInner->old.pp.y2,
                0.0f, pInner->old.stroke, &(pInner->old.color)
            );
            break;
        case CCL_2D_ELIPSE:
            DrawElipse(
                pInner->old.cr.x_r, pInner->old.cr.y_r,
                pInner->old.cr.r_x, pInner->old.cr.r_y,
                0.0f, pInner->old.stroke, &(pInner->old.color)
            );
            break;
        default:
            break;
        }
    }
}

void ccl_gl::PaintItems(void* data, void* user, CCLUINT64 idThis)
{
    ccl_gl* pThis = (ccl_gl*)user;
    CCLDataStructureEnum(data, paint, user);
    while (CCLDataStructureSize(pThis->toremove))
    {
        PCCL_2D_ITEM_INNER pInner = (PCCL_2D_ITEM_INNER)CCLTreeMapGet(data, (CCLINT64)CCLLinearTableOut(pThis->toremove));
    }
    if (!CCLDataStructureSize(data))
    {
        CCLLinearTableIn(pThis->toremove, (void*)idThis);
    }
}
