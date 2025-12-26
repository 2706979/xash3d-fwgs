/*
* Copyright (C) 2011-2025 Xash3D Engine Project
*
* This code is licensed under the GNU GPL v3 license.
*
* vid_common.c - common video subsystem functions, resolution lock & touch fix
*/

#include "common.h"
#include "client.h"
#include "vid.h"
#include "input.h"

// 锁定目标分辨率为 640x480 (4:3 比例)
#define TARGET_WIDTH    640
#define TARGET_HEIGHT   480

// 全局视频参数
viddef_t viddef;
qboolean vid_fullscreen = false;
qboolean vid_waitvsync = false;

// 触摸坐标映射缓存
static float touch_scale_x = 1.0f;
static float touch_scale_y = 1.0f;

/*
=============
VID_GetModeInfo
=============
*/
qboolean VID_GetModeInfo( int *width, int *height, int mode )
{
    // 强制返回锁定的 640x480 分辨率，忽略模式参数
    *width = TARGET_WIDTH;
    *height = TARGET_HEIGHT;
    return true;
}

/*
=============
VID_CalcTouchScale
计算触摸坐标到游戏分辨率的缩放比例
解决屏幕物理分辨率与游戏逻辑分辨率不匹配问题
=============
*/
static void VID_CalcTouchScale( void )
{
    // 获取安卓设备物理屏幕分辨率
    int physical_w = VID_GetPhysicalWidth();
    int physical_h = VID_GetPhysicalHeight();

    if( physical_w <= 0 || physical_h <= 0 )
    {
        touch_scale_x = 1.0f;
        touch_scale_y = 1.0f;
        return;
    }

    // 计算缩放比例：游戏分辨率 / 物理分辨率
    touch_scale_x = (float)TARGET_WIDTH / physical_w;
    touch_scale_y = (float)TARGET_HEIGHT / physical_h;

    // 4:3 比例适配，避免拉伸导致坐标偏移
    float scale = min( touch_scale_x, touch_scale_y );
    touch_scale_x = scale;
    touch_scale_y = scale;
}

/*
=============
VID_TranslateTouch
将物理触摸坐标转换为游戏逻辑坐标
=============
*/
void VID_TranslateTouch( int *x, int *y )
{
    if( !x || !y ) return;

    // 应用缩放比例，得到游戏内逻辑坐标
    *x = (int)((float)*x * touch_scale_x);
    *y = (int)((float)*y * touch_scale_y);

    // 坐标边界限制，防止超出游戏视口
    *x = clamp( *x, 0, TARGET_WIDTH - 1 );
    *y = clamp( *y, 0, TARGET_HEIGHT - 1 );
}

/*
=============
VID_Init
初始化视频子系统，锁定分辨率并计算触摸缩放
=============
*/
qboolean VID_Init( const char *driver, int mode, qboolean fullscreen )
{
    // 强制使用目标分辨率初始化
    viddef.width = TARGET_WIDTH;
    viddef.height = TARGET_HEIGHT;
    viddef.conwidth = TARGET_WIDTH;
    viddef.conheight = TARGET_HEIGHT / 2;
    viddef.aspect = (float)TARGET_WIDTH / TARGET_HEIGHT; // 固定 4:3 比例
    vid_fullscreen = fullscreen;

    // 初始化视频驱动（保留原有逻辑）
    if( !VID_InitDriver( driver, viddef.width, viddef.height, fullscreen ) )
    {
        Com_Error( ERR_FATAL, "VID_Init: failed to initialize video driver" );
        return false;
    }

    // 计算触摸坐标缩放比例，修复映射问题
    VID_CalcTouchScale();
    Com_Printf( "VID_Init: locked to 640x480 (4:3), touch scale: %.2fx%.2f\n", touch_scale_x, touch_scale_y );

    return true;
}

/*
=============
VID_Shutdown
关闭视频子系统
=============
*/
void VID_Shutdown( void )
{
    VID_ShutdownDriver();
    memset( &viddef, 0, sizeof( viddef ) );
    touch_scale_x = 1.0f;
    touch_scale_y = 1.0f;
}

/*
=============
VID_SetMode
切换视频模式（强制锁定为 640x480）
=============
*/
qboolean VID_SetMode( int mode, qboolean fullscreen )
{
    return VID_Init( viddef.driver, mode, fullscreen );
}

/*
=============
VID_ToggleFullscreen
切换全屏/窗口模式，保持分辨率不变
=============
*/
void VID_ToggleFullscreen( void )
{
    vid_fullscreen = !vid_fullscreen;
    VID_SetMode( 0, vid_fullscreen );
}

/*
=============
VID_GetCurrentMode
返回当前模式（固定为 0，对应 640x480）
=============
*/
int VID_GetCurrentMode( void )
{
    return 0;
}

/*
=============
VID_GetModeCount
返回支持的模式数量（仅 1 种：640x480）
=============
*/
int VID_GetModeCount( void )
{
    return 1;
}
