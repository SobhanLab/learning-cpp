#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>

// Shared data
std::vector<float> cpuHistory;
std::mutex dataMutex;
std::atomic<bool> running(true);

// Read CPU usage from /proc/stat
float getCPUUsage() {
    static long long prevIdle = 0, prevTotal = 0;

    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line);

    std::istringstream ss(line);
    std::string cpu;
    long long user, nice, system, idle, iowait, irq, softirq;

    ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;

    long long idleTime = idle + iowait;
    long long totalTime = user + nice + system + idle + iowait + irq + softirq;

    long long deltaIdle = idleTime - prevIdle;
    long long deltaTotal = totalTime - prevTotal;

    prevIdle = idleTime;
    prevTotal = totalTime;

    if (deltaTotal == 0) return 0.0f;

    return 100.0f * (1.0f - (float)deltaIdle / deltaTotal);
}

// Worker thread
void monitorCPU() {
    while (running) {
        float usage = getCPUUsage();

        {
            std::lock_guard<std::mutex> lock(dataMutex);
            cpuHistory.push_back(usage);
            if (cpuHistory.size() > 100)
                cpuHistory.erase(cpuHistory.begin());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main() {
    // Init GLFW
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 500, "CPU Monitor", NULL, NULL);
    glfwMakeContextCurrent(window);

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Start monitoring thread
    std::thread worker(monitorCPU);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("CPU Monitor");

        std::vector<float> dataCopy;
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            dataCopy = cpuHistory;
        }

        if (!dataCopy.empty()) {
            ImGui::PlotLines("CPU Usage (%)", dataCopy.data(), dataCopy.size(), 0, NULL, 0.0f, 100.0f, ImVec2(0, 150));
            ImGui::Text("Current: %.2f%%", dataCopy.back());
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    running = false;
    worker.join();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}