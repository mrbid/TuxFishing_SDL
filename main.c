/*
    James William Fletcher ( github.com/mrbid )
        June 2024
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define uint GLuint
#define sint GLint

#ifdef WEB
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

//#define GL_DEBUG
#define MAX_MODELS 60 // hard limit, be aware and increase if needed
#include "inc/esAux7.h"
#include "inc/matvec.h"

#include "inc/res.h"
#include "assets/sky.h"    //0
#include "assets/water.h"  //1
#include "assets/boat.h"   //2
#include "assets/tux.h"    //3
#include "assets/rod.h"    //4
#include "assets/float.h"  //5
#include "assets/splash.h" //6

#include "assets/fish/a0.h"     //7
#include "assets/fish/a1.h"     //8
#include "assets/fish/a2.h"     //9
#include "assets/fish/a3.h"     //10
#include "assets/fish/a4.h"     //11
#include "assets/fish/a5.h"     //12
#include "assets/fish/a6.h"     //13
#include "assets/fish/a7.h"     //14
#include "assets/fish/a8.h"     //15
#include "assets/fish/a9.h"     //16
#include "assets/fish/a10.h"    //17
#include "assets/fish/a11.h"    //18
#include "assets/fish/a12.h"    //19
#include "assets/fish/a13.h"    //20
#include "assets/fish/a14.h"    //21

#include "assets/fish/b0.h"     //22
#include "assets/fish/b1.h"     //23
#include "assets/fish/b2.h"     //24
#include "assets/fish/b3.h"     //25
#include "assets/fish/b4.h"     //26
#include "assets/fish/b5.h"     //27
#include "assets/fish/b6.h"     //28
#include "assets/fish/b7.h"     //29
#include "assets/fish/b8.h"     //30
#include "assets/fish/b9.h"     //31
#include "assets/fish/b10.h"    //32
#include "assets/fish/b11.h"    //33
#include "assets/fish/b12.h"    //34

#include "assets/fish/c0.h"     //35
#include "assets/fish/c1.h"     //36
#include "assets/fish/c2.h"     //37
#include "assets/fish/c3.h"     //38
#include "assets/fish/c4.h"     //39
#include "assets/fish/c5.h"     //40
#include "assets/fish/c6.h"     //41
#include "assets/fish/c7.h"     //42
#include "assets/fish/c8.h"     //43
#include "assets/fish/c9.h"     //44
#include "assets/fish/c10.h"    //45
#include "assets/fish/c11.h"    //46

#include "assets/fish/d0.h"     //47
#include "assets/fish/d1.h"     //48
#include "assets/fish/d2.h"     //49
#include "assets/fish/d3.h"     //50
#include "assets/fish/d4.h"     //51
#include "assets/fish/d5.h"     //52
#include "assets/fish/d6.h"     //53
#include "assets/fish/d7.h"     //54
#include "assets/fish/d8.h"     //55
#include "assets/fish/d9.h"     //56
#include "assets/fish/d10.h"    //57
#include "assets/fish/d11.h"    //58

#include "assets/fish/e1.h"     //59


//*************************************
// globals
//*************************************
const char appTitle[]="Tux Fishing";
SDL_Window* wnd;
SDL_GLContext glc;
SDL_Surface* s_icon = NULL;
uint winw=1024, winh=768;
float t=0.f, dt=0.f, lt=0.f, fc=0.f, lfct=0.f, aspect;

// render state
mat projection, view, model, modelview;

// camera vars
float sens = 0.003f;
float xrot = d2PI;
float yrot = 1.3f;
float zoom = -3.3f;

// game vars
#define FAR_DISTANCE 16.f
uint ks[2]; // is rotate key pressed toggle
uint cast = 0; // is casting toggle
uint caught = 0; // total fish caught
float woff = 0.f; // wave offset
float pr = 0.f; // player rotation (yaw)
float rodr = 0.f; // fishing rod rotation (pitch)
vec fp = (vec){0.f, 0.f, 0.f}; // float position

float frx=0.f, fry=0.f, frr=0.f; // float return direction
int hooked = -1; // is a fish hooked, if so, its the ID of the fish.
float next_wild_fish = 0.f; // time for next wild fish discovery
int last_fish[2]={0};
uint lfi=0;
float winning_fish = 0.f;
uint winning_fish_id = 0;

float shoal_x[3]; // position of shoal
float shoal_y[3]; // position of shoal
uint shoal_lfi[3];// last fish id that jumped
float shoal_nt[3];// next shoal jump time
float shoal_r1[3];// jump rots
float shoal_r2[3];
float shoal_r3[3];

float caught_list[53]={0};


//*************************************
// utility functions
//*************************************
void timestamp(char* ts){const time_t tt=time(0);strftime(ts,16,"%H:%M:%S",localtime(&tt));}
float fTime(){return ((float)SDL_GetTicks())*0.001f;}
SDL_Surface* surfaceFromData(const Uint32* data, Uint32 w, Uint32 h)
{
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    memcpy(s->pixels, data, s->pitch*h);
    return s;
}
void updateModelView()
{
    mMul(&modelview, &model, &view);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (float*)&modelview.m[0][0]);
}
void updateWindowSize(int width, int height)
{
    winw = width, winh = height;
    glViewport(0, 0, winw, winh);
    aspect = (float)winw / (float)winh;
    mIdent(&projection);
    mPerspective(&projection, 30.0f, aspect, 0.01f, FAR_DISTANCE);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (float*)&projection.m[0][0]);
}
#ifdef WEB
EM_BOOL emscripten_resize_event(int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
{
    winw = uiEvent->documentBodyClientWidth;
    winh = uiEvent->documentBodyClientHeight;
    updateWindowSize(winw, winh);
    emscripten_set_canvas_element_size("canvas", winw, winh);
    return EM_FALSE;
}
#endif

//*************************************
// game functions
//*************************************
uint ratioCaught()
{
    uint r = 0;
    for(uint i=0; i<53; i++){if(caught_list[i] == 1){r++;}}
    return r;
}
void rndShoalPos(uint i)
{
    const float ra = esRandFloat(-PI, PI);
    const float rr = esRandFloat(2.3f, 3.6f);
    shoal_x[i] = sinf(ra)*rr;
    shoal_y[i] = cosf(ra)*rr;
    shoal_lfi[i] = (int)roundf(esRandFloat(7.f, 59.f));
    shoal_nt[i] = t + esRandFloat(6.5f, 16.f);
}
void resetGame(uint mode)
{
    cast=0;
    pr=0.f;
    rodr=0.f;
    fp=(vec){0.f, 0.f, 0.f};
    frx=0.f;
    fry=0.f;
    frr=0.f;
    hooked=-1;
    last_fish[0]=-1;
    last_fish[1]=-1;
    lfi=0;
    winning_fish=0.f;
    winning_fish_id=0;
    next_wild_fish=t+esRandFloat(23.f,180.f);
    caught=0;
    //for(uint i=0; i<53; i++){caught_list[i]=0;}
    memset(&caught_list[0], 0x00, sizeof(float)*53);
    rndShoalPos(0);
    rndShoalPos(1);
    rndShoalPos(2);
    if(mode == 1)
    {
        char strts[16];
        timestamp(&strts[0]);
        printf("[%s] Game Reset.\n", strts);
    }
    SDL_SetWindowTitle(wnd, appTitle);
}
float getWaterHeight(float x, float y)
{
    const uint imax = water_numvert*3;
    int ci = -1;
    float cid = 9999.f;
    for(uint i=0; i < imax; i+=3)
    {
        const float xm = water_vertices[i]   - x;
        const float ym = water_vertices[i+1] - y;
        const float nd = xm*xm + ym*ym;
        if(nd < cid)
        {
            ci = i;
            cid = nd;
        }
    }
    if(ci != -1)
    {
        return water_vertices[ci+2];
    }
    return woff;
}

//*************************************
// update & render
//*************************************
void main_loop()
{
//*************************************
// core logic
//*************************************
    fc++;
    t = fTime();
    dt = t-lt;
    lt = t;

    static int mx=0, my=0, lx=0, ly=0, md=0;
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_WINDOWEVENT:
            {
                switch(event.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                    {
                        updateWindowSize(event.window.data1, event.window.data2);
                    }
                    break;
                }
            }
            break;

            case SDL_KEYDOWN:
            {
                if(     event.key.keysym.sym == SDLK_LEFT  || event.key.keysym.sym == SDLK_a) { ks[0] = 1; }
                else if(event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_d) { ks[1] = 1; }
                else if(event.key.keysym.sym == SDLK_SPACE) { cast=1;
                                                              next_wild_fish = t + esRandFloat(23.f, 180.f);}
            }
            break;

            case SDL_KEYUP:
            {
                if(     event.key.keysym.sym == SDLK_LEFT  || event.key.keysym.sym == SDLK_a) { ks[0] = 0; }
                else if(event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_d) { ks[1] = 0; }
                else if(event.key.keysym.sym == SDLK_SPACE)                                   { cast=0;}
                else if(event.key.keysym.sym == SDLK_f)
                {
                    if(t-lfct > 2.0)
                    {
                        char strts[16];
                        timestamp(&strts[0]);
                        printf("[%s] FPS: %g\n", strts, fc/(t-lfct));
                        lfct = t;
                        fc = 0;
                    }
                }
            }
            break;

            case SDL_MOUSEBUTTONDOWN:
            {
                lx = event.button.x, mx = lx;
                ly = event.button.y, my = ly;
                if(event.button.button == SDL_BUTTON_LEFT){md = 1;}
            }
            break;

            case SDL_MOUSEBUTTONUP:
            {
                if(event.button.button == SDL_BUTTON_LEFT){md = 0;}
            }
            break;

            case SDL_MOUSEMOTION:
            {
                if(md > 0){mx = event.motion.x, my = event.motion.y;}
            }
            break;

            case SDL_MOUSEWHEEL:
            {
                if(event.wheel.y < 0){zoom += 0.12f * zoom;}else{zoom -= 0.12f * zoom;}
                if(zoom > -0.73f){zoom = -0.73f;}else if(zoom < -5.f){zoom = -5.f;}
            }
            break;

            case SDL_QUIT:
            {
                SDL_FreeSurface(s_icon);
                SDL_GL_DeleteContext(glc);
                SDL_DestroyWindow(wnd);
                SDL_Quit();
                exit(0);
            }
            break;
        }
    }

//*************************************
// game logic
//*************************************

    // inputs
    if(hooked == -1)
    {
        if(ks[0] == 1){pr -= 1.6f*dt;fp=(vec){0.f, 0.f, 0.f};}
        if(ks[1] == 1){pr += 1.6f*dt;fp=(vec){0.f, 0.f, 0.f};}
        if(cast == 1)
        {
            if(rodr < 2.f){rodr += 1.5f*dt;}
            const float trodr = (rodr+0.23f)*1.65f;
            frx = sinf(pr+d2PI), fry = cosf(pr+d2PI), frr = -d2PI+pr;
            fp.x = frx*trodr, fp.y = fry*trodr;
            fp.z = getWaterHeight(fp.x, fp.y);
        }
        else{if(rodr > 0.f){rodr -= 9.f*dt;}}
    }

    // water offset
    woff = sinf(t*0.42f);

    // camera
    const float dx = (float)(lx-mx);
    const float dy = (float)(ly-my);
    if(dx != 0.f || dy != 0.f)
    {
        xrot += dx*sens;
        yrot += dy*sens;
        if(yrot > 1.5f){yrot = 1.5f;}
        if(yrot < 0.5f){yrot = 0.5f;}
        lx = mx, ly = my;
    }
    mIdent(&view);
    mSetPos(&view, (vec){0.f, -0.13f, zoom});
    mRotate(&view, yrot, 1.f, 0.f, 0.f);
    mRotate(&view, xrot, 0.f, 0.f, 1.f);

//*************************************
// render
//*************************************

    // clear buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ///

    // render sky
    shadeFullbright(&position_id, &projection_id, &modelview_id, &color_id, &lightness_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (float*)&projection.m[0][0]);
    glUniform1f(lightness_id, 1.f);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (float*)&view.m[0][0]);
    esBindRenderF(0);
    
    // render water
    mIdent(&model);
    mSetPos(&model, (vec){0.f, 0.f, 0.f});
    mScale(&model, 1.f, 1.f, woff);
    updateModelView();
    esBindRenderF(1);

    // glEnable(GL_BLEND);
    // glUniform1f(opacity_id, 0.5f);
    // mIdent(&model);
    // mSetPos(&model, (vec){0.f, 0.f, 0.01f});
    // mScale(&model, 1.f, 1.f, woff);
    // updateModelView();
    // esBindRenderF(1);
    // glDisable(GL_BLEND);

    // shade lambert
    shadeLambert(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &ambient_id, &saturate_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (float*)&projection.m[0][0]);
    glUniform1f(ambient_id, 0.4f);
    glUniform1f(saturate_id, 0.5f);

    // render boat
    mIdent(&model);
    mSetPos(&model, (vec){0.f, 0.f, woff*-0.026f});
    updateModelView();
    esBindRender(2);

    // render last catch(es)
    if(last_fish[0] != -1)
    {
        mIdent(&model);
        mSetPos(&model, (vec){0.f, -0.14f, 0.04f+(woff*-0.026f)});
        updateModelView();
        esBindRender(last_fish[0]);
    }
    if(last_fish[1] != -1)
    {
        mIdent(&model);
        mSetPos(&model, (vec){0.02f, 0.2f, 0.05f+(woff*-0.026f)});
        mRotZ(&model, 90.f*DEG2RAD);
        updateModelView();
        esBindRender(last_fish[1]);
    }

    // render tux
    mIdent(&model);
    mSetPos(&model, (vec){0.f, 0.f, woff*-0.026f});
    mRotZ(&model, pr);
    updateModelView();
    esBindRender(3);

    // render rod
    mIdent(&model);
    mSetPos(&model, (vec){0.f, 0.f, 0.125378f+(woff*-0.026f)});
    mRotZ(&model, pr);
    mRotX(&model, rodr);
    updateModelView();
    esBindRender(4);

    // render float
    if(fp.x != 0.f || fp.y != 0.f || fp.z != 0.f)
    { 
        // is a fish hooked?
        if(hooked != -1)
        {
            // reel it in
            rodr = 0.8f;
            const float rs = 0.32f*dt;
            const float n1 = -fp.x*0.3f*dt;
            const float n2 = -frx*rs;
            const float o1 = -fp.y*0.3f*dt;
            const float o2 = -fry*rs;
            const float x1 = n2+o2, x2 = n1+o1;
            if(x1*x1 < x2*x2)
            {
                fp.x += n1;
                fp.y += o1;
            }
            else
            {
                fp.x += n2;
                fp.y += o2;
            }
            fp.z = getWaterHeight(fp.x, fp.y);
            if(vMag(fp) < 0.1f)
            {
                winning_fish = t+4.f;
                winning_fish_id = hooked;
                fp = (vec){0.f, 0.f, 0.f};
                last_fish[lfi] = hooked;
                if(++lfi > 1){lfi=0;}
                caught_list[hooked-7] = 1;
                hooked = -1;
                caught++;
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] Fish Caught: %u (%u/53)\n", strts, caught, ratioCaught());
                char tmp[256];
                sprintf(tmp, "Tux 🐟 %u (%u/53) 🐟 Fishing", caught, ratioCaught());
                SDL_SetWindowTitle(wnd, tmp);
            }
            else
            {
                // render fish
                mIdent(&model);
                mSetPos(&model, (vec){fp.x, fp.y, fp.z*woff});
                mRotZ(&model, frr);
                updateModelView();
                esBindRender(hooked);
            }
        }
        else
        {
            if(t > next_wild_fish)
            {
                const float rc = esRandFloat(0.f, 100.f);
                if(rc < 50.f)     {hooked = (int)roundf(esRandFloat( 7.f, 21.f));}
                else if(rc < 80.f){hooked = (int)roundf(esRandFloat(22.f, 34.f));}
                else if(rc < 90.f){hooked = (int)roundf(esRandFloat(35.f, 46.f));}
                else if(rc < 97.f){hooked = (int)roundf(esRandFloat(47.f, 58.f));}
                else{hooked = 59;}
                //hooked = (int)roundf(esRandFloat(7.f, 59.f));
                next_wild_fish = t + esRandFloat(23.f, 180.f);
            }

            if(cast == 1){glEnable(GL_BLEND);glUniform1f(opacity_id, 0.5f);}
            mIdent(&model);
            mSetPos(&model, (vec){fp.x, fp.y, fp.z*woff});
            updateModelView();
            esBindRender(5);
            if(cast == 1){glDisable(GL_BLEND);}
        }
    }

    // render jumping fish
    for(uint i=0; i<3; i++)
    {
        if(hooked == -1)
        {
            const float xm = fp.x - shoal_x[i];
            const float ym = fp.y - shoal_y[i];
            const float nd = xm*xm + ym*ym;
            if(nd < 0.3f){if(shoal_nt[i]-t < -4.5f){hooked = shoal_lfi[i];}}
        }

        if(shoal_nt[i]-t < -11.f){rndShoalPos(i);}

        const float d = shoal_nt[i]-t;
        if(d < 0.f && d >= -1.5f)
        {
            const float z = -0.03f+(0.33f*(fabsf(d)/1.5f));
            const float wah = (getWaterHeight(shoal_x[i], shoal_y[i])*woff)-0.016f;

            mIdent(&model);
            mSetPos(&model, (vec){shoal_x[i], shoal_y[i], wah});
            mRotZ(&model, t*0.3f);
            updateModelView();
            esBindRender(6);

            mIdent(&model);
            mSetPos(&model, (vec){shoal_x[i], shoal_y[i], z});
            shoal_r1[i] += esRandFloat(0.1f, 0.6f)*dt;
            shoal_r2[i] += esRandFloat(0.1f, 0.6f)*dt;
            shoal_r3[i] += esRandFloat(0.1f, 0.6f)*dt;
            mRotX(&model, shoal_r1[i]);
            mRotY(&model, shoal_r2[i]);
            mRotZ(&model, shoal_r3[i]);
            updateModelView();
            esBindRender(shoal_lfi[i]);
        }
        else if(d > -2.5f && d < -1.5f)
        {
            const float wah = (getWaterHeight(shoal_x[i], shoal_y[i])*woff)-0.016f;

            mIdent(&model);
            mSetPos(&model, (vec){shoal_x[i], shoal_y[i], wah});
            mRotZ(&model, t*0.3f);
            updateModelView();
            esBindRender(6);

            mIdent(&model);
            mSetPos(&model, (vec){shoal_x[i], shoal_y[i], 0.3f});
            shoal_r1[i] += esRandFloat(0.1f, 0.6f)*dt;
            shoal_r2[i] += esRandFloat(0.1f, 0.6f)*dt;
            shoal_r3[i] += esRandFloat(0.1f, 0.6f)*dt;
            mRotX(&model, shoal_r1[i]);
            mRotY(&model, shoal_r2[i]);
            mRotZ(&model, shoal_r3[i]);
            updateModelView();
            esBindRender(shoal_lfi[i]);
        }
        else if(d > -5.5f && d < -2.5f)
        {
            const float z = 0.3f-(0.303f*(fabsf(d+2.5f)/1.5f));
            const float wah = (getWaterHeight(shoal_x[i], shoal_y[i])*woff)-0.016f;

            glEnable(GL_BLEND);
            glUniform1f(opacity_id, d+4.5f);
            mIdent(&model);
            mSetPos(&model, (vec){shoal_x[i], shoal_y[i], wah});
            mRotZ(&model, t*0.3f);
            updateModelView();
            esBindRender(6);
            glDisable(GL_BLEND);

            mIdent(&model);
            mSetPos(&model, (vec){shoal_x[i], shoal_y[i], z});
            shoal_r1[i] += esRandFloat(0.1f, 0.6f)*dt;
            shoal_r2[i] += esRandFloat(0.1f, 0.6f)*dt;
            shoal_r3[i] += esRandFloat(0.1f, 0.6f)*dt;
            mRotX(&model, shoal_r1[i]);
            mRotY(&model, shoal_r2[i]);
            mRotZ(&model, shoal_r3[i]);
            updateModelView();
            esBindRender(shoal_lfi[i]);
        }
    }

    // render winning fish
    if(winning_fish > t)
    {
        const float d = winning_fish - t;
        if(d < 1.f)
        {
            glEnable(GL_BLEND);
            glUniform1f(opacity_id, d);
            mIdent(&model);
            mSetPos(&model, (vec){0.f, 0.f, 0.37f});
            mScale1(&model, 3.f);
            mRotZ(&model, t*2.1f);
            updateModelView();
            esBindRender(winning_fish_id);
            glDisable(GL_BLEND);
        }
        else
        {
            mIdent(&model);
            mSetPos(&model, (vec){0.f, 0.f, 0.37f});
            mScale1(&model, 3.f);
            mRotZ(&model, t*2.1f);
            updateModelView();
            esBindRender(winning_fish_id);
        }
    }

    ///

    // display render
    SDL_GL_SwapWindow(wnd);
}

//*************************************
// process entry point
//*************************************
int main(int argc, char** argv)
{
    // allow custom msaa level
    int msaa = 16;
    if(argc >= 2){msaa = atoi(argv[1]);}

    // help
    printf("----\n");
    printf("James William Fletcher (github.com/mrbid)\n");
    printf("%s - 3D Fishing Game, with 53 species of fish!\n", appTitle);
    printf("----\n");
#ifndef WEB
    printf("One command line argument, msaa 0-16.\n");
    printf("e.g; ./tuxfishing 16\n");
    printf("----\n");
#endif
    printf("Mouse = Click & Drag to Rotate Camera, Scroll = Zoom Camera\n");
    printf("W,A / Arrows = Move Rod Cast Direction\n");
    printf("Space = Cast Rod, the higher the rod when you release space the farther the lure launches.\n");
    printf("If you see a fish jump out of the water throw a lure after it and you will catch it straight away.\n");
    printf("F = FPS to console.\n");
    printf("----\n");
    printf("All assets where generated using LUMA GENIE (https://lumalabs.ai/genie).\n");
    printf("----\n");
    printf("Tux made by Andy Cuccaro\n");
    printf("https://andycuccaro.gumroad.com/\n");
    printf("----\n");
    printf("Fishing Rod made by Shedmon\n");
    printf("https://sketchfab.com/shedmon\n");
    printf("----\n");
    SDL_version compiled;
    SDL_version linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    printf("Compiled against SDL version %u.%u.%u.\n", compiled.major, compiled.minor, compiled.patch);
    printf("Linked against SDL version %u.%u.%u.\n", linked.major, linked.minor, linked.patch);
    printf("----\n");

    // init sdl
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS) < 0)
    {
        printf("ERROR: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }
#ifdef WEB
    double width, height;
    emscripten_get_element_css_size("body", &width, &height);
    winw = (Uint32)width, winh = (Uint32)height;
#endif
    if(msaa > 0)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    wnd = SDL_CreateWindow(appTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winw, winh, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    while(wnd == NULL)
    {
        msaa--;
        if(msaa == 0)
        {
            printf("ERROR: SDL_CreateWindow(): %s\n", SDL_GetError());
            return 1;
        }
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);
        wnd = SDL_CreateWindow(appTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winw, winh, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    }
    SDL_GL_SetSwapInterval(1); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync
    glc = SDL_GL_CreateContext(wnd);
    if(glc == NULL)
    {
        printf("ERROR: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return 1;
    }

    // set icon
    s_icon = surfaceFromData((Uint32*)&icon_image, 16, 16);
    SDL_SetWindowIcon(wnd, s_icon);

//*************************************
// bind vertex and index buffers
//*************************************
    register_sky();
    register_water();
    register_boat();
    register_tux();
    register_rod();
    register_float();
    register_splash();

    register_a0();
    register_a1();
    register_a2();
    register_a3();
    register_a4();
    register_a5();
    register_a6();
    register_a7();
    register_a8();
    register_a9();
    register_a10();
    register_a11();
    register_a12();
    register_a13();
    register_a14();

    register_b0();
    register_b1();
    register_b2();
    register_b3();
    register_b4();
    register_b5();
    register_b6();
    register_b7();
    register_b8();
    register_b9();
    register_b10();
    register_b11();
    register_b12();

    register_c0();
    register_c1();
    register_c2();
    register_c3();
    register_c4();
    register_c5();
    register_c6();
    register_c8();
    register_c9();
    register_c10();
    register_c11();

    register_d0();
    register_d1();
    register_d2();
    register_d3();
    register_d4();
    register_d5();
    register_d6();
    register_d7();
    register_d8();
    register_d9();
    register_d10();
    register_d11();

    register_e1();

//*************************************
// configure render options
//*************************************
    makeLambert();
    makeFullbright();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.f, 0.f, 0.f, 0.f);

    shadeFullbright(&position_id, &projection_id, &modelview_id, &color_id, &lightness_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (float*)&projection.m[0][0]);
    updateWindowSize(winw, winh);

#ifdef GL_DEBUG
    esDebug(1);
#endif

//*************************************
// execute update / render loop
//*************************************

    // init
    srand(time(0));
    srandf(time(0));
    t = fTime();
    lfct = t;

    // game init
    resetGame(0);

    // loop
#ifdef WEB
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_FALSE, emscripten_resize_event);
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while(1){main_loop();}
#endif

    // done
    return 0;
}
