// main_gui.cpp

#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>  // for std::min_element, std::max_element, and std::min
#include <array>
#include <sstream> // Added include
#include <future>  // Add for std::future and std::future_status

// Define M_PI if not defined (e.g. on MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// HeatStack includes
#include "MeshHandler.h"
#include "InitialTemperature.h"
#include "BoundaryConditions.h"
#include "HeatEquationSolver.h"
#include "TimeHandler.h"
#include "MaterialProperties.h"
#include "TemperatureComparator.h"

// GUI + OpenGL includes
#ifdef _WIN32
#define NOMINMAX // Prevent Windows headers from defining min/max macros
#include <windows.h>
#endif
#include <GL/gl.h>
// #include <GL/glu.h> // Using manual lookAt and perspective to avoid GLU dependency if needed
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "tinyfiledialogs.h"

// 🌟 Global GUI Variables 🌟
char meshPath[512] = "";
char initTempPath[512] = "";
float simDuration = 10.0f;
float timeStep = 0.1f;
bool triggerSimulation = false;
bool simulationCompleted = false;
float progress = 0.0f;
std::string appLog;
std::string currentProcessingStatus = "";

// Add missing inputs to the GUI
static int nSlices = 10;
static int pointsPerLayer = 100;
static bool useAdaptiveTimeStep = false;
static float theta = 0.5f;
static char outputFile[512] = "summary_output.csv";
static bool meshLoadedForVis = false; // Track if mesh is loaded for visualization

// 🌟 Mesh Handler and Solver Objects (keep them persistent)
MeshHandler meshHandler;
HeatEquationSolver solver; // Keep solver instance for potential result access

// Global variables for 3D visualization
bool cameraMovementEnabled = true; // Enable by default
float camDistance = 5.0f;       // Distance from origin (used for zoom)
float camAzimuth = 45.0f;       // Rotation around Y axis (degrees)
float camElevation = 30.0f;     // Rotation around X axis (degrees)
float camTargetX = 0.0f, camTargetY = 0.0f, camTargetZ = 0.0f; // Look-at point (for panning)
bool renderWireframe = false;
const float mouseSensitivity = 0.4f;
const float zoomSensitivity = 0.5f;
const float panSensitivity = 0.01f; // Sensitivity for panning
enum VisualizationMode { TEMPERATURE_VIS, THICKNESS_VIS };
VisualizationMode currentVisMode = THICKNESS_VIS; // Default to thickness visualization
bool showColorScale = true; // Whether to show the color scale
bool showSliceLines = true; // Add toggle for slice lines

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
    strcpy_s(outputFile, sizeof(outputFile), "summary_output.csv"); // Use strcpy_s for safety
    progress = 0.0f;
    appLog.clear();
    currentProcessingStatus.clear();
    simulationCompleted = false;
    triggerSimulation = false;
    meshLoadedForVis = false; // Reset mesh loaded flag
    // Reset camera ? Optional, might be nice to keep view
    // camDistance = 5.0f;
    // camAzimuth = 45.0f;
    // camElevation = 30.0f;
}

