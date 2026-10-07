// Сторонние библиотеки
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

// Стандартные библиотеки
#include <cstdint>
#include <string>
#include <vector>
#include <cstdio>
#include <array>
#include <algorithm>
#include <iostream>
#include <stack>



SDL_Window* window = nullptr; // Указатель на окно
SDL_Renderer* renderer = nullptr; // Указатель на рендерер для отрисовки
SDL_Event eventer; // Контролёр событий

struct MainImageCallback // Callback, вызываемый ImGui внутри RenderDrawData
{
    SDL_Renderer* renderer = nullptr;
    ImVec2 origin;
    ImVec2 area;
};


std::vector<ImVec2>  Graham_convex_hull(const std::vector<ImVec2>& current_vertices) {
    std::vector<ImVec2> convex_hull = {};

    ImVec2 start_vertice = current_vertices[0];
    std::vector<ImVec2> other_vertices = {};

    // находим самую нижнюю левую точку
    for (int i = 1; i < current_vertices.size(); ++i) {
        if (current_vertices[i].y < start_vertice.y || current_vertices[i].y == start_vertice.y && current_vertices[i].x < start_vertice.x)
        {
            other_vertices.push_back(start_vertice);
            start_vertice = current_vertices[i];
        }
        else other_vertices.push_back(current_vertices[i]);
    }

    // сортируем остальные по полярному углу
    std::sort(other_vertices.begin(), other_vertices.end(),
        [start_vertice](const ImVec2& a, const ImVec2& b)
        {
            if (std::atan2(a.y, a.x) == std::atan2(b.y, b.x)) {
                int a_dist = (a.y - start_vertice.y) * (a.y - start_vertice.y) + (a.x - start_vertice.x) * (a.x - start_vertice.x);
                int b_dist = (b.y - start_vertice.y) * (b.y - start_vertice.y) + (b.x - start_vertice.x) * (b.x - start_vertice.x);
                return a_dist < b_dist;
            }
            return std::atan2(a.y - start_vertice.y, a.x - start_vertice.x) < std::atan2(b.y - start_vertice.y, b.x - start_vertice.x);
        });

    // построение оболочки
    convex_hull.push_back(start_vertice);
    for (ImVec2& vertice : other_vertices) {
        // пока в оболочке есть ребра и новая точка справа от предыдущего ребра - удаляем последнюю точку из оболочки
        while (convex_hull.size() > 1 &&
            (convex_hull[convex_hull.size() - 1].x - convex_hull[convex_hull.size() - 2].x) * (vertice.y - convex_hull[convex_hull.size() - 2].y)
            - (convex_hull[convex_hull.size() - 1].y - convex_hull[convex_hull.size() - 2].y) * (vertice.x - convex_hull[convex_hull.size() - 2].x) < 0)
            convex_hull.pop_back();

        // добавляем новую точку
        convex_hull.push_back(vertice);
    }

    return convex_hull;
}


