/*
vid_common.c - common vid component
Copyright (C) 2018 a1batross, Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include "common.h"
#include "client.h"
#include "mod_local.h"
#include "input.h"
#include "vid_common.h"
#include "platform/platform.h"

// 强制分辨率宏定义
#define FORCED_WIDTH  680
#define FORCED_HEIGHT 481

static CVAR_DEFINE_AUTO( vid_mode, "0", FCVAR_RENDERINFO | FCVAR_READ_ONLY, "current video mode index (used only for storage)" );
static CVAR_DEFINE_AUTO( vid_rotate, "0", FCVAR_RENDERINFO | FCVAR_VIDRESTART | FCVAR_READ_ONLY, "screen rotation (0-3)" );
static CVAR_DEFINE_AUTO( vid_scale, "1.0", FCVAR_RENDERINFO | FCVAR_VIDRESTART | FCVAR_READ_ONLY, "pixel scale" );

CVAR_DEFINE_AUTO( vid_maximized, "0", FCVAR_RENDERINFO | FCVAR_READ_ONLY, "window maximized state, read-only" );
CVAR_DEFINE( vid_fullscreen, "fullscreen", "0", FCVAR_RENDERINFO | FCVAR_VIDRESTART, "fullscreen state (0 windowed, 1 fullscreen, 2 borderless)" );
CVAR_DEFINE( window_width, "width", "680", FCVAR_RENDERINFO | FCVAR_VIDRESTART | FCVAR_READ_ONLY, "screen width" );
CVAR_DEFINE( window_height, "height", "481", FCVAR_RENDERINFO | FCVAR_VIDRESTART | FCVAR_READ_ONLY, "screen height" );
CVAR_DEFINE( window_xpos, "_window_xpos", "0", FCVAR_RENDERINFO | FCVAR_READ_ONLY, "window position by horizontal" );
CVAR_DEFINE( window_ypos, "_window_ypos", "0", FCVAR_RENDERINFO | FCVAR_READ_ONLY, "window position by vertical" );
CVAR_DEFINE( vid_width, "vid_width", "680", FCVAR_READ_ONLY, "actual window viewport size" );
CVAR_DEFINE( vid_height, "vid_height", "481", FCVAR_READ_ONLY, "actual window viewport size" );

glwstate_t	glw_state;

void R_SaveVideoMode( int w, int h, int render_w, int render_h, qboolean maximized )
{
	string temp;

	// 强制覆盖所有传入参数为目标分辨率
	w = FORCED_WIDTH;
	h = FORCED_HEIGHT;
	render_w = FORCED_WIDTH;
	render_h = FORCED_HEIGHT;
	maximized = false; // 禁用最大化，避免分辨率被拉伸

	host.window_center_x = w / 2;
	host.window_center_y = h / 2;

	// 强制写入CVAR，防止被引擎其他模块修改
	Q_snprintf( temp, sizeof( temp ), "%d", w );
	Cvar_DirectSet( &window_width, temp );
	Q_snprintf( temp, sizeof( temp ), "%d", h );
	Cvar_DirectSet( &window_height, temp );
	Q_snprintf( temp, sizeof( temp ), "%d", render_w );
	Cvar_FullSet( "vid_width", temp, vid_width.flags );
	Q_snprintf( temp, sizeof( temp ), "%d", render_h );
	Cvar_FullSet( "vid_height", temp, vid_height.flags  );
	Cvar_DirectSet( &vid_maximized, "0" );
	
	host.renderinfo_changed = false;

	// 强制重置渲染状态，避免缩放偏移
	refState.scale_x = 1.0f;
	refState.scale_y = 1.0f;
	refState.width = render_w;
	refState.height = render_h;
	refState.wideScreen = (render_w * 3 != render_h * 4) && (render_w * 4 != render_h * 5);

	SCR_VidInit();
}

const char *VID_GetModeString( int vid_mode )
{
	vidmode_t *vidmode;
	if( vid_mode < 0 || vid_mode >= R_MaxVideoModes() )
		return NULL;
	if( !( vidmode = R_GetVideoMode( vid_mode ) ) )
		return NULL;
	return vidmode->desc;
}

void VID_CheckChanges( void )
{
	if( FBitSet( cl_allow_levelshots.flags, FCVAR_CHANGED ))
	{
		SCR_RegisterTextures();
		ClearBits( cl_allow_levelshots.flags, FCVAR_CHANGED );
	}

	if( host.renderinfo_changed )
	{
		if( VID_SetMode( ))
		{
			SCR_VidInit();
		}
		else
		{
			Sys_Error( "Can't re-initialize video subsystem\n" );
		}
		host.renderinfo_changed = false;
	}
}

void VID_SetDisplayTransform( int *render_w, int *render_h )
{
	// 强制关闭旋转和缩放，直接赋值目标分辨率
	*render_w = FORCED_WIDTH;
	*render_h = FORCED_HEIGHT;
	ref.rotation = REF_ROTATE_NONE;
}

static void VID_Mode_f( void )
{
	// 强制应用680×481分辨率，忽略控制台参数
	R_ChangeDisplaySettings( FORCED_WIDTH, FORCED_HEIGHT, bound( 0, vid_fullscreen.value, WINDOW_MODE_COUNT - 1 ));
	Con_Printf( "Video mode locked to %dx%d\n", FORCED_WIDTH, FORCED_HEIGHT );
}

void VID_Init( void )
{
	Cvar_RegisterVariable( &window_width );
	Cvar_RegisterVariable( &window_height );
	Cvar_RegisterVariable( &vid_mode );
	Cvar_RegisterVariable( &vid_rotate );
	Cvar_RegisterVariable( &vid_scale );
	Cvar_RegisterVariable( &vid_fullscreen );
	Cvar_RegisterVariable( &vid_maximized );
	Cvar_RegisterVariable( &vid_width );
	Cvar_RegisterVariable( &vid_height );
	Cvar_RegisterVariable( &window_xpos );
	Cvar_RegisterVariable( &window_ypos );

	Cmd_AddRestrictedCommand( "vid_setmode", VID_Mode_f, "display video mode" );

	V_Init();
	R_Init();

	// 初始化时直接强制设置窗口位置和尺寸，避免偏移
	R_ChangeDisplaySettings( FORCED_WIDTH, FORCED_HEIGHT, vid_fullscreen.value );
}