// Helper function for perspective projection
void perspectiveGL(GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar) {
    const GLdouble pi = 3.1415926535897932384626433832795;
    GLdouble fH = tan(fovY / 360 * pi) * zNear;
    GLdouble fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

// Helper function for lookAt (manual implementation to avoid GLU dependency)
void lookAtGL(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ,
              GLdouble centerX, GLdouble centerY, GLdouble centerZ,
              GLdouble upX, GLdouble upY, GLdouble upZ)
{
    GLdouble f[3], up[3], s[3], u[3];
    GLdouble m[16];

    f[0] = centerX - eyeX;
    f[1] = centerY - eyeY;
    f[2] = centerZ - eyeZ;

    up[0] = upX;
    up[1] = upY;
    up[2] = upZ;

    // Normalize f
    GLdouble len = sqrt(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
    if (len != 0) { f[0] /= len; f[1] /= len; f[2] /= len; }

    // Calculate s = f x up
    s[0] = f[1] * up[2] - f[2] * up[1];
    s[1] = f[2] * up[0] - f[0] * up[2];
    s[2] = f[0] * up[1] - f[1] * up[0];

    // Normalize s
    len = sqrt(s[0]*s[0] + s[1]*s[1] + s[2]*s[2]);
    if (len != 0) { s[0] /= len; s[1] /= len; s[2] /= len; }

    // Calculate u = s x f
    u[0] = s[1] * f[2] - s[2] * f[1];
    u[1] = s[2] * f[0] - s[0] * f[2];
    u[2] = s[0] * f[1] - s[1] * f[0];

    // Set matrix
    m[0] = s[0]; m[4] = s[1]; m[8] = s[2];  m[12] = 0.0;
    m[1] = u[0]; m[5] = u[1]; m[9] = u[2];  m[13] = 0.0;
    m[2] = -f[0]; m[6] = -f[1]; m[10] = -f[2]; m[14] = 0.0;
    m[3] = 0.0;  m[7] = 0.0;  m[11] = 0.0;  m[15] = 1.0;

    glMultMatrixd(m);
    glTranslated(-eyeX, -eyeY, -eyeZ);
}

// ---- Simulation Logic ----
void runSimulationLogic() {
    try {
        if (strlen(meshPath) == 0) {
            appLog += "❌ Error: Please select a mesh file.\n";
            triggerSimulation = false; // Stop trigger if error
            return;
        }

        // Use the MeshHandler constructor with filename parameter
        meshHandler = MeshHandler(meshPath); // Use the constructor that takes a filename
        
        // Check if mesh is properly loaded
        if (meshHandler.getVertices().empty() || meshHandler.getFaces().empty()) {
            appLog += "❌ Error: Failed to load mesh file or mesh is empty: " + std::string(meshPath) + "\n";
            triggerSimulation = false; // Stop trigger if error
            meshLoadedForVis = false;
            return;
        }
        meshLoadedForVis = true; // Mark mesh as loaded for visualization
        appLog += "✅ Mesh loaded successfully for simulation.\n";

        // Get mesh bounds - use safe accessors with error checking
        double zmin = meshHandler.getMinZ();
        double zmax = meshHandler.getMaxZ();
        double height = zmax - zmin;
        if (height <= 0) height = 1.0; // Avoid division by zero if mesh is flat

        // Material properties
        MaterialProperties matProps;

        // Open output files with better error handling
        std::ofstream summaryOut(outputFile);
         if (!summaryOut) {
            appLog += "❌ Error: Could not open summary output file: " + std::string(outputFile) + "\n";
            triggerSimulation = false; return;
        }
        std::ofstream detailsOut("stack_details.csv");
        if (!detailsOut) {
            appLog += "❌ Error: Could not open details output file: stack_details.csv\n";
            triggerSimulation = false; return;
        }

        summaryOut << "slice,l/L,method,finalSteelTemp,TPS_thickness\n";
        detailsOut << "slice,l/L,TPS_thickness,CarbonFiber_thickness,Glue_thickness,Steel_thickness,finalSteelTemp\n";

        double totalSolveMs = 0.0;
        HeatEquationSolver currentSolver; // Local solver for the simulation run

        // Loop over all slices
        for (int slice = 0; slice < nSlices; ++slice) {
            currentProcessingStatus = "Processing stack " + std::to_string(slice + 1) + " of " + std::to_string(nSlices);

            double z = zmin + (nSlices > 1 ? (double(slice)/(nSlices-1)) * height : height / 2.0); // Handle nSlices=1 case
            double lL = (nSlices > 1 ? (z - zmin) / height : 0.5);
             if (height <= 0) lL = 0.0; // Handle flat mesh case

            // Initialize timer
            TimeHandler timer(simDuration, timeStep, useAdaptiveTimeStep);

            // Create stack with proper initialization
            Stack stack;
            stack.id = slice + 1;

             // Define layers within the loop for clarity
            Layer tpsLayer       = {{"TPS",         0.2,  160.0, 1200.0,   0.0, 1200.0}, matProps.getTPSThickness(lL),          pointsPerLayer};
            Layer carbonFiberLayer = {{"CarbonFiber", 500.0,1600.0, 700.0,   0.0,  350.0}, matProps.getCarbonFiberThickness(lL),     pointsPerLayer};
            Layer glueLayer      = {{"Glue",        200.0,1300.0, 900.0,   0.0,  400.0}, matProps.getGlueThickness(lL),            pointsPerLayer};
            Layer steelLayer     = {{"Steel",       100.0,7850.0, 500.0, 800.0,    0.0}, matProps.getSteelThickness(lL),           pointsPerLayer};

            stack.layers = { tpsLayer, carbonFiberLayer, glueLayer, steelLayer };

            matProps.generateGrid(stack, pointsPerLayer);

             if (stack.xGrid.empty()) {
                appLog += "❌ Error: Grid generation failed for slice " + std::to_string(slice + 1) + "\n";
                continue; // Skip this slice
            }

            // Initialize solver for this slice
            currentSolver = HeatEquationSolver(theta); // Use the local solver
            currentSolver.initialize(stack, timer);

            // Set initial temperature
            std::vector<double> initialTemps;
            if (strlen(initTempPath) > 0) {
                InitialTemperature tempLoader;
                if (!std::filesystem::exists(initTempPath)) {
                     appLog += "⚠️ Warning: Initial temperature file not found: " + std::string(initTempPath) + ". Using default 300K.\n";
                } else {
                    initialTemps = tempLoader.loadInitialTemperature(initTempPath);
                     if (initialTemps.empty()) {
                         appLog += "⚠️ Warning: Failed to load or empty initial temperature file. Using default 300K.\n";
                    }
                }
            }

            if (!initialTemps.empty()) {
                 // Check if size matches grid size
                if (initialTemps.size() != stack.xGrid.size()) {
                     appLog += "⚠️ Warning: Initial temperature data size mismatch (expected "
                              + std::to_string(stack.xGrid.size()) + ", got "
                              + std::to_string(initialTemps.size()) + "). Using default 300K.\n";
                     currentSolver.setInitialTemperature(std::vector<double>(stack.xGrid.size(), 300.0));
                } else {
                    currentSolver.setInitialTemperature(initialTemps);
                }
            } else {
                currentSolver.setInitialTemperature(std::vector<double>(stack.xGrid.size(), 300.0));
                if (slice == 0) { // Only log this once per run
                    appLog += "Using default initial temperature: 300K\n";
                }
            }

            // Set boundary conditions
             try {
                 currentSolver.setBoundaryConditions(
                    new DirichletCondition(static_cast<float>(matProps.getExhaustTemp(lL))), 
                    new NeumannCondition(0.0f)
                 );
             } catch (const std::exception& bc_err) {
                appLog += "❌ Error setting boundary conditions: " + std::string(bc_err.what()) + "\n";
                continue; // Skip slice if BCs fail
            }

            // Timer for solver per slice
            auto solve_start = std::chrono::high_resolution_clock::now();

            // Run simulation for this slice
             double maxSteps = (timeStep > 0) ? (simDuration / timeStep) : 1.0; // Avoid division by zero
             if (maxSteps <= 0) maxSteps = 1.0;

            while (!timer.isFinished()) {
                 try {
                     currentSolver.step();
                 } catch (const std::exception& step_err) {
                    appLog += "❌ Error during solver step for slice " + std::to_string(slice+1) + ": " + std::string(step_err.what()) + "\n";
                    goto next_slice; // Break inner loop, go to next slice
                }
                timer.advance();
                // Update progress (divide by nSlices to show overall progress)
                progress = (slice + (static_cast<float>(timer.getStepCount()) / maxSteps)) / nSlices;
            }

            next_slice:; // Label for jumping to next slice on error

            auto solve_end = std::chrono::high_resolution_clock::now();
            double solveMs = std::chrono::duration<double, std::milli>(solve_end - solve_start).count();
            totalSolveMs += solveMs;

            // Get results for this slice
            const auto& Tdist = currentSolver.getTemperatureDistribution();
            if (Tdist.empty()) {
                appLog += "⚠️ Warning: No temperature distribution result for slice " + std::to_string(slice + 1) + "\n";
                continue; // Skip results processing for this slice
            }
             double steelT = Tdist.back(); // Assume last point is steel surface
            double tpsThick = matProps.getTPSThickness(lL);
            double cfThick = matProps.getCarbonFiberThickness(lL);
            double glueThick = matProps.getGlueThickness(lL);
            double steelThick = matProps.getSteelThickness(lL);

            // Suggest TPS thickness
             double tpsOpt = -1.0; // Default invalid value
             try {
                currentProcessingStatus = "Optimizing TPS thickness for stack " + std::to_string(slice + 1);
                TemperatureComparator comp;
                // Ensure stack is valid before passing to comparator
                if (!stack.layers.empty() && !stack.xGrid.empty()) {
                     tpsOpt = comp.suggestTPSThickness(
                         stack,
                         800.0,   // max steel temp @ steel/glue
                         400.0,   // max glue  temp @ glue/carbon
                         350.0,   // max carbon temp @ carbon/external
                         simDuration,
                         lL,
                         matProps,
                         theta
                     );
                 } else {
                     appLog += "⚠️ Warning: Cannot optimize TPS for slice " + std::to_string(slice + 1) + " due to invalid stack.\n";
                 }
             } catch (const std::exception& opt_err) {
                 appLog += "❌ Error during TPS optimization for slice " + std::to_string(slice+1) + ": " + std::string(opt_err.what()) + "\n";
             }

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
                 solver = currentSolver; // Store the solver state of the *last* slice globally for visualization
                std::ofstream outFile("final_temperature.csv");
                 if (outFile) {
                     outFile << "x,Temperature\n"; // Add header
                     for (size_t i = 0; i < Tdist.size(); ++i) {
                         outFile << stack.xGrid[i] << "," << Tdist[i] << "\n";
                     }
                     outFile.close();
                 } else {
                     appLog += "❌ Error: Could not open final_temperature.csv for writing.\n";
                 }
            }
        } // End slice loop

        // Clear processing status when done
        currentProcessingStatus.clear();

        // Close output files
        summaryOut.close();
        detailsOut.close();

        appLog += "✅ Simulation completed!\n";
        appLog += "Processed " + std::to_string(nSlices) + " slices.\n";
        appLog += "Total solver time: " + std::to_string(totalSolveMs) + " ms\n";
        appLog += "Output Files:\n";
        appLog += "- final_temperature.csv: Temperature distribution (last slice)\n";
        appLog += "- " + std::string(outputFile) + ": Summary results\n";
        appLog += "- stack_details.csv: Detailed layer information\n";
        simulationCompleted = true; // Mark as completed
        triggerSimulation = false;  // Reset trigger

    } catch (const std::exception& e) {
        appLog += "❌ Exception in runSimulationLogic: ";
        appLog += e.what();
        appLog += "\n";
        currentProcessingStatus.clear();
        triggerSimulation = false; // Ensure trigger is reset on exception
        simulationCompleted = false;
    } catch (...) {
         appLog += "❌ Unknown exception occurred in runSimulationLogic.\n";
         currentProcessingStatus.clear();
         triggerSimulation = false;
         simulationCompleted = false;
    }
}

// ---- GUI Simulation Controls ----
void renderSimulationControls() {
    // This window will be explicitly sized and positioned in the main loop
    ImGui::Begin("HeatStack Simulation");

    if (ImGui::InputText("Mesh Path", meshPath, sizeof(meshPath))) {
        meshLoadedForVis = false; // Reset vis flag if path changes
        simulationCompleted = false; // Results are invalid if mesh changes
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse Mesh")) {
        const char* filter[] = { "*.obj", "*.csv" };
        const char* file = tinyfd_openFileDialog("Select Mesh", "", 2, filter, "OBJ or CSV files", 0);
        if (file) {
            strncpy_s(meshPath, sizeof(meshPath), file, _TRUNCATE);
            meshLoadedForVis = false; // Reset vis flag
            simulationCompleted = false; // Results are invalid
        }
    }

    ImGui::InputText("Initial Temp (.csv)", initTempPath, sizeof(initTempPath));
    ImGui::SameLine();
    if (ImGui::Button("Browse Temp")) {
        const char* filter[] = { "*.csv" };
        const char* file = tinyfd_openFileDialog("Select Initial Temp CSV", "", 1, filter, "CSV files", 0);
        if (file) strncpy_s(initTempPath, sizeof(initTempPath), file, _TRUNCATE);
    }

    ImGui::InputFloat("Duration (s)", &simDuration, 0.1f, 1.0f, "%.1f");
    ImGui::InputFloat("Time Step (s)", &timeStep, 0.01f, 0.1f, "%.3f");
    ImGui::InputInt("Number of Slices", &nSlices);
    ImGui::InputInt("Points Per Layer", &pointsPerLayer);
    ImGui::Checkbox("Use Adaptive Time Step", &useAdaptiveTimeStep);
    ImGui::InputFloat("Theta Parameter", &theta, 0.05f, 0.1f, "%.2f");
    ImGui::InputText("Output File", outputFile, sizeof(outputFile));

    // Clamp inputs to reasonable values
    if (simDuration <= 0) simDuration = 0.1f;
    if (timeStep <= 0) timeStep = 0.001f;
    if (nSlices < 1) nSlices = 1;
    if (pointsPerLayer < 2) pointsPerLayer = 2;
    theta = std::clamp(theta, 0.0f, 1.0f); // Clamp theta [0, 1]

    if (triggerSimulation) {
        ImGui::BeginDisabled(true); // Disable button while running
        ImGui::Button("Running...");
        ImGui::EndDisabled();
    } else {
       if (ImGui::Button("Run Simulation")) {
            triggerSimulation = true; // This will trigger the logic in the main loop
            simulationCompleted = false;
            progress = 0.0f;
            appLog.clear();
            currentProcessingStatus = "Starting simulation...";
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset All")) {
        resetSimulation();
    }

    ImGui::Separator();
    ImGui::ProgressBar(progress, ImVec2(-1, 0));
    if (!currentProcessingStatus.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", currentProcessingStatus.c_str());
    }

    // Log window
    ImGui::Separator();
    ImGui::Text("Log:");
    ImGui::BeginChild("LogScrollingRegion", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 8), true, ImGuiWindowFlags_HorizontalScrollbar); // Limit height
    ImGui::TextUnformatted(appLog.c_str());
     if(ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) // Auto-scroll
           ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::End(); // End Simulation Controls Window
}

// Helper function to draw coordinate axes
void drawCoordinateAxes() {
    // Make axes smaller and relative to mesh size? For now, fixed size.
    float axisLength = 0.5f; // Adjust as needed
    glLineWidth(2.0f);
    glBegin(GL_LINES);
        // X axis (red) - horizontal
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(axisLength, 0.0f, 0.0f);
        
        // Y axis (blue) - depth
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, axisLength, 0.0f);
        
        // Z axis (green) - vertical (this is the axis used for stacks)
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, axisLength);
    glEnd();
    glLineWidth(1.0f);
}