int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) { // вызов SDL-подсистемы
        std::cerr << "SDL_Init failed" << std::endl;
        return -1;
    }
    MainImageCallback mainCb;

    window = SDL_CreateWindow("ImGui Window", 1000, 700, SDL_WINDOW_RESIZABLE); // Создание окна
    if (!window) { std::cerr << "SDL_CreateWindow failed" << std::endl; return 1; }

    renderer = SDL_CreateRenderer(window, nullptr); // Создание рендерера
    if (!renderer) { std::cerr << "SDL_CreateRenderer failed" << std::endl; return 1; }

    IMGUI_CHECKVERSION(); // Gроверки версий файлов .h библиотеки ImGUI
    ImGui::CreateContext(); // Создание глобального контекста для отрисовки
    ImGui::StyleColorsDark(); // Установка тёмного стиля

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer); // Dызов слоя, отвечающего за отрисовку в ImGUI

    ImGuiIO& io = ImGui::GetIO(); // Получение глобального объекта ввода-вывода (для контроля текущего состояния программы)

    bool running(true); // Флаг работы программы

    std::vector<ImVec2>  current_vertices = std::vector<ImVec2>(); // точки на холсте
    std::vector<ImVec2> current_convex_hull = std::vector<ImVec2>(); // выпуклая оболочка

    while (running) { // Пока не закрыли программу
        while (SDL_PollEvent(&eventer)) { // Контроль событий
            ImGui_ImplSDL3_ProcessEvent(&eventer);
            if (eventer.type == SDL_EVENT_QUIT) running = false;
        }

        ImGui_ImplSDLRenderer3_NewFrame(); // Подготовка внутренних буферов рендера ImGui
        ImGui_ImplSDL3_NewFrame(); // Передача накопленных событий ввода и актуализация состояния мыши/клавиатуры
        ImGui::NewFrame(); // Создание нового кадра, в течение которого формируется список виджетов

        // Отрисовка главного окна
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always); // 10 пикселей от левого верхнего угла окна
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 20, io.DisplaySize.y - 20), ImGuiCond_Always);
        ImGui::Begin("Graham convex hull", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        const float buttonPanelW = 260; // Ширина правой панели с кнопками
        ImVec2 contentAvail = ImGui::GetContentRegionAvail(); // Сколько места сейчас доступно внутри главного окна — ширина и высота в пикселях


        ImVec2 content_avail = ImGui::GetContentRegionAvail();
        const float BUTTON_PANEL_W(260.0f);
        float avail_w = content_avail.x;
        float avail_h = content_avail.y;
        float canvas_w = avail_w - BUTTON_PANEL_W - ImGui::GetStyle().ItemSpacing.x;

        // Левая область — холст
        ImGui::BeginChild("canvas", ImVec2(canvas_w, avail_h), ImGuiChildFlags_Borders);

        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        ImVec2 canvas_end(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);

        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Белый фон холста
        dl->AddRectFilled(canvas_pos, canvas_end, IM_COL32(255, 255, 255, 255));

        // Невидимая кнопка на всю площадь холста — чтобы ловить клики
        ImGui::InvisibleButton("canvas_click", canvas_size, ImGuiButtonFlags_MouseButtonLeft);
        bool hovered = ImGui::IsItemHovered();

        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mouse = ImGui::GetMousePos();
            current_vertices.emplace_back(mouse.x, mouse.y);
        }

        // вершины
        for (const ImVec2& p : current_vertices) {
            dl->AddCircleFilled(ImVec2(p.x, p.y), 4.0f, IM_COL32(0, 0, 0, 255));
        }

        // Рёбра
        for (size_t i = 1; i < current_convex_hull.size(); ++i) {
            dl->AddLine(ImVec2(current_convex_hull[i - 1].x, current_convex_hull[i - 1].y),
                ImVec2(current_convex_hull[i].x, current_convex_hull[i].y),
                IM_COL32(0, 0, 0, 255), 2.0f);
        }

        // Замыкающее ребро
        if (!current_convex_hull.empty())
            dl->AddLine(ImVec2(current_convex_hull.back().x, current_convex_hull.back().y),
                ImVec2(current_convex_hull.front().x, current_convex_hull.front().y),
                IM_COL32(0, 0, 0, 255), 2.0f);

        ImGui::EndChild();
        ImGui::SameLine();

        // Правая область — кнопки
        ImGui::BeginChild("control_panel", ImVec2(BUTTON_PANEL_W, avail_h),
            ImGuiChildFlags_Borders);

        ImGui::Spacing();

        if (ImGui::Button("convex hull", ImVec2(-FLT_MIN, 0))) {
            if (!current_vertices.empty())
                current_convex_hull = Graham_convex_hull(current_vertices);
        }
        ImGui::Spacing();

        if (ImGui::Button("clear canvas", ImVec2(-FLT_MIN, 0))) {
            current_vertices.clear();
            current_convex_hull.clear();
        }
        ImGui::EndChild();
        ImGui::End();

  

        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

       

        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}