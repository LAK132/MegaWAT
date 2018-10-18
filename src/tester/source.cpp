#include "source.h"

void init(lak::loopData_t &ld)
{
    ld.userData = new userData_t;
    userData_t &ud = *ld.userData;

    assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) == 0);

    ld.screen.w = 1280;
    ld.screen.h = 720;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    ld.window = SDL_CreateWindow("MegaWAT!?", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ld.screen.w, ld.screen.h, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    ld.context = SDL_GL_CreateContext(ld.window);
    SDL_GL_MakeCurrent(ld.window, ld.context);
    if (SDL_GL_SetSwapInterval(-1) == -1) // adaptive vsync
    {
        SDL_GL_SetSwapInterval(1); // standard vsync
    }

    gl3wInit();

    // ImGui::CreateContext();
    // ud.io = &ImGui::GetIO();
    // ImGui_ImplSdlGL3_Init(ld.window);
    // ImGui::StyleColorsDark();
    // ud.style = &ImGui::GetStyle();
    // ud.style->WindowRounding = 0;
    // ImGui_ImplSdlGL3_NewFrame(ld.window);

    glViewport(0, 0, ld.screen.w, ld.screen.h);
    glClearColor(ud.clearColor[0], ud.clearColor[1], ud.clearColor[2], ud.clearColor[3]);
    glEnable(GL_DEPTH_TEST);
}

void update(lak::loopData_t &ld)
{
    assert(ld.userData != nullptr);
    userData_t &ud = *ld.userData;

    for (auto &event : ld.events)
    {
        switch(event.type)
        {
            case SDL_QUIT: {
                ld.running = false;
            } break;
        }
        // ImGui_ImplSdlGL3_ProcessEvent(&event);
    }
    ld.events.clear();
}

void draw(lak::loopData_t &ld)
{
    assert(ld.userData != nullptr);
    userData_t &ud = *ld.userData;
    glViewport(0, 0, ld.screen.w, ld.screen.h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


}

void shutdown(lak::loopData_t &ld)
{
    if (ld.userData != nullptr)
        delete ld.userData;

    // ImGui_ImplSdlGL3_Shutdown();
    // ImGui::DestroyContext();

    SDL_GL_DeleteContext(ld.context);
    SDL_DestroyWindow(ld.window);

    SDL_Quit();
}

void event(lak::loopData_t &ld)
{
    assert(ld.userData != nullptr);
    userData_t &ud = *ld.userData;

    SDL_Event event;
    while (SDL_PollEvent(&event) && ld.events.size() < 100)
    {
        ld.events.push_back(event);
    }
    // ImGui_ImplSdlGL3_NewFrame(ld.window);
}