// Helper function to draw vertical lines at slice positions
void drawSliceLines(const MeshHandler& mesh) {
    if (!meshLoadedForVis || nSlices <= 1) return;

    // Calculate min and max bounds of the mesh directly from MeshHandler
    double xMin = mesh.getMinX();
    double xMax = mesh.getMaxX();
    double yMin = mesh.getMinY();
    double yMax = mesh.getMaxY();
    double zMin = mesh.getMinZ();
    double zMax = mesh.getMaxZ();
    double height = zMax - zMin;
    
    if (height <= 0) height = 1.0; // Avoid division by zero

    // Extend boundaries a bit for better visibility
    double xExtend = (xMax - xMin) * 0.05;
    double yExtend = (yMax - yMin) * 0.05;
    xMin -= xExtend;
    xMax += xExtend;
    yMin -= yExtend;
    yMax += yExtend;

    // Calculate slice positions along the Z-axis (toe to head)
    std::vector<double> sliceZPositions;
    for (int slice = 0; slice < nSlices; ++slice) {
        // Ensure we cover the ENTIRE range from zMin to zMax inclusively
        double z = zMin + ((nSlices > 1) ? (double(slice) / (nSlices - 1)) * height : height / 2.0);
        sliceZPositions.push_back(z);
    }

    // Log slice positions and bounds for debugging
    appLog += "Drawing slice lines:\n";
    appLog += "Mesh bounds: X=[" + std::to_string(xMin) + ", " + std::to_string(xMax) + "], "
            + "Y=[" + std::to_string(yMin) + ", " + std::to_string(yMax) + "], "
            + "Z=[" + std::to_string(zMin) + ", " + std::to_string(zMax) + "]\n";
    
    for (size_t i = 0; i < sliceZPositions.size(); ++i) {
        double z = sliceZPositions[i];
        double lL = (height > 0) ? ((z - zMin) / height) : 0.5;
        appLog += "Slice " + std::to_string(i + 1) + ": Z=" + std::to_string(z) +
                 ", l/L=" + std::to_string(lL) + "\n";
    }

    // Save the current depth testing state
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST); // Disable depth testing so lines are visible

    // Disable lighting for lines
    glDisable(GL_LIGHTING);

    // Set line properties
    glLineWidth(2.0f); // Increase line width for visibility
    glColor3f(0.0f, 1.0f, 1.0f); // Cyan color for better contrast

    glBegin(GL_LINES);
    for (double z : sliceZPositions) {
        // In the unrotated coordinate system, Z is already vertical
        // Draw horizontal grid lines at each slice height
        
        // Draw lines along X axis at specific Z height
        glVertex3d(xMin, yMin, z);
        glVertex3d(xMax, yMin, z);
        glVertex3d(xMin, yMax, z);
        glVertex3d(xMax, yMax, z);
        
        // Draw lines along Y axis at specific Z height
        glVertex3d(xMin, yMin, z);
        glVertex3d(xMin, yMax, z);
        glVertex3d(xMax, yMin, z);
        glVertex3d(xMax, yMax, z);
    }
    glEnd();

    // Reset OpenGL state
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST); // Restore depth testing state
    }
}

