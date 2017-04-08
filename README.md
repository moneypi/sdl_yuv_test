# sdl_yuv_test<br>
最简单的SDL2播放视频的例子（SDL2播放RGB/YUV）<br>
 Simplest Video Play SDL2 (SDL2 play RGB/YUV)<br>
<br>
 雷霄骅 Lei Xiaohua<br>
 leixiaohua1020@126.com<br>
 中国传媒大学/数字电视技术<br>
 Communication University of China / Digital TV Technology<br>
 http://blog.csdn.net/leixiaohua1020<br>
<br>
 本程序使用SDL2播放RGB/YUV视频像素数据。SDL实际上是对底层绘图<br>
 API（Direct3D，OpenGL）的封装，使用起来明显简单于直接调用底层<br>
 API。<br>
<br>
 函数调用步骤如下:<br>
<br>
 [初始化]<br>
 SDL_Init(): 初始化SDL。<br>
 SDL_CreateWindow(): 创建窗口（Window）。<br>
 SDL_CreateRenderer(): 基于窗口创建渲染器（Render）。<br>
 SDL_CreateTexture(): 创建纹理（Texture）。<br>
<br>
 [循环渲染数据]<br>
 SDL_UpdateTexture(): 设置纹理的数据。<br>
 SDL_RenderCopy(): 纹理复制给渲染器。<br>
 SDL_RenderPresent(): 显示。<br>

<br>
 This software plays RGB/YUV raw video data using SDL2.<br>
 SDL is a wrapper of low-level API (Direct3D, OpenGL).<br>
 Use SDL is much easier than directly call these low-level API.<br>
<br>
 The process is shown as follows:<br>
<br>
 [Init]<br>
 SDL_Init(): Init SDL.<br>
 SDL_CreateWindow(): Create a Window.<br>
 SDL_CreateRenderer(): Create a Render.<br>
 SDL_CreateTexture(): Create a Texture.<br>
<br>
 [Loop to Render data]<br>
 SDL_UpdateTexture(): Set Texture's data.<br>
 SDL_RenderCopy(): Copy Texture to Render.<br>
 SDL_RenderPresent(): Show.<br>
<br>
环境搭建：<br>
1. 安装Ubuntu 14.04或以上，我用Ubuntu12.04尝试，默认更新源中不包含sdl2
2. 执行命令 sudo apt-get install libsdl2-dev 安装sdl2依赖库
