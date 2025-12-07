#include "simulation.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "implot.h"

#include <cstring>

void Simulation::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_window = glfwCreateWindow(Constants::WIDTH, Constants::HEIGHT, "Simulation", nullptr, nullptr);
    glfwSetWindowSizeLimits(m_window, Constants::WIDTH, Constants::HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

void Simulation::addParticle(Particle &&particle)
{
    auto &particleRef = m_particles.emplace_back(std::move(particle));
    m_plotSelectedParticle = m_particles.begin();
}

void Simulation::removeParticle(std::vector<Particle>::iterator &it)
{
    if (it != m_particles.end())
    {
        (*it).cleanup();
        m_particles.erase(it);
        if (m_particles.size() > 0)
            m_plotSelectedParticle = m_particles.begin();
    }
}

void Simulation::run()
{
    initWindow();
    Input::setWindow(m_window);
    m_rendererHandle->setWindow(m_window);
    m_rendererHandle->setCamera(&m_camera);
    m_rendererHandle->init();
    glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwMakeContextCurrent(m_window);

    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImFontConfig config;
    config.OversampleH = 1;
    config.OversampleV = 1;
    std::filesystem::path arimoPath = Constants::FONTS_PATH / "arimo" / "Arimo-Regular.ttf";
    io.Fonts->AddFontFromFileTTF(arimoPath.string().c_str(), 16.0f, &config, io.Fonts->GetGlyphRangesGreek());
    io.Fonts->Build();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_CursorPosCallback(m_window, Input::mousePos.x, Input::mousePos.y);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    AxesModel axes(glm::vec3(0.0f));
    m_rendererHandle->addRenderables(&axes);

    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
        m_camera.update(static_cast<float>(m_rendererHandle->getDeltaTime()));
        m_rendererHandle->newFrame();
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImPlot::ShowDemoWindow();
        ImGui::ShowDemoWindow();

        if (not m_paused)
        {
            if (m_skipUpdate)
            {
                m_skipUpdate = false;
            }
            else
            {
                switch (m_mode)
                {
                case SimulationMode::Static:
                    updateStatic();
                    break;
                case SimulationMode::PrecalculatedAll:
                    updateAllPrecalc();
                    break;
                case SimulationMode::Precalculated20Ms:
                    update20MsPecalc();
                    break;
                case SimulationMode::Realtime:
                    updateRealTime();
                    break;
                }
            }
        }

        displayMainCtrlWindow();
        displayParticleListWindow();
        displayParticleAddWindow();
        displayPlotWindow();

        ImGui::Render();
        m_rendererHandle->recordImguiData(ImGui::GetDrawData());
    }

    m_rendererHandle->cleanup();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Simulation::launchParticleThreads()
{
    for (uint32_t i = 0; i < m_threads.size(); ++i)
    {
        uint32_t minIdx = i * static_cast<uint32_t>(m_particles.size()) / NUM_THREADS;
        uint32_t maxIdx = (i + 1) * static_cast<uint32_t>(m_particles.size()) / NUM_THREADS;
        m_threads[i] =
            std::jthread(&Simulation::calculateParticlePostionsThreaded, this, std::stop_token{}, minIdx, maxIdx);
    }
}

void Simulation::calculateParticlePostionsThreaded(std::stop_token stopToken, uint32_t minIdx, uint32_t maxIdx)
{
    while (m_elapsedTime <= m_simulationTime && not m_paused)
    {
        if (stopToken.stop_requested())
        {
            std::cout << "thread stop requested" << '\n';
        }
        for (uint32_t i = minIdx; i < maxIdx; ++i)
        {
            calculatePositionsForSingleParticle(&m_particles[i]);
        }
    }
}

void Simulation::calculatePositionsForSingleParticle(Particle *particle)
{
    uint32_t stepIdx = particle->getMaxStep() + 1;

    if (m_mutualMaxStep < (stepIdx - 1))
    {
        return;
    }

    if (particle->getBufferedStepCount() >= STEPS_BUFFERED_AT_ONCE)
    {
        return;
    }

    std::unique_lock<std::mutex> lk(particle->mutexData());

    Types::Vec3d force{0.0};
    for (auto &particleOther : m_particles)
    {
        if (particle->getId() != particleOther.getId())
        {
            force += particle->getCoulombForce(stepIdx - 1, particleOther);
        }
    }
    auto acc = force / particle->getMass();
    auto vel =
        particle->statesData()[stepIdx - 1].velocity + (particle->statesData()[stepIdx - 1].acceleration * m_timeStep);
    auto pos = particle->statesData()[stepIdx - 1].pos + (particle->statesData()[stepIdx - 1].velocity * m_timeStep);
    particle->pushState(stepIdx, force, acc, vel, pos);
}

void Simulation::calculateParticlePositions(bool all)
{
    uint32_t startingStep{0};
    if (not all)
    {
        startingStep = m_mutualMaxStep + 1;
        m_mutualMaxStep =
            std::min(m_mutualMaxStep + getStepsPer20ms(), static_cast<uint32_t>(m_simulationTime / m_timeStep));
    }
    else
    {
        startingStep = 1;
        m_mutualMaxStep = static_cast<uint32_t>(m_simulationTime / m_timeStep);
    }

    calculateSteps(startingStep);
}

void Simulation::rk4Step(uint32_t stepIdx, Particle &particle)
{
    Types::Vec3d k1r = m_timeStep * particle.statesData()[stepIdx - 1].velocity;
    Types::Vec3d k1v = m_timeStep * particle.statesData()[stepIdx - 1].acceleration;
    Types::Vec3d k2r = m_timeStep * (particle.statesData()[stepIdx - 1].velocity + k1v / 2.0);
    Types::Vec3d k2v = m_timeStep * calcForce(stepIdx - 1, particle, k1r / 2.0) / particle.getMass();
    Types::Vec3d k3r = m_timeStep * (particle.statesData()[stepIdx - 1].velocity + k2v / 2.0);
    Types::Vec3d k3v = m_timeStep * calcForce(stepIdx - 1, particle, k2r / 2.0) / particle.getMass();
    Types::Vec3d k4r = m_timeStep * (particle.statesData()[stepIdx - 1].velocity + k3v);
    Types::Vec3d k4v = m_timeStep * calcForce(stepIdx - 1, particle, k3r) / particle.getMass();
    auto pos = particle.statesData()[stepIdx - 1].pos + (k1r + 2.0 * k2r + 2.0 * k3r + k4r) / 6.0;
    auto vel = particle.statesData()[stepIdx - 1].velocity + (k1v + 2.0 * k2v + 2.0 * k3v + k4v) / 6.0;
    auto force = calcForcePosOverride(stepIdx - 1, particle, pos);
    auto acc = force / particle.getMass();
    particle.pushState(stepIdx, force, acc, vel, pos);
}

void Simulation::forwardEulerStep(uint32_t stepIdx, Particle &particle)
{
    auto pos = particle.statesData()[stepIdx - 1].pos + m_timeStep * particle.statesData()[stepIdx - 1].velocity;
    auto vel =
        particle.statesData()[stepIdx - 1].velocity + m_timeStep * particle.statesData()[stepIdx - 1].acceleration;
    auto force = calcForcePosOverride(stepIdx - 1, particle, pos);
    auto acc = force / particle.getMass();
    particle.pushState(stepIdx, force, acc, vel, pos);
}

void Simulation::leapfrogStep(uint32_t stepIdx, Particle &particle)
{
    auto pos = particle.statesData()[stepIdx - 1].pos + m_timeStep * particle.statesData()[stepIdx - 1].velocity +
               0.5 * m_timeStep * m_timeStep * particle.statesData()[stepIdx - 1].acceleration;
    auto force = calcForcePosOverride(stepIdx - 1, particle, pos);
    auto acc = force / particle.getMass();
    auto vel = particle.statesData()[stepIdx - 1].velocity +
               0.5 * m_timeStep * (particle.statesData()[stepIdx - 1].acceleration + acc);
    particle.pushState(stepIdx, force, acc, vel, pos);
}

void Simulation::calculateSteps(uint32_t startingStep)
{
    auto start = std::chrono::high_resolution_clock::now();
    for (uint32_t i = startingStep; i < m_mutualMaxStep + 1; ++i)
    {
        for (auto &particle : m_particles)
        {
            if (particle.isMovable())
            {
                switch (particle.getMethodMask())
                {
                case Types::OdeMethod::RK4:
                    rk4Step(i, particle);
                    break;
                case Types::OdeMethod::ForwardEuler:
                    forwardEulerStep(i, particle);
                    break;
                case Types::OdeMethod::Leapfrog:
                    leapfrogStep(i, particle);
                    break;
                }
            }
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    m_timeToCalculateAllParticlePos = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

Types::Vec3d Simulation::calcForce(uint32_t stateId, Particle &particle, Types::Vec3d distanceMod)
{
    Types::Vec3d force{0.0};
    const uint64_t particleId = particle.getId();
    const Types::OdeMethod particleMethod = particle.getMethodMask();
    for (auto &particleOther : m_particles)
    {
        if ((particleId != particleOther.getId()) && (particleMethod == particleOther.getMethodMask()))
        {
            force += particle.getCoulombForce(stateId, particleOther, distanceMod);
        }
    }
    return force;
}

Types::Vec3d Simulation::calcForcePosOverride(uint32_t stateId, Particle &particle, Types::Vec3d posOverride)
{
    Types::Vec3d force{0.0};
    const uint64_t particleId = particle.getId();
    const Types::OdeMethod particleMethod = particle.getMethodMask();
    for (auto &particleOther : m_particles)
    {
        if ((particleId != particleOther.getId()) && (particleMethod == particleOther.getMethodMask()))
        {
            force += particle.getCoulombForcePosOverwrite(stateId, particleOther, posOverride);
        }
    }
    return force;
}

void Simulation::updatePositionsThreaded()
{
    if (not m_paused && m_isHung) // Handle case when there was a lack of
                                  // particle step data in previous iteration
    {
        m_isHung = false;
        for (std::vector<Particle>::iterator it = m_hungIt; it != m_particles.end(); it++)
        {
            std::unique_lock<std::mutex> lk((*it).mutexData());
            if (not(*it).isMovable())
            {
                continue;
            }
            bool updateSuccess = (*it).updateFromPrecalcPos(static_cast<uint32_t>(m_elapsedTime / m_timeStep));
            if (not updateSuccess && (m_elapsedTime <= m_simulationTime))
            {
                m_hungIt = it;
                m_isHung = true;
                break;
            }
        }
    }
    else if (not m_paused && not m_isHung)
    {
        for (auto it = m_particles.begin(); it != m_particles.end(); it++) // Loop for precalculated simulation
        {
            std::unique_lock<std::mutex> lk((*it).mutexData());
            if (not(*it).isMovable())
            {
                continue;
            }
            bool updateSuccess = (*it).updateFromPrecalcPos(static_cast<uint32_t>(m_elapsedTime / m_timeStep));
            if (not updateSuccess && (m_elapsedTime <= m_simulationTime))
            {
                m_hungIt = it;
                m_isHung = true;
                break;
            }
        }
    }
}

bool Simulation::updatePositions()
{
    for (auto it = m_particles.begin(); it != m_particles.end(); it++)
    {
        if (not(*it).isMovable())
        {
            continue;
        }
        bool updateSuccess = (*it).updateFromPrecalcPos(static_cast<uint32_t>(m_maxUsedStep));
    }
    return true;
}

void Simulation::startSimulation()
{
    if (m_elapsedTime > 0.0)
    {
        restartSimulation();
    }
    if (m_particles.size() > 0)
    {
        for (auto &particle : m_particles)
        {
            if (particle.initialStateData())
            {
                particle.setAffectingForce(particle.getInitialState().affectingForce);
                particle.setAcceleration(particle.getInitialState().acceleration);
                particle.setVelocity(particle.getInitialState().velocity);
                particle.setPos(particle.getInitialState().pos);
                particle.update();
            }
            particle.clearStates();
            particle.pushState(0);
        }

        switch (m_mode)
        {
        case SimulationMode::Static:
            break;
        case SimulationMode::PrecalculatedAll:
            calculateParticlePositions(true);
            break;
        case SimulationMode::Precalculated20Ms:
            if (m_threadedCalculation)
            {
                launchParticleThreads();
            }
            break;
        case SimulationMode::Realtime:
            break;
        }
        m_paused = false;
    }
}

void Simulation::resetAll()
{
    m_skipUpdate = true;
    m_paused = true;
    m_mutualMaxStep = 0;
    m_elapsedTime = 0.0;

    if (m_threadedCalculation)
    {
        for (size_t i = 0; i < m_threads.size(); i++)
        {
            if (m_threads[i].joinable())
            {
                m_threads[i].join();
            }
        }
    }

    for (auto &particle : m_particles)
    {
        particle.cleanup();
    }
    m_particles.clear();
    Particle::resetId();
}

void Simulation::restartSimulation()
{
    m_skipUpdate = true;
    m_paused = true;
    m_mutualMaxStep = 0;
    m_elapsedTime = 0.0;

    if (m_threadedCalculation)
    {
        for (size_t i = 0; i < m_threads.size(); i++)
        {
            if (m_threads[i].joinable())
            {
                m_threads[i].join();
            }
        }
    }

    for (auto &particle : m_particles)
    {
        particle.resetTrail();
        particle.statesData().clear();
        particle.setMaxStep(0);
        particle.setAffectingForce(particle.getInitialState().affectingForce);
        particle.setAcceleration(particle.getInitialState().acceleration);
        particle.setVelocity(particle.getInitialState().velocity);
        particle.setPos(particle.getInitialState().pos);
        particle.update();
    }
}

void Simulation::updateStatic()
{
    for (auto &particle : m_particles)
    {
        Types::Vec3d force{0.0};
        const uint64_t particleId = particle.getId();
        for (auto &particle_other : m_particles)
        {
            if (particleId != particle_other.getId())
            {
                force += particle.getCoulombForce(0, particle_other);
            }
        }
        particle.setAffectingForce(force);
    }

    for (auto &particle : m_particles)
    {
        particle.update();
    }
}

void Simulation::updateAllPrecalc()
{
    m_maxUsedStep = m_elapsedTime / m_timeStep;
    updatePositions();
    m_elapsedTime = std::clamp(m_elapsedTime + m_rendererHandle->getDeltaTime(), 0.0, m_simulationTime);

    if (m_elapsedTime == m_simulationTime)
    {
        m_maxUsedStep = m_mutualMaxStep;
        updatePositions();
        m_paused = true;
    }
}

void Simulation::update20MsPecalc()
{
    m_maxUsedStep = m_elapsedTime / m_timeStep;

    if (m_maxUsedStep <= m_mutualMaxStep)
    {
        updatePositions();
        m_elapsedTime =
            std::clamp(m_elapsedTime + std::min(m_rendererHandle->getDeltaTime(), 0.02), 0.0, m_simulationTime);
        for (auto &particle : m_particles)
        {
            std::erase_if(particle.statesData(), [this](const auto &item) {
                auto const &[key, value] = item;
                return key < std::min(m_maxUsedStep - 2000, m_maxUsedStep);
            });
        }
    }
    calculateParticlePositions();

    if (m_elapsedTime == m_simulationTime)
    {
        m_maxUsedStep = m_mutualMaxStep;
        updatePositions();
        m_paused = true;
    }
}

void Simulation::updateRealTime()
{
    for (auto &particle : m_particles)
    {
        Types::Vec3d force{0.0};
        const uint64_t particleId = particle.getId();
        for (auto &particleOther : m_particles)
        {
            if (particleId != particleOther.getId())
            {
                force += particle.getCoulombForce(particleOther);
            }
        }
        particle.setAffectingForce(force);
        if (particle.isMovable())
        {
            particle.update(m_rendererHandle->getDeltaTime());
        }
    }
    m_elapsedTime += m_rendererHandle->getDeltaTime();
}

//////////////////////////////////////////////////////////////////
/*                                                              */
/*                           GUI                                */
/*                                                              */
//////////////////////////////////////////////////////////////////

void Simulation::displayMainCtrlWindow()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowBgAlpha(WINDOWS_BG_ALPHA);
    ImGui::SetNextWindowSizeConstraints(
        MAIN_CTRL_WINDOW_MIN_SIZE,
        ImVec2(m_particleAddWindowInfo.pos.x, static_cast<float>(m_rendererHandle->getFramebufferHeight()) / 2.0f));
    if (!ImGui::Begin("Options"))
    {
        ImGui::End();
        return;
    }
    m_mainCtrlWindowInfo.size = ImGui::GetWindowSize();
    m_mainCtrlWindowInfo.pos = ImGui::GetWindowPos();

    ImVec2 mousePositionAbsolute = ImGui::GetMousePos();
    ImVec2 screenPositionAbsolute = ImGui::GetItemRectMin();
    ImVec2 mousePositionRelative =
        ImVec2(mousePositionAbsolute.x - screenPositionAbsolute.x, mousePositionAbsolute.y - screenPositionAbsolute.y);
    ImGui::Text("Time Elapsed: %fs", m_elapsedTime.load());
    // ImGui::Text("Position: %f, %f", mousePositionRelative.x,
    // mousePositionRelative.y);
    ImGui::Text("Own Delta Time: %fs", m_rendererHandle->getDeltaTime());
    ImGui::Text("ImGui Delta Time: %fs", ImGui::GetIO().DeltaTime);
    ImGui::Text("Framerate: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Camera (x, y, z): (%.1f, %.1f, %.1f)", m_camera.position().x, m_camera.position().y,
                m_camera.position().z);
    ImGui::Text("Time to calculate: %.2f", m_timeToCalculateAllParticlePos);

    static double simulationTime = DEFAULT_SIMULATION_TIME;
    static double timeStep = DEFAULT_TIME_STEP;

    ImGui::InputDouble("Sim Time(s)", &simulationTime);

    ImGui::InputDouble("Time step(s)", &timeStep);

    const char *modes[] = {"1. STATIC", "2. PRECALCULATED ALL", "3. PRECALCULATED 20MS", "4. REAL TIME"};
    static const char *currentMode = modes[2];

    if (ImGui::BeginCombo("Mode##modeCombo", currentMode))
    {
        for (int n = 0; n < IM_ARRAYSIZE(modes); n++)
        {
            bool isSelected = (std::strcmp(currentMode, modes[n]) == 0);
            if (ImGui::Selectable(modes[n], isSelected) && m_paused)
            {
                currentMode = modes[n];
                switch (currentMode[0])
                {
                case '1':
                    m_mode = SimulationMode::Static;
                    break;
                case '2':
                    m_mode = SimulationMode::PrecalculatedAll;
                    break;
                case '3':
                    m_mode = SimulationMode::Precalculated20Ms;
                    break;
                case '4':
                    m_mode = SimulationMode::Realtime;
                    break;
                }
            }
        }
        ImGui::EndCombo();
    }

    static const char *currentMethod = Constants::methods[0];

    if (ImGui::BeginCombo("Method##methodCombo", currentMethod))
    {
        for (int n = 0; n < IM_ARRAYSIZE(Constants::methods); n++)
        {
            bool isSelected = (std::strcmp(currentMethod, Constants::methods[n]) == 0);
            if (ImGui::Selectable(Constants::methods[n], isSelected) && m_paused)
            {
                currentMethod = Constants::methods[n];
                switch (currentMethod[0])
                {
                case '1':
                    m_method = Types::OdeMethod::RK4;
                    break;
                case '2':
                    m_method = Types::OdeMethod::ForwardEuler;
                    break;
                case '3':
                    m_method = Types::OdeMethod::Leapfrog;
                    break;
                }
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Set all") && m_paused)
    {
        for (auto &particle : m_particles)
        {
            particle.setMethodMask(m_method);
        }
    }

    // ImGui::Checkbox("Multithreading(EXPERIMENTAL)", &m_threadedCalculation);

    if (ImGui::Button("Start", START_BUTTON_SIZE) && m_paused)
    {
        m_timeStep = timeStep;
        if (getStepsPer20ms() <= getMaxStepsBuffered())
        {
            m_simulationTime = simulationTime;
            m_skipUpdate = true;
            startSimulation();
        }
    }

    if (getStepsPer20ms() > getMaxStepsBuffered())
    {
        ImGui::SameLine();
        ImGui::Text("Reduce Time Step or amount of particles");
    }

    if (ImGui::Button("Pause", PRESET1_BUTTON_SIZE) && not m_paused)
    {
        m_paused = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Resume", PRESET1_BUTTON_SIZE) && m_paused)
    {
        m_paused = false;
    }
    ImGui::SameLine();
    ImGui::Text("Paused: %s", m_paused ? "true" : "false");
    ImGui::SameLine();
    ImGui::Text("Hung: %s", m_isHung ? "true" : "false");

    if (ImGui::Button("Current state as initial", SET_INIT_STATE_BUTTON_SIZE) && m_paused)
    {
        for (auto &particle : m_particles)
        {
            particle.setInitialState();
        }
        restartSimulation();
    }

    if (ImGui::Button("Restart", PRESET1_BUTTON_SIZE) && m_paused)
    {
        restartSimulation();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset All", PRESET1_BUTTON_SIZE) && m_paused)
    {
        resetAll();
    }

    ImGui::PushItemWidth(148.0f);
    ImGui::DragScalar("Distance Softening", ImGuiDataType_Double, &Particle::distanceSoftening, 0.0005f,
                      &Particle::MIN_DISTANCE_SOFTENING, &Particle::MAX_DISTANCE_SOFTENING, "%.4f");
    ImGui::PopItemWidth();

    ImGui::Text("Steps per 20ms: %d", getStepsPer20ms());

    ImGui::Text("Max buffered: %d", getMaxStepsBuffered());

    if (ImGui::Button("Dump states") && m_paused && m_mode == SimulationMode::PrecalculatedAll)
    {
        unsigned int timestamp = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
        std::filesystem::path dumpDir = std::to_string(timestamp);
        if (std::filesystem::create_directory(dumpDir))
        {
            for (auto &particle : m_particles)
            {
                std::ofstream file(dumpDir / (std::to_string(particle.getId()) + ".csv"));
                for (const auto &[id, state] : particle.statesData())
                {
                    file << state.pos.x() << ',' << state.pos.y() << ',' << state.pos.z() << ',' << state.velocity.x()
                         << ',' << state.velocity.y() << ',' << state.velocity.z() << '\n';
                }
            }
        }
    }

    ImGui::End();
}

void Simulation::displayParticleListWindow()
{
    std::vector<std::vector<Particle>::iterator> particlesToRemove;
    ImGui::SetNextWindowPos(ImVec2(m_rendererHandle->getFramebufferWidth() - m_particleListWindowInfo.size.x, 0.0f));
    ImGui::SetNextWindowBgAlpha(WINDOWS_BG_ALPHA);
    ImGui::SetNextWindowSizeConstraints(
        PARTICLE_LIST_WINDOW_MIN_SIZE,
        ImVec2(m_rendererHandle->getFramebufferWidth() - MAIN_CTRL_WINDOW_MIN_SIZE.x - PARTICLE_ADD_WINDOW_MIN_SIZE.x,
               m_rendererHandle->getFramebufferHeight() * 0.9f));
    if (!ImGui::Begin("Particles"))
    {
        ImGui::End();
        return;
    }
    m_particleListWindowInfo.size = ImGui::GetWindowSize();
    m_particleListWindowInfo.pos = ImGui::GetWindowPos();
    if (ImGui::BeginTable("ParticleTable", 5))
    {
        ImGui::TableSetupColumn("Particle ID");
        ImGui::TableSetupColumn("Pos[m]");
        ImGui::TableSetupColumn("Vel[m/s]");
        ImGui::TableSetupColumn("Acc[m/s^2]");
        ImGui::TableSetupColumn("F[N]");
        ImGui::TableHeadersRow();
        for (auto it = m_particles.begin(); it != m_particles.end(); ++it)
        {
            Particle &particle = *it;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Button(std::format("X##{}", particle.getId()).c_str()) && m_paused)
            {
                particlesToRemove.push_back(it);
            }
            ImGui::SameLine();
            if (ImGui::CollapsingHeader(particleHeaderText(particle).c_str()) &&
                ((m_mode == SimulationMode::PrecalculatedAll ||
                  m_mode == SimulationMode::Precalculated20Ms &&
                      (m_elapsedTime == 0.0 || m_elapsedTime == m_simulationTime)) ||
                 m_mode == SimulationMode::Static || m_mode == SimulationMode::Realtime))
            {
                ImGui::InputDouble(std::format("Ch[C]##chargel{}", particle.getId()).c_str(), &particle.chargeData());
                ImGui::InputDouble(std::format("M[kg]##massl{}", particle.getId()).c_str(), &particle.massData());
                ImGui::InputDouble(std::format("X[m]##posxl{}", particle.getId()).c_str(), &particle.posData().x());
                ImGui::InputDouble(std::format("Y[m]##posyl{}", particle.getId()).c_str(), &particle.posData().y());
                ImGui::InputDouble(std::format("Z[m]##poszl{}", particle.getId()).c_str(), &particle.posData().z());
                ImGui::Checkbox(std::format("Mvbl##movable{}", particle.getId()).c_str(), &particle.movableData());

                auto currentSelected = Constants::methods[static_cast<int>(particle.getMethodMask()) - 1];
                if (ImGui::BeginCombo(std::format("Method##method{}", particle.getId()).c_str(), currentSelected))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(Constants::methods); n++)
                    {
                        bool isSelected = (std::strcmp(currentSelected, Constants::methods[n]) == 0);
                        if (ImGui::Selectable(Constants::methods[n], isSelected) && m_paused)
                        {
                            currentSelected = Constants::methods[n];
                            switch (currentSelected[0])
                            {
                            case '1':
                                particle.setMethodMask(Types::OdeMethod::RK4);
                                break;
                            case '2':
                                particle.setMethodMask(Types::OdeMethod::ForwardEuler);
                                break;
                            case '3':
                                particle.setMethodMask(Types::OdeMethod::Leapfrog);
                                break;
                            }
                        }
                    }
                    ImGui::EndCombo();
                }
                if (m_mode == SimulationMode::Static)
                {
                    particle.setInitialState();
                    particle.clearStates();
                    particle.pushState(0);
                }
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text(Helpers::vectorFormat(particle.getPos()).c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text(Helpers::vectorFormat(particle.getVelocity()).c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::Text(Helpers::vectorFormat(particle.getAcceleration()).c_str());
            ImGui::TableSetColumnIndex(4);
            ImGui::Text(Helpers::vectorFormat(particle.getAffectingForce()).c_str());
        }
        ImGui::EndTable();
    }
    ImGui::End();
    for (auto &it : particlesToRemove)
    {
        removeParticle(it);
    }
}

void Simulation::displayParticleAddWindow()
{
    ImGui::SetNextWindowPos(ImVec2(m_rendererHandle->getFramebufferWidth() - m_particleListWindowInfo.size.x -
                                       m_particleAddWindowInfo.size.x,
                                   0.0f));
    ImGui::SetNextWindowBgAlpha(WINDOWS_BG_ALPHA);
    ImGui::SetNextWindowSizeConstraints(
        PARTICLE_ADD_WINDOW_MIN_SIZE,
        ImVec2(m_rendererHandle->getFramebufferWidth() - m_mainCtrlWindowInfo.size.x - m_particleListWindowInfo.size.x,
               static_cast<float>(m_rendererHandle->getFramebufferHeight()) / 2.0f));
    if (!ImGui::Begin("Add Particle"))
    {
        ImGui::End();
        return;
    }
    m_particleAddWindowInfo.size = ImGui::GetWindowSize();
    m_particleAddWindowInfo.pos = ImGui::GetWindowPos();

    static double charge = DEFAULT_PARTICLE_CHARGE;
    static double mass = DEFAULT_PARTICLE_MASS;
    static bool movable = DEFAULT_PARTICLE_MOVABLE;
    static Types::Vec3d pos = DEFAULT_PARTICLE_POSITION;
    static Types::Vec3d vel = DEFAULT_PARTICLE_VELOCITY;

    static int chargePrefixIdx = 2;
    static int massPrefixIdx = 3;
    static int distancePrefixIdx = 0;
    static int velocityPrefixIdx = 0;

    const std::string &chargeText =
        std::format("Charge [{}C]", UNIT_PREFIXES[chargePrefixIdx] == "none" ? "" : UNIT_PREFIXES[chargePrefixIdx]);
    ImGui::Text(chargeText.c_str());
    ImGui::SameLine();
    displayUnitSelector(chargeText, chargePrefixIdx);
    ImGui::InputDouble("##charge", &charge);

    const std::string &massText =
        std::format("Mass [{}g]", UNIT_PREFIXES[massPrefixIdx] == "none" ? "" : UNIT_PREFIXES[massPrefixIdx]);
    ImGui::Text(massText.c_str());
    ImGui::SameLine();
    displayUnitSelector(massText, massPrefixIdx);
    ImGui::InputDouble("##mass", &mass);

    const std::string &distanceText =
        std::format("Pos [{}m]", UNIT_PREFIXES[distancePrefixIdx] == "none" ? "" : UNIT_PREFIXES[distancePrefixIdx]);
    ImGui::Text(distanceText.c_str());
    ImGui::SameLine();
    displayUnitSelector(distanceText, distancePrefixIdx);

    ImGui::InputDouble("X##posx", &pos.x());
    ImGui::InputDouble("Y##posy", &pos.y());
    ImGui::InputDouble("Z##posz", &pos.z());

    const std::string &velocityText =
        std::format("Vel [{}m/s]", UNIT_PREFIXES[velocityPrefixIdx] == "none" ? "" : UNIT_PREFIXES[velocityPrefixIdx]);
    ImGui::Text(velocityText.c_str());
    ImGui::SameLine();
    displayUnitSelector(velocityText, velocityPrefixIdx);

    ImGui::InputDouble("X##velx", &vel.x());
    ImGui::InputDouble("Y##vely", &vel.y());
    ImGui::InputDouble("Z##velz", &vel.z());

    ImGui::Checkbox("Movable", &movable);

    if (not(m_paused || (m_mode == SimulationMode::Realtime || m_mode == SimulationMode::Static)))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Add"))
    {
        addParticle(
            Particle(charge * Constants::unitPrefixFactor<double>(
                                  UNIT_PREFIXES[chargePrefixIdx] == "none" ? ' ' : UNIT_PREFIXES[chargePrefixIdx][0]),
                     mass *
                         Constants::unitPrefixFactor<double>(
                             UNIT_PREFIXES[massPrefixIdx] == "none" ? ' ' : UNIT_PREFIXES[massPrefixIdx][0]) /
                         1000.0,
                     movable,
                     Types::Vec3d(pos.x(), pos.y(), pos.z()) *
                         Constants::unitPrefixFactor<double>(
                             UNIT_PREFIXES[distancePrefixIdx] == "none" ? ' ' : UNIT_PREFIXES[distancePrefixIdx][0]),
                     Types::Vec3d(vel.x(), vel.y(), vel.z()) *
                         Constants::unitPrefixFactor<double>(
                             UNIT_PREFIXES[velocityPrefixIdx] == "none" ? ' ' : UNIT_PREFIXES[velocityPrefixIdx][0]),
                     m_method));
        charge = DEFAULT_PARTICLE_CHARGE;
        mass = DEFAULT_PARTICLE_MASS;
        movable = DEFAULT_PARTICLE_MOVABLE;
        pos = DEFAULT_PARTICLE_POSITION;
        vel = DEFAULT_PARTICLE_VELOCITY;
    }

    displayPresetButtons();

    ImGui::End();
}

void Simulation::displayPlotWindow()
{
    ImGui::SetNextWindowBgAlpha(WINDOWS_BG_ALPHA);
    ImGui::SetNextWindowSizeConstraints(PLOT_WINDOW_MIN_SIZE,
                                        ImVec2(PLOT_WINDOW_MIN_SIZE.x * 2, PLOT_WINDOW_MIN_SIZE.y * 2));
    if (!ImGui::Begin("Plot") || m_particles.size() == 0)
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginCombo("Particle##modeCombo", std::to_string((*m_plotSelectedParticle).getId()).c_str()))
    {
        for (auto it = m_particles.begin(); it != m_particles.end(); it++)
        {
            bool isSelected = ((*m_plotSelectedParticle).getId() == (*it).getId());
            if (ImGui::Selectable(std::to_string((*it).getId()).c_str(), isSelected))
            {
                m_plotSelectedParticle = it;
            }
        }
        ImGui::EndCombo();
    }

    // Pre-allocate vectors to avoid repeated allocations
    const size_t stateCount = (*m_plotSelectedParticle).statesData().size();
    std::vector<double> posX;
    std::vector<double> velX;
    std::vector<double> forceX;
    std::vector<double> time;
    posX.reserve(stateCount);
    velX.reserve(stateCount);
    forceX.reserve(stateCount);
    time.reserve(stateCount);

    double posXmin = 0.0;
    double posXmax = 0.0;

    for (auto &[stepId, state] : (*m_plotSelectedParticle).statesData())
    {
        if (stepId > m_maxUsedStep)
        {
            break;
        }
        // posXmin = std::min(posXmin, state.pos.x);
        // posXmax = std::max(posXmax, state.pos.x);
        posX.push_back(state.pos.x());
        velX.push_back(state.velocity.x());
        forceX.push_back(state.affectingForce.x());
        time.push_back(stepId * m_timeStep);
    }

    if (posX.size() == 0)
    {
        ImGui::End();
        return;
    }

    ImPlot::SetNextAxisLimits(ImAxis_X1, -0.1, m_simulationTime + 0.1);
    ImPlot::SetNextAxisLimits(ImAxis_Y1, posXmin - 0.1, posXmax + 0.1);
    if (ImPlot::BeginPlot("##posxplot", ImVec2(-1, 300)))
    {
        ImPlot::SetupAxes("time[s]", "x[m]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Pos X", &time.data()[0], &posX.data()[0], posX.size());
        ImPlot::EndPlot();
    }

    ImPlot::SetNextAxisLimits(ImAxis_X1, -0.1, m_simulationTime + 0.1);
    ImPlot::SetNextAxisLimits(ImAxis_Y1, posXmin - 0.1, posXmax + 0.1);
    if (ImPlot::BeginPlot("##velxplot", ImVec2(-1, 300)))
    {
        ImPlot::SetupAxes("time[s]", "velocity[m/s]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Vel X", &time.data()[0], &velX.data()[0], velX.size());
        ImPlot::EndPlot();
    }

    ImPlot::SetNextAxisLimits(ImAxis_X1, -0.1, m_simulationTime + 0.1);
    ImPlot::SetNextAxisLimits(ImAxis_Y1, posXmin - 0.1, posXmax + 0.1);
    if (ImPlot::BeginPlot("##fxplot", ImVec2(-1, 300)))
    {
        ImPlot::SetupAxes("time[s]", "F[N]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Force X", &time.data()[0], &forceX.data()[0], forceX.size());
        ImPlot::EndPlot();
    }

    ImGui::End();
}

void Simulation::displayUnitSelector(const std::string &unit, int &prefixIdx)
{
    if (ImGui::BeginCombo(("##" + unit).c_str(), UNIT_PREFIXES[prefixIdx]))
    {
        for (int n = 0; n < IM_ARRAYSIZE(UNIT_PREFIXES); n++)
        {
            const bool is_selected = (prefixIdx == n);
            const char *selectableText = UNIT_PREFIXES[n] == "none" ? "" : UNIT_PREFIXES[n];
            if (ImGui::Selectable(UNIT_PREFIXES[n], is_selected))
            {
                prefixIdx = n;
            }
        }
        ImGui::EndCombo();
    }
}