// Helper function to draw the mesh with temperature colors (only if simulation completed)
void drawMeshWithTemperatures(const MeshHandler& mesh, const HeatEquationSolver& completedSolver) {
    const auto& vertices = mesh.getVertices();
    const auto& faces = mesh.getFaces();
    const auto& Tdist = completedSolver.getTemperatureDistribution(); // Use the completed solver results

    if (vertices.empty() || faces.empty() || Tdist.empty()) return;

    // Calculate exact temperature range from temperature distribution
    double minTemp = *std::min_element(Tdist.begin(), Tdist.end());
    double maxTemp = *std::max_element(Tdist.begin(), Tdist.end());
    
    double tempRange = maxTemp - minTemp;
    if (tempRange <= 0) tempRange = 1.0; // Avoid division by zero

    // Calculate min and max Z for mapping
    double zMin = mesh.getMinZ();
    double zMax = mesh.getMaxZ();
    double height = zMax - zMin;
    if (height <= 0) height = 1.0; // Avoid division by zero

    // Log temperature range for debugging
    appLog += "Temperature visualization range: " + std::to_string(minTemp) + "K to " + std::to_string(maxTemp) + "K\n";
    appLog += "Mesh Z range: " + std::to_string(zMin) + " to " + std::to_string(zMax) + "\n";

    glBegin(GL_TRIANGLES);
    for (const auto& face : faces) {
        // Compute average face normal for flat shading (simple lighting)
        const auto& v1 = vertices[face[0]];
        const auto& v2 = vertices[face[1]];
        const auto& v3 = vertices[face[2]];

        double nx = (v2[1] - v1[1]) * (v3[2] - v1[2]) - (v2[2] - v1[2]) * (v3[1] - v1[1]);
        double ny = (v2[2] - v1[2]) * (v3[0] - v1[0]) - (v2[0] - v1[0]) * (v3[2] - v1[2]);
        double nz = (v2[0] - v1[0]) * (v3[1] - v1[1]) - (v2[1] - v1[1]) * (v3[0] - v1[0]);
        double len = sqrt(nx*nx + ny*ny + nz*nz);
        if (len > 0) { nx /= len; ny /= len; nz /= len; }
        glNormal3d(nx, ny, nz); // Set normal per face

        // Calculate average Z position of this face
        double avgZ = (v1[2] + v2[2] + v3[2]) / 3.0;
        
        // Map Z position to normalized position in distribution array
        double normalizedPos = (avgZ - zMin) / height;
        normalizedPos = std::clamp(normalizedPos, 0.0, 1.0);
        
        // Calculate temperature at this position by interpolating from distribution
        int index = static_cast<int>(normalizedPos * (Tdist.size() - 1));
        int nextIndex = std::min(index + 1, static_cast<int>(Tdist.size() - 1));
        double fraction = normalizedPos * (Tdist.size() - 1) - index;
        
        double temp;
        if (index == nextIndex) {
            temp = Tdist[index];
        } else {
            temp = Tdist[index] * (1 - fraction) + Tdist[nextIndex] * fraction;
        }
        
        // Convert temperature to color (red=hot to blue=cold)
        float t = static_cast<float>((temp - minTemp) / tempRange);
        t = std::clamp(t, 0.0f, 1.0f); // Clamp [0, 1]
        
        // Apply color to all vertices of this face
        glColor3f(t, 0.0f, 1.0f - t);  // Red to Blue gradient

        // Draw all vertices
        for (int i = 0; i < 3; ++i) {
            int vertexIdx = face[i];
            const auto& vertex = vertices[vertexIdx];
            double vertexDouble[3] = {static_cast<double>(vertex[0]), 
                                     static_cast<double>(vertex[1]), 
                                     static_cast<double>(vertex[2])};
            glVertex3dv(vertexDouble);
        }
    }
    glEnd();
}

