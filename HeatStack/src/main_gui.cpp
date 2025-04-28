// main_gui.cpp

#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>  // for std::min_element, std::max_element
#include <array>

// HeatStack includes
#include "MeshHandler.h"
#include "InitialTemperature.h"
#include "BoundaryConditions.h"
#include "HeatEquationSolver.h"
#include "TimeHandler.h"
#include "MaterialProperties.h"
#include "TemperatureComparator.h"  // Added missing header

// GUI + OpenGL includes
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "tinyfiledialogs.h"

#include <sstream>

// 🌟 Global GUI Variables 🌟
char meshPath[512] = "";
char initTempPath[512] = "";
float simDuration = 10.0f;
float timeStep = 0.1f;
bool triggerSimulation = false;
bool simulationCompleted = false;
float progress = 0.0f;
std::string appLog;
std::string currentProcessingStatus = "";  // Added for processing status

// Add missing inputs to the GUI
static int nSlices = 10;
static int pointsPerLayer = 100;
static bool useAdaptiveTimeStep = false;
static float theta = 0.5f;
static char outputFile[512] = "summary_output.csv";

// Reset function to clear all inputs
void resetSimulation() {
    meshPath[0] = '\0';
    initTempPath[0] = '\0';
    simDuration = 10.0f;
    timeStep = 0.1f;
    nSlices = 10;
    pointsPerLayer = 100;
    useAdaptiveTimeStep = false;
    theta = 0.5f;
    strcpy_s(outputFile, "summary_output.csv");
    progress = 0.0f;
    appLog.clear();
    currentProcessingStatus.clear();
    simulationCompleted = false;
    triggerSimulation = false;
}

// 🌟 Mesh Handler and Solver Objects (keep them persistent)
MeshHandler meshHandler;
HeatEquationSolver solver;

// Global variables for 3D visualization
bool cameraMovementEnabled = false;
float camZoom = 1.0f;
float camRotX = 0.0f;  // Camera rotation around X axis (pitch)
float camRotY = 0.0f;  // Camera rotation around Y axis (yaw)
float camPosX = 0.0f;  // Camera position
float camPosY = 0.0f;
float camPosZ = -5.0f;
bool renderWireframe = false;
float lastX = 0.0f;    // Last mouse position for camera rotation
float lastY = 0.0f;
bool firstMouse = true;
const float mouseSensitivity = 0.1f;

// Helper function for perspective projection since some systems don't have gluPerspective
void perspectiveGL(GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar) {
    const GLdouble pi = 3.1415926535897932384626433832795;
    GLdouble fW, fH;
    fH = tan(fovY / 360 * pi) * zNear;
    fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

// ---- Updated Simulation Logic ----
void runSimulationLogic() {
    try {
        if (strlen(meshPath) == 0) {
            appLog += "❌ Error: Please select a mesh file.\n";
            return;
        }

        // Load mesh and retrieve vertices
        if (!meshHandler.loadMesh(meshPath)) {
            appLog += "❌ Error: Failed to load mesh file.\n";
            return;
        }
        
        // Get mesh bounds
        double zmin = meshHandler.getMinZ();
        double zmax = meshHandler.getMaxZ();
        double height = zmax - zmin;

        // Material properties
        MaterialProperties matProps;

        // Open output files
        std::ofstream summaryOut(outputFile);
        std::ofstream detailsOut("stack_details.csv");
        summaryOut << "slice,l/L,method,finalSteelTemp,TPS_thickness\n";
        detailsOut << "slice,l/L,TPS_thickness,CarbonFiber_thickness,Glue_thickness,Steel_thickness,finalSteelTemp\n";

        double totalSolveMs = 0.0;

        // Loop over all slices
        for (int slice = 0; slice < nSlices; ++slice) {
            currentProcessingStatus = "Processing stack " + std::to_string(slice + 1) + " of " + std::to_string(nSlices);
            
            double z = zmin + (double(slice)/(nSlices-1)) * height;
            double lL = (z - zmin) / height;
            
            // Initialize timer
            TimeHandler timer(simDuration, timeStep, useAdaptiveTimeStep);

            // Create stack with proper initialization
            Stack stack;
            stack.id = slice + 1;
            
            stack.layers = {
                {{"TPS",         0.2,  160.0, 1200.0,   0.0, 1200.0}, matProps.getTPSThickness(lL),          pointsPerLayer},
                {{"CarbonFiber", 500.0,1600.0, 700.0,   0.0,  350.0}, matProps.getCarbonFiberThickness(lL),     pointsPerLayer},
                {{"Glue",        200.0,1300.0, 900.0,   0.0,  400.0}, matProps.getGlueThickness(lL),            pointsPerLayer},
                {{"Steel",       100.0,7850.0, 500.0, 800.0,    0.0}, matProps.getSteelThickness(lL),           pointsPerLayer}
            };
            matProps.generateGrid(stack, pointsPerLayer);

            // Initialize solver
            solver = HeatEquationSolver(theta);
            solver.initialize(stack, timer);

            // Set initial temperature
            std::vector<double> initialTemps;
            if (strlen(initTempPath) > 0) {
                InitialTemperature tempLoader;
                initialTemps = tempLoader.loadInitialTemperature(initTempPath);
            }
            
            if (!initialTemps.empty()) {
                solver.setInitialTemperature(initialTemps);
            } else {
                solver.setInitialTemperature(std::vector<double>(stack.xGrid.size(), 300.0));
                if (slice == 0) { // Only log this once
                    appLog += "Using default initial temperature: 300K\n";
                }
            }

            // Set boundary conditions
            solver.setBoundaryConditions(
                new DirichletCondition(static_cast<float>(matProps.getExhaustTemp(lL))),
                new NeumannCondition(0.0f)
            );

            // Timer for solver per slice
            auto solve_start = std::chrono::high_resolution_clock::now();

            // Run simulation for this slice
            while (!timer.isFinished()) {
                solver.step();
                timer.advance();
                // Update progress (divide by nSlices to show overall progress)
                progress = (slice + static_cast<float>(timer.getStepCount()) / (simDuration / timeStep)) / nSlices;
            }

            auto solve_end = std::chrono::high_resolution_clock::now();
            double solveMs = std::chrono::duration<double, std::milli>(solve_end - solve_start).count();
            totalSolveMs += solveMs;

            // Get results for this slice
            const auto& Tdist = solver.getTemperatureDistribution();
            double steelT = Tdist.back();
            double tpsThick = matProps.getTPSThickness(lL);
            double cfThick = matProps.getCarbonFiberThickness(lL);
            double glueThick = matProps.getGlueThickness(lL);
            double steelThick = matProps.getSteelThickness(lL);

            // Suggest TPS thickness
            currentProcessingStatus = "Optimizing TPS thickness for stack " + std::to_string(slice + 1);
            TemperatureComparator comp;
            double tpsOpt = comp.suggestTPSThickness(stack, 800.0, simDuration, lL, matProps);

            // Write results for this slice
            summaryOut << (slice + 1) << "," << lL << ",BTCS," << steelT << "," << tpsOpt << "\n";
            detailsOut << (slice + 1) << ","
                      << lL << ","
                      << tpsThick << ","
                      << cfThick << ","
                      << glueThick << ","
                      << steelThick << ","
                      << steelT << "\n";

            // Export final temperature distribution for last slice only
            if (slice == nSlices - 1) {
                std::ofstream outFile("final_temperature.csv");
                for (const auto& temp : Tdist) {
                    outFile << temp << "\n";
                }
                outFile.close();
            }
        }

        // Clear processing status when done
        currentProcessingStatus.clear();

        // Close output files
        summaryOut.close();
        detailsOut.close();

        appLog += "✅ Simulation completed!\n";
        appLog += "Processed " + std::to_string(nSlices) + " slices.\n";
        appLog += "Total solver time: " + std::to_string(totalSolveMs) + " ms\n";
        appLog += "- final_temperature.csv: Temperature distribution (last slice)\n";
        appLog += "- " + std::string(outputFile) + ": Summary results\n";
        appLog += "- stack_details.csv: Detailed layer information\n";
        simulationCompleted = true;
    } catch (const std::exception& e) {
        appLog += "❌ Exception: ";
        appLog += e.what();
        appLog += "\n";
        currentProcessingStatus.clear();
    }
}

// ---- GUI Simulation Controls ----
void renderSimulationControls() {
    ImGui::Begin("HeatStack Simulation");

    ImGui::InputText("Mesh Path", meshPath, sizeof(meshPath));
    ImGui::SameLine();
    if (ImGui::Button("Browse Mesh")) {
        const char* filter[] = { "*.obj", "*.csv" };
        const char* file = tinyfd_openFileDialog("Select Mesh", "", 2, filter, "OBJ or CSV files", 0);
        if (file) strncpy_s(meshPath, sizeof(meshPath), file, _TRUNCATE);
    }

    ImGui::InputText("Initial Temp", initTempPath, sizeof(initTempPath));
    ImGui::SameLine();
    if (ImGui::Button("Browse Temp")) {
        const char* filter[] = { "*.csv" };
        const char* file = tinyfd_openFileDialog("Select Initial Temp CSV", "", 1, filter, "CSV files", 0);
        if (file) strncpy_s(initTempPath, sizeof(initTempPath), file, _TRUNCATE);
    }

    ImGui::InputFloat("Simulation Duration (s)", &simDuration);
    ImGui::InputFloat("Time Step (s)", &timeStep);

    ImGui::InputInt("Number of Slices", &nSlices);
    ImGui::InputInt("Points Per Layer", &pointsPerLayer);
    ImGui::Checkbox("Use Adaptive Time Step", &useAdaptiveTimeStep);
    ImGui::InputFloat("Theta Parameter", &theta);
    ImGui::InputText("Output File", outputFile, sizeof(outputFile));

    if (ImGui::Button("Run Simulation")) {
        triggerSimulation = true;
        simulationCompleted = false;
        progress = 0.0f;
        appLog.clear();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Reset All")) {
        resetSimulation();
    }

    if (triggerSimulation && !simulationCompleted) {
        runSimulationLogic();
        triggerSimulation = false;
    }

    ImGui::Separator();
    ImGui::ProgressBar(progress, ImVec2(-1, 0));
    if (!currentProcessingStatus.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", currentProcessingStatus.c_str());
    }
    ImGui::TextWrapped("%s", appLog.c_str());

    ImGui::End();
}

// Helper function for ImGui::PlotLines
static float TempGetter(void* data, int idx)
{
    const std::vector<double>* temps = static_cast<const std::vector<double>*>(data);
    return static_cast<float>((*temps)[idx]);
}

// Helper function to draw coordinate axes
void drawCoordinateAxes() {
    glLineWidth(2.0f);
    glBegin(GL_LINES);
        // X axis (red)
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(1.0f, 0.0f, 0.0f);
        // Y axis (green)
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 1.0f, 0.0f);
        // Z axis (blue)
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 1.0f);
    glEnd();
    glLineWidth(1.0f);
}

// Helper function to draw the mesh with temperature colors
void drawMeshWithTemperatures(const std::vector<double>& temperatures) {
    const auto& vertices = meshHandler.getVertices();
    const auto& faces = meshHandler.getFaces();
    
    if (vertices.empty() || temperatures.empty())
        return;

    // Find temperature range
    float minTemp = static_cast<float>(*std::min_element(temperatures.begin(), temperatures.end()));
    float maxTemp = static_cast<float>(*std::max_element(temperatures.begin(), temperatures.end()));
    float tempRange = maxTemp - minTemp;

    // Draw the mesh faces
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < faces.size(); ++i) {
        const auto& face = faces[i];
        // For each vertex in the face
        for (int j = 0; j < 3; ++j) {
            int vertexIdx = face[j];
            if (vertexIdx < temperatures.size()) {
                // Convert temperature to color (blue=cold to red=hot)
                float t = (temperatures[vertexIdx] - minTemp) / tempRange;
                glColor3f(t, 0.0f, 1.0f - t);  // Red to Blue gradient
            } else {
                glColor3f(1.0f, 1.0f, 1.0f);  // White for vertices without temperature
            }
            
            // Get vertex position
            const auto& vertex = vertices[vertexIdx];
            glVertex3f(static_cast<float>(vertex[0]), 
                      static_cast<float>(vertex[1]), 
                      static_cast<float>(vertex[2]));
        }
    }
    glEnd();
}

// Updated render visualization function with proper OpenGL initialization and rendering setup
void renderVisualization() {
    ImGui::Begin("3D Visualization");
    
    // Get the content region for the visualization
    ImVec2 viewportPos = ImGui::GetCursorScreenPos();
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    
    // Handle mouse input for camera control
    if (cameraMovementEnabled && ImGui::IsWindowHovered()) {
        ImVec2 mousePos = ImGui::GetMousePos();
        mousePos.x -= viewportPos.x;
        mousePos.y -= viewportPos.y;
        
        if (ImGui::IsMouseDown(0)) {  // Left mouse button
            if (firstMouse) {
                lastX = mousePos.x;
                lastY = mousePos.y;
                firstMouse = false;
            }
            
            float xoffset = mousePos.x - lastX;
            float yoffset = lastY - mousePos.y;
            lastX = mousePos.x;
            lastY = mousePos.y;
            
            xoffset *= mouseSensitivity;
            yoffset *= mouseSensitivity;
            
            camRotY += xoffset;
            camRotX += yoffset;
            
            // Clamp vertical rotation
            if (camRotX > 89.0f) camRotX = 89.0f;
            if (camRotX < -89.0f) camRotX = -89.0f;
        }
        
        // Handle zoom with mouse wheel
        camPosZ += ImGui::GetIO().MouseWheel * 0.5f;
        if (camPosZ > -0.1f) camPosZ = -0.1f;
        if (camPosZ < -20.0f) camPosZ = -20.0f;
    }
    
    if (ImGui::IsMouseReleased(0)) {
        firstMouse = true;
    }
    
    // Set up OpenGL viewport and camera
    glViewport(static_cast<int>(viewportPos.x), 
              static_cast<int>(viewportPos.y), 
              static_cast<int>(viewportSize.x), 
              static_cast<int>(viewportSize.y));
              
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = viewportSize.x / viewportSize.y;
    perspectiveGL(45.0f, aspect, 0.1f, 100.0f);
    
    // Clear with proper depth buffer
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable depth testing and backface culling
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Setup basic lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 0.0f };
    GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(camPosX, camPosY, camPosZ);
    glRotatef(camRotX, 1.0f, 0.0f, 0.0f);
    glRotatef(camRotY, 0.0f, 1.0f, 0.0f);
    
    // Add a scale to make the model more visible
    glScalef(0.1f, 0.1f, 0.1f);  // Scale down the model
    
    // Draw coordinate axes
    glDisable(GL_LIGHTING);
    drawCoordinateAxes();
    glEnable(GL_LIGHTING);
    
    // Set wireframe mode if enabled
    if (renderWireframe) {
        glDisable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    static bool meshLoaded = false;
    if (strlen(meshPath) > 0 && !meshLoaded) {
        if (meshHandler.loadMesh(meshPath)) {
            meshLoaded = true;
            appLog += "✅ Mesh loaded successfully.\n";
        }
    }

    // Draw mesh with proper materials
    if (meshLoaded) {
        GLfloat material_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
        GLfloat material_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
        GLfloat material_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
        GLfloat material_shininess = 32.0f;
        
        glMaterialfv(GL_FRONT, GL_AMBIENT, material_ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, material_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, material_specular);
        glMaterialf(GL_FRONT, GL_SHININESS, material_shininess);
        
        if (simulationCompleted) {
            const auto& temps = solver.getTemperatureDistribution();
            if (!temps.empty()) {
                drawMeshWithTemperatures(temps);
            }
        } else {
            const auto& vertices = meshHandler.getVertices();
            const auto& faces = meshHandler.getFaces();
            
            glBegin(GL_TRIANGLES);
            for (const auto& face : faces) {
                // Calculate face normal for proper lighting
                const auto& v1 = vertices[face[0]];
                const auto& v2 = vertices[face[1]];
                const auto& v3 = vertices[face[2]];
                
                // Compute normal using cross product
                float nx = (v2[1] - v1[1]) * (v3[2] - v1[2]) - (v2[2] - v1[2]) * (v3[1] - v1[1]);
                float ny = (v2[2] - v1[2]) * (v3[0] - v1[0]) - (v2[0] - v1[0]) * (v3[2] - v1[2]);
                float nz = (v2[0] - v1[0]) * (v3[1] - v1[1]) - (v2[1] - v1[1]) * (v3[0] - v1[0]);
                float len = sqrt(nx*nx + ny*ny + nz*nz);
                if (len > 0) {
                    nx /= len; ny /= len; nz /= len;
                }
                
                glNormal3f(nx, ny, nz);
                for (int i = 0; i < 3; ++i) {
                    const auto& vertex = vertices[face[i]];
                    glVertex3f(vertex[0], vertex[1], vertex[2]);
                }
            }
            glEnd();
        }
    }
    
    // Reset OpenGL state
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // Add visualization controls
    ImGui::Separator();
    ImGui::Checkbox("Enable Camera Movement", &cameraMovementEnabled);
    ImGui::Checkbox("Wireframe Mode", &renderWireframe);
    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", camPosX, camPosY, camPosZ);
    ImGui::Text("Camera Rotation: (%.1f, %.1f)", camRotX, camRotY);
    ImGui::Text("Controls:");
    ImGui::Text("- Left Mouse Button: Rotate Camera");
    ImGui::Text("- Mouse Wheel: Zoom");
    
    ImGui::End();
}

int main(int argc, char** argv) {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Create Window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "HeatStack Simulator", NULL, NULL);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create GLFW window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Main Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Get window size for proper viewport setup
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        
        // Clear the entire window first
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Enable depth testing globally
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        renderSimulationControls();
        renderVisualization();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