// Helper function to draw the mesh with default material
void drawMeshDefault(const MeshHandler& mesh) {
    const auto& vertices = mesh.getVertices();
    const auto& faces = mesh.getFaces();

     if (vertices.empty() || faces.empty()) return;

    // Set a default material color (e.g., grey)
    glColor3f(0.7f, 0.7f, 0.7f);

    // Use basic lighting materials
    GLfloat material_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
    GLfloat material_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat material_specular[] = { 0.2f, 0.2f, 0.2f, 1.0f }; // Less shiny
    GLfloat material_shininess = 10.0f; // Lower shininess

    glMaterialfv(GL_FRONT, GL_AMBIENT, material_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, material_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, material_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, material_shininess);

    glBegin(GL_TRIANGLES);
    for (const auto& face : faces) {
         // Calculate face normal for proper lighting (flat shading)
         const auto& v1 = vertices[face[0]];
         const auto& v2 = vertices[face[1]];
         const auto& v3 = vertices[face[2]];

         double nx = (v2[1] - v1[1]) * (v3[2] - v1[2]) - (v2[2] - v1[2]) * (v3[1] - v1[1]);
         double ny = (v2[2] - v1[2]) * (v3[0] - v1[0]) - (v2[0] - v1[0]) * (v3[2] - v1[2]);
         double nz = (v2[0] - v1[0]) * (v3[1] - v1[1]) - (v2[1] - v1[1]) * (v3[0] - v1[0]);
         double len = sqrt(nx*nx + ny*ny + nz*nz);
         if (len > 0) { nx /= len; ny /= len; nz /= len; }

         glNormal3d(nx, ny, nz); // Set normal once per face

        for (int i = 0; i < 3; ++i) {
            const auto& vertex = vertices[face[i]];
            // Convert float vector to double for glVertex3dv
            double vertexDouble[3] = {static_cast<double>(vertex[0]), 
                                    static_cast<double>(vertex[1]), 
                                    static_cast<double>(vertex[2])};
            glVertex3dv(vertexDouble);
        }
    }
    glEnd();
}

// Helper function to draw mesh with TPU thickness visualization
void drawMeshWithThickness(const MeshHandler& mesh, const HeatEquationSolver& solver) {
    const auto& vertices = mesh.getVertices();
    const auto& faces = mesh.getFaces();

    if (vertices.empty() || faces.empty()) return;

    // Calculate min and max Z for slicing
    double zMin = mesh.getMinZ();
    double zMax = mesh.getMaxZ();
    double height = zMax - zMin;
    if (height <= 0) height = 1.0; // Avoid division by zero

    // Load TPS thickness data from summary file if available
    struct SliceData {
        int sliceNumber;  // Add slice number for tracking
        double lL;
        double tpsThickness;
    };
    std::vector<SliceData> sliceData;
    
    // Try to load from summary_output.csv first (contains optimal TPS thickness)
    std::ifstream summaryFile("summary_output.csv");
    if (summaryFile.is_open()) {
        std::string line;
        // Skip header
        std::getline(summaryFile, line);
        
        while (std::getline(summaryFile, line)) {
            std::stringstream ss(line);
            std::string token;
            
            // Read slice number
            std::getline(ss, token, ',');
            int sliceNum = std::stoi(token);
            
            // Read l/L
            std::getline(ss, token, ',');
            double lL = std::stod(token);
            
            // Skip method
            std::getline(ss, token, ',');
            
            // Skip final steel temperature
            std::getline(ss, token, ',');
            
            // Read TPS thickness
            std::getline(ss, token, ',');
            double tpsThickness = std::stod(token);
            
            // Ensure we have a positive value (sometimes optimization returns negative values)
            if (tpsThickness > 0) {
                sliceData.push_back({sliceNum, lL, tpsThickness});
            }
        }
        summaryFile.close();
    }

    // If summary_output.csv didn't work, try stack_details.csv (contains initial TPS thickness)
    if (sliceData.empty()) {
        std::ifstream detailsFile("stack_details.csv");
        if (detailsFile.is_open()) {
            std::string line;
            // Skip header
            std::getline(detailsFile, line);
            
            while (std::getline(detailsFile, line)) {
                std::stringstream ss(line);
                std::string token;
                
                // Read slice number
                std::getline(ss, token, ',');
                int sliceNum = std::stoi(token);
                
                // Read l/L
                std::getline(ss, token, ',');
                double lL = std::stod(token);
                
                // Read TPS thickness
                std::getline(ss, token, ',');
                double tpsThickness = std::stod(token);
                
                sliceData.push_back({sliceNum, lL, tpsThickness});
            }
            detailsFile.close();
        }
    }

    // If no data was loaded from CSV files, log an error and use a default thickness
    if (sliceData.empty()) {
        appLog += "❌ Error: Could not load TPS thickness data from summary_output.csv or stack_details.csv.\n";
        appLog += "Using default thickness values for visualization.\n";
        
        // Default thickness values as a fallback - ensure we cover the ENTIRE mesh
        for (int i = 0; i < nSlices; i++) {
            double lL = (nSlices > 1) ? static_cast<double>(i) / (nSlices - 1) : 0.5;
            sliceData.push_back({i + 1, lL, 0.001}); // Default thickness of 0.001m
        }
    }

    // Find min/max thickness for color mapping directly from sliceData
    double minThickness = DBL_MAX;
    double maxThickness = -DBL_MAX;
    
    for (const auto& slice : sliceData) {
        if (slice.tpsThickness < minThickness) minThickness = slice.tpsThickness;
        if (slice.tpsThickness > maxThickness) maxThickness = slice.tpsThickness;
    }
    
    if (minThickness == DBL_MAX) minThickness = 0.0;
    double thicknessRange = maxThickness - minThickness;
    if (thicknessRange <= 0) thicknessRange = 1.0; // Avoid division by zero
    
    // Log thickness range for debugging
    appLog += "Using TPS thickness values from simulation data.\n";
    appLog += "Thickness range: " + std::to_string(minThickness) + " to " + std::to_string(maxThickness) + "m\n";
    
    // Sort slice data by l/L value (from feet to head)
    std::sort(sliceData.begin(), sliceData.end(), 
             [](const SliceData& a, const SliceData& b) { return a.lL < b.lL; });
    
    // Calculate slice boundaries in Z-space - ensure COMPLETE coverage
    std::vector<double> sliceBoundaries;
    if (nSlices > 1) {
        // Use nSlices+1 boundaries to define nSlices regions
        for (int i = 0; i <= nSlices; i++) {
            double boundary = zMin + (static_cast<double>(i) / nSlices) * height;
            sliceBoundaries.push_back(boundary);
        }
    } else {
        // If only one slice, use the entire Z range
        sliceBoundaries.push_back(zMin);
        sliceBoundaries.push_back(zMax);
    }

    // Log slice boundaries for debugging
    appLog += "Slice boundaries (Z-axis): ";
    for (double boundary : sliceBoundaries) {
        appLog += std::to_string(boundary) + " ";
    }
    appLog += "\n";

    // Draw mesh with color based on TPS thickness
    glBegin(GL_TRIANGLES);
    for (const auto& face : faces) {
        // Compute face normal for lighting
        const auto& v1 = vertices[face[0]];
        const auto& v2 = vertices[face[1]];
        const auto& v3 = vertices[face[2]];

        double nx = (v2[1] - v1[1]) * (v3[2] - v1[2]) - (v2[2] - v1[2]) * (v3[1] - v1[1]);
        double ny = (v2[2] - v1[2]) * (v3[0] - v1[0]) - (v2[0] - v1[0]) * (v3[2] - v1[2]);
        double nz = (v2[0] - v1[0]) * (v3[1] - v1[1]) - (v2[1] - v1[1]) * (v3[0] - v1[0]);
        double len = sqrt(nx*nx + ny*ny + nz*nz);
        if (len > 0) { nx /= len; ny /= len; nz /= len; }
        glNormal3d(nx, ny, nz);

        // Determine the average Z position of this face to assign it to a specific slice
        double avgZ = (v1[2] + v2[2] + v3[2]) / 3.0;
        
        // Find which slice this face belongs to
        int sliceIndex = 0;
        for (size_t i = 0; i < sliceBoundaries.size() - 1; i++) {
            if (avgZ >= sliceBoundaries[i] && avgZ <= sliceBoundaries[i+1]) {
                sliceIndex = i;
                break;
            }
        }
        
        // Map slice index to l/L value - use evenly distributed l/L values
        double sliceLL = (nSlices > 1) ? 
                      static_cast<double>(sliceIndex) / (nSlices - 1) : 
                      0.5;
        
        // Find the corresponding slice data
        double thickness = 0.001; // Default
        for (const auto& slice : sliceData) {
            // Use approximate comparison for floating point
            if (fabs(slice.lL - sliceLL) < 0.01 || 
                (slice.sliceNumber == sliceIndex + 1)) {
                thickness = slice.tpsThickness;
                break;
            }
        }
        
        // Map thickness to color (yellow to green)
        float t = static_cast<float>((thickness - minThickness) / thicknessRange);
        t = std::clamp(t, 0.0f, 1.0f);
        
        // Yellow (thick) to Green (thin) gradient
        glColor3f(1.0f - t, 1.0f, 0.0f);
        
        // Draw all vertices with the same color for this face
        for (int i = 0; i < 3; ++i) {
            int vertexIdx = face[i];
            const auto& vertex = vertices[vertexIdx];
            double vertexDouble[3] = {static_cast<double>(vertex[0]), 
                                     static_cast<double>(vertex[1]), 
                                     static_cast<double>(vertex[2])};
            glVertex3dv(vertexDouble);
        }
    }
    glEnd();
}

// Helper function to draw a color scale for temperature or thickness visualization
void drawColorScale(int x, int y, int width, int height, bool isTemperature) {
    // Draw background
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::Begin("##colorscale", nullptr, 
                ImGuiWindowFlags_NoTitleBar | 
                ImGuiWindowFlags_NoResize | 
                ImGuiWindowFlags_NoMove | 
                ImGuiWindowFlags_NoScrollbar);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Get window position for absolute coordinates
    ImVec2 pos = ImGui::GetCursorScreenPos();
    
    // Draw gradient
    const int barHeight = height - 60; // Leave room for text
    const int barWidth = 30;
    const int startX = pos.x + 10;
    const int startY = pos.y + 10;
    
    // Draw gradient bar
    for (int i = 0; i < barHeight; i++) {
        float t = 1.0f - (float)i / barHeight; // 1.0 at top, 0.0 at bottom
        ImVec4 color;
        if (isTemperature) {
            // Red to Blue gradient for temperature
            color = ImVec4(t, 0.0f, 1.0f - t, 1.0f);
        } else {
            // Yellow to Green gradient for thickness (matching the mesh visualization)
            color = ImVec4(1.0f - t, 1.0f, 0.0f, 1.0f);
        }
        ImU32 col32 = ImGui::ColorConvertFloat4ToU32(color);
        draw_list->AddRectFilled(
            ImVec2(startX, startY + i), 
            ImVec2(startX + barWidth, startY + i + 1), 
            col32);
    }
    
    // Draw labels
    draw_list->AddRect(
        ImVec2(startX, startY), 
        ImVec2(startX + barWidth, startY + barHeight), 
        IM_COL32_WHITE);
    
    if (isTemperature) {
        // Get min/max temperature from solver if available
        double minTemp = 273.0; // Default min (0°C)
        double maxTemp = solver.getTemperatureDistribution().empty() ? 
                        800.0 : solver.getTemperatureDistribution().back(); // Default max
        
        // Draw temperature labels
        std::string maxLabel = "Max: " + std::to_string((int)maxTemp) + "K";
        std::string minLabel = "Min: " + std::to_string((int)minTemp) + "K";
        draw_list->AddText(ImVec2(startX + barWidth + 5, startY), IM_COL32_WHITE, maxLabel.c_str());
        draw_list->AddText(ImVec2(startX + barWidth + 5, startY + barHeight - 15), IM_COL32_WHITE, minLabel.c_str());
        
        // Title
        ImGui::SetCursorPos(ImVec2(10, barHeight + 20));
        ImGui::Text("Temperature (K)");
    } else {
        // Get min/max TPU thickness from the simulation data
        double minThickness = DBL_MAX;
        double maxThickness = -DBL_MAX;

        std::ifstream summaryFile("summary_output.csv");
        if (summaryFile.is_open()) {
            std::string line;
            std::getline(summaryFile, line); // Skip header
            while (std::getline(summaryFile, line)) {
                std::stringstream ss(line);
                std::string token;
                std::getline(ss, token, ','); // slice
                std::getline(ss, token, ','); // l/L
                std::getline(ss, token, ','); // method
                std::getline(ss, token, ','); // finalSteelTemp
                std::getline(ss, token, ','); // TPS_thickness
                double thickness = std::stod(token);
                if (thickness > 0) {
                    if (thickness < minThickness) minThickness = thickness;
                    if (thickness > maxThickness) maxThickness = thickness;
                }
            }
            summaryFile.close();
        } else {
            std::ifstream detailsFile("stack_details.csv");
            if (detailsFile.is_open()) {
                std::string line;
                std::getline(detailsFile, line); // Skip header
                while (std::getline(detailsFile, line)) {
                    std::stringstream ss(line);
                    std::string token;
                    std::getline(ss, token, ','); // slice
                    std::getline(ss, token, ','); // l/L
                    std::getline(ss, token, ','); // TPS_thickness
                    double thickness = std::stod(token);
                    if (thickness < minThickness) minThickness = thickness;
                    if (thickness > maxThickness) maxThickness = thickness;
                }
                detailsFile.close();
            }
        }

        if (minThickness == DBL_MAX) minThickness = 0.0;
        if (maxThickness == -DBL_MAX) maxThickness = 0.001; // Default max if no data

        // Draw thickness labels
        std::string maxLabel = "Max: " + std::to_string(maxThickness).substr(0, 8) + "m";
        std::string minLabel = "Min: " + std::to_string(minThickness).substr(0, 8) + "m";
        draw_list->AddText(ImVec2(startX + barWidth + 5, startY), IM_COL32_WHITE, maxLabel.c_str());
        draw_list->AddText(ImVec2(startX + barWidth + 5, startY + barHeight - 15), IM_COL32_WHITE, minLabel.c_str());
        
        // Title
        ImGui::SetCursorPos(ImVec2(10, barHeight + 20));
        ImGui::Text("TPU Thickness (m)");
    }
    
    ImGui::End();
}

// ---- Visualization Controls Window ----
void renderVisualizationControls() {
    // This window will also be explicitly positioned and sized in the main loop
    ImGui::Begin("Visualization Controls");

    ImGui::Checkbox("Enable Camera Movement", &cameraMovementEnabled);
    ImGui::Checkbox("Wireframe Mode", &renderWireframe);
    ImGui::Checkbox("Show Color Scale", &showColorScale);
    ImGui::Checkbox("Show Slice Lines", &showSliceLines);
    
    if (ImGui::Button("Reset View")) {
        // Reset camera to default position
        camDistance = 5.0f;
        camAzimuth = 45.0f;
        camElevation = 30.0f;
        camTargetX = 0.0f;
        camTargetY = 0.0f;
        camTargetZ = 0.0f;
    }
    
    ImGui::Separator();
    ImGui::Text("Visualization Type:");
    if (ImGui::RadioButton("Temperature", currentVisMode == TEMPERATURE_VIS)) {
        currentVisMode = TEMPERATURE_VIS;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("TPU Thickness", currentVisMode == THICKNESS_VIS)) {
        currentVisMode = THICKNESS_VIS;
    }

    ImGui::Separator();
    ImGui::Text("Camera:");
    // Display Azimuth/Elevation/Distance instead of Position/Rotation
    ImGui::Text("Azimuth: %.1f, Elevation: %.1f", camAzimuth, camElevation);
    ImGui::Text("Distance: %.1f", camDistance);
    ImGui::Text("Target: (%.1f, %.1f, %.1f)", camTargetX, camTargetY, camTargetZ);

    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::Text("- Left Mouse Drag: Orbit Camera");
    ImGui::Text("- Right Mouse Drag: Pan Camera");
    ImGui::Text("- Mouse Wheel: Zoom");

    ImGui::End();
}

// ---- Updated Visualization Rendering Function ----
void renderVisualization(int vx, int vy, int vw, int vh, bool isHovered) {
    // Set OpenGL viewport and scissor test to constrain drawing
    glViewport(vx, vy, vw, vh);
    glScissor(vx, vy, vw, vh);
    glEnable(GL_SCISSOR_TEST);

    // --- Camera Control Logic ---
    if (cameraMovementEnabled && isHovered) {
        ImGuiIO& io = ImGui::GetIO();

        // Zoom: Mouse Wheel
        if (io.MouseWheel != 0.0f) {
            camDistance -= io.MouseWheel * zoomSensitivity;
            camDistance = std::clamp(camDistance, 0.1f, 50.0f); // Clamp distance
        }

        // Orbit: Left Mouse Drag (rotates around the target point)
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            // Reset delta start pos for continuous dragging
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);

            camAzimuth += dragDelta.x * mouseSensitivity;
            camElevation += dragDelta.y * mouseSensitivity;

            // Clamp elevation to avoid flipping
            camElevation = std::clamp(camElevation, -89.9f, 89.9f);
        }
        
        // Pan: Right Mouse Drag (moves the target point without changing distance)
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
            // Reset delta start pos for continuous dragging
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
            
            // Apply panning movement
            float panSpeed = 0.01f * camDistance; // Scale by distance for consistent feel
            camTargetX += dragDelta.x * panSpeed;
            camTargetZ += dragDelta.y * panSpeed;
        }
    }

    // --- OpenGL Setup for this Viewport ---
    // Set up projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (vh > 0) ? ((float)vw / (float)vh) : 1.0f;
    perspectiveGL(45.0f, aspect, 0.1f, 100.0f); // Use helper

    // Set up modelview matrix (camera)
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Calculate camera position using spherical coordinates (orbit)
    // Using Z as the up axis by default - matching simulation coordinates
    float camX = camTargetX + camDistance * cos(camElevation * M_PI / 180.0f) * sin(camAzimuth * M_PI / 180.0f);
    float camY = camTargetY + camDistance * cos(camElevation * M_PI / 180.0f) * cos(camAzimuth * M_PI / 180.0f);
    float camZ = camTargetZ + camDistance * sin(camElevation * M_PI / 180.0f);

    // Use lookAtGL with Z as up vector to match simulation coordinates
    lookAtGL(camX, camY, camZ,             // Eye position
             camTargetX, camTargetY, camTargetZ, // Target position
             0.0f, 0.0f, 1.0f);           // Up vector (Z is up)

    // Clear background specifically for this viewport
    glClearColor(0.15f, 0.15f, 0.17f, 1.0f); // Slightly different background for vis
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth testing, lighting, etc.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Setup basic lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat light_position[] = { 5.0f, 5.0f, 5.0f, 1.0f }; // Adjusted light position
    GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glEnable(GL_COLOR_MATERIAL); // Allow glColor to affect material diffuse
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH); // Use smooth shading

    // --- Drawing ---
    glPushMatrix();
    
    // No need to rotate the mesh - keep original coordinates where Z is already vertical

    // Add scaling to handle size differences
    float meshScaleFactor = 1.0f;  // Adjust this if mesh is too large or small
    glScalef(meshScaleFactor, meshScaleFactor, meshScaleFactor);
    
    // Center the mesh automatically
    if (meshLoadedForVis) {
        // Get the vertices
        const auto& vertices = meshHandler.getVertices();
        
        // Calculate min/max for all axes using the MeshHandler's methods
        float minX = meshHandler.getMinX();
        float maxX = meshHandler.getMaxX();
        float minY = meshHandler.getMinY();
        float maxY = meshHandler.getMaxY();
        float minZ = meshHandler.getMinZ();
        float maxZ = meshHandler.getMaxZ();
        
        // Calculate center - keeping the original coordinate system intact
        double centerX = 0.5 * (minX + maxX);
        double centerY = 0.5 * (minY + maxY);
        double centerZ = 0.5 * (minZ + maxZ);
        
        // Center the mesh - simply translate to center
        glTranslatef(-centerX, -centerY, -centerZ);
    }

    // Draw coordinate axes
    glDisable(GL_LIGHTING); // Draw axes without lighting
    drawCoordinateAxes();
    glEnable(GL_LIGHTING); // Re-enable lighting

    // Set wireframe mode if enabled
    if (renderWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Load mesh for visualization if path is set and not yet loaded
    if (!meshLoadedForVis && strlen(meshPath) > 0) {
        if (meshHandler.loadMesh(meshPath)) {
            meshLoadedForVis = true;
            appLog += "✅ Mesh loaded successfully for visualization.\n";
        } else {
            appLog += "❌ Error loading mesh for visualization: " + std::string(meshPath) + "\n";
            meshPath[0] = '\0'; // Clear invalid path
        }
    }

    // Draw the mesh based on the selected visualization mode
    if (meshLoadedForVis) {
        if (currentVisMode == TEMPERATURE_VIS && simulationCompleted && !solver.getTemperatureDistribution().empty()) {
            drawMeshWithTemperatures(meshHandler, solver); // Temperature visualization
        } else if (currentVisMode == THICKNESS_VIS) {
            drawMeshWithThickness(meshHandler, solver); // TPU thickness visualization
        } else {
            drawMeshDefault(meshHandler); // Default material
        }
    }

    // Draw slice lines if enabled
    if (showSliceLines && meshLoadedForVis) {
        drawSliceLines(meshHandler);
    }

    glPopMatrix();

    // Draw color scale if enabled
    if (showColorScale) {
        int scaleWidth = 150;
        int scaleHeight = 300;
        int scaleX = vx + vw - scaleWidth - 10; // Position near the right edge
        int scaleY = vy + 10; // Position near the top
        drawColorScale(scaleX, scaleY, scaleWidth, scaleHeight, currentVisMode == TEMPERATURE_VIS);
    }

    // --- Cleanup OpenGL state for this viewport ---
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Reset polygon mode
    glDisable(GL_SCISSOR_TEST); // Disable scissor test IMPORTANT!
    glDisable(GL_LIGHTING); // Disable lighting
    glDisable(GL_DEPTH_TEST); // Disable depth test (ImGui prefers it off)
}

int main(int argc, char** argv) {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Create Window
    GLFWwindow* window = glfwCreateWindow(1600, 900, "HeatStack Simulator", NULL, NULL);
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

    // --- Simulation Thread ---
    std::future<void> simulationFuture;
    bool isSimThreadRunning = false;

    // --- Main Loop ---
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Handle simulation triggering and thread management
        if (triggerSimulation && !isSimThreadRunning) {
            // Start simulation in a separate thread using std::async
            simulationFuture = std::async(std::launch::async, runSimulationLogic);
            isSimThreadRunning = true;
            triggerSimulation = false; // Consume the trigger
        }

        // Check if simulation thread finished
        if (isSimThreadRunning) {
            // Non-blocking check if thread is finished using future_status
            auto status = simulationFuture.wait_for(std::chrono::seconds(0));
            if (status == std::future_status::ready) {
                isSimThreadRunning = false;
                // SimulationCompleted flag should be set by runSimulationLogic
                if (!simulationCompleted) { // If logic finished but didn't set completed (e.g., due to error)
                    currentProcessingStatus = "Simulation finished with errors or was stopped.";
                }
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- Get Window Size ---
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // --- Define Layout ---
        // Simple split: Left ~40% for controls, Right ~60% for visualization
        float controlPanelWidth = display_w * 0.4f;
        int visX = static_cast<int>(controlPanelWidth);
        int visY = 0;
        int visW = display_w - visX;
        int visH = display_h;

        // --- Render Control Windows on the Left ---
        // Simulation Controls Window
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(controlPanelWidth, display_h * 0.65f), ImGuiCond_Always);
        renderSimulationControls();

        // Visualization Controls Window (below simulation controls)
        ImGui::SetNextWindowPos(ImVec2(0, display_h * 0.65f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(controlPanelWidth, display_h * 0.35f), ImGuiCond_Always);
        renderVisualizationControls();

        // --- Prepare for Visualization Rendering ---
        // Determine if the mouse is hovering over the visualization viewport *area*
        ImVec2 mousePos = ImGui::GetMousePos();
        bool isVisViewportHovered = (mousePos.x >= visX && mousePos.x < (visX + visW) &&
                                     mousePos.y >= visY && mousePos.y < (visY + visH));

        // Clear the entire window background (needed for ImGui)
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the visualization in its viewport
        renderVisualization(visX, visY, visW, visH, isVisViewportHovered);

        // --- Render ImGui ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // --- Cleanup ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}