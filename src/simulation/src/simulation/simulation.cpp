#include "simulation/simulation.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "implot.h"

#include "simulation/integration.hpp"
#include <cstring>

void Simulation::initWindow()
{
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_window = glfwCreateWindow(Constants::WIDTH, Constants::HEIGHT, "Simulation", nullptr, nullptr);
    if (!m_window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwSetWindowSizeLimits(m_window, Constants::WIDTH, Constants::HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

void Simulation::addParticle(Particle &&particle)
{
    m_particles.emplace_back(std::move(particle));
    if (m_mode == SimulationMode::PrecalculatedAll || m_mode == SimulationMode::Precalculated20Ms)
        restartSimulation();
    m_plotSelectedParticle = m_particles.front().getId();
}

void Simulation::removeParticle(std::vector<Particle>::iterator &it)
{
    if (it != m_particles.end())
    {
        (*it).cleanup();
        it = m_particles.erase(it);
        if (m_mode == SimulationMode::PrecalculatedAll || m_mode == SimulationMode::Precalculated20Ms)
            restartSimulation();
        if (m_particles.size() > 0)
            m_plotSelectedParticle = m_particles.front().getId();
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

void Simulation::calculateParticlePositions(bool all)
{
    uint32_t startingStep{0};
    if (not all)
    {
        startingStep = m_mutualMaxStep + 1;
        m_mutualMaxStep =
            m_mutualMaxStep +
            std::min(getStepsPer20ms(), static_cast<uint32_t>(m_simulationTime / m_timeStep) - m_mutualMaxStep);
    }
    else
    {
        startingStep = 1;
        m_mutualMaxStep = static_cast<uint32_t>(m_simulationTime / m_timeStep);
    }

    calculateSteps(startingStep);
}

void Simulation::calculateSteps(uint32_t startingStep)
{
    const auto start = std::chrono::steady_clock::now();
    std::vector<Physics::Body> bodies;
    bodies.reserve(m_particles.size());
    for (uint32_t step = startingStep; step <= m_mutualMaxStep; ++step)
    {
        bodies.clear();
        for (auto &particle : m_particles)
        {
            const auto &previous = particle.statesData().at(step - 1);
            bodies.push_back({previous.pos, previous.velocity, particle.getCharge(), particle.getMass(),
                              particle.isMovable(), particle.getMethodMask()});
        }
        Physics::advance(bodies, m_timeStep, Particle::distanceSoftening);
        auto forces = Physics::forces(bodies, Particle::distanceSoftening);
        for (size_t i = 0; i < m_particles.size(); ++i)
        {
            auto acceleration = forces[i] / bodies[i].mass;
            m_particles[i].pushState(step, forces[i], acceleration, bodies[i].velocity, bodies[i].position);
        }
    }
    m_timeToCalculateAllParticlePos =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

bool Simulation::updatePositions()
{
    for (auto it = m_particles.begin(); it != m_particles.end(); it++)
    {
        if (not(*it).isMovable())
        {
            continue;
        }
        if (!it->updateFromPrecalcPos(m_maxUsedStep))
            return false;
    }
    return true;
}

void Simulation::startSimulation()
{
    restartSimulation();
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
        std::vector<Physics::Body> initialBodies;
        for (const auto &particle : m_particles)
            initialBodies.push_back({particle.getPos(), particle.getVelocity(), particle.getCharge(),
                                     particle.getMass(), particle.isMovable(), particle.getMethodMask()});
        const auto initialForces = Physics::forces(initialBodies, Particle::distanceSoftening);
        for (size_t i = 0; i < m_particles.size(); ++i)
        {
            auto &particle = m_particles[i];
            particle.setAffectingForce(initialForces[i]);
            particle.setAcceleration(initialForces[i] / particle.getMass());
            particle.statesData().at(0).affectingForce = particle.getAffectingForce();
            particle.statesData().at(0).acceleration = particle.getAcceleration();
        }

        switch (m_mode)
        {
        case SimulationMode::Static:
            break;
        case SimulationMode::PrecalculatedAll:
            calculateParticlePositions(true);
            break;
        case SimulationMode::Precalculated20Ms:
            break;
        case SimulationMode::Realtime:
            break;
        }
        m_hasStarted = true;
        m_paused = false;
    }
}

void Simulation::resetAll()
{
    m_skipUpdate = true;
    m_paused = true;
    m_hasStarted = false;
    m_mutualMaxStep = 0;
    m_maxUsedStep = 0;
    m_elapsedTime = 0.0;

    for (auto &particle : m_particles)
    {
        particle.cleanup();
    }
    m_particles.clear();
    m_plotSelectedParticle.reset();
    Particle::resetId();
}

void Simulation::restartSimulation()
{
    m_skipUpdate = true;
    m_paused = true;
    m_hasStarted = false;
    m_mutualMaxStep = 0;
    m_maxUsedStep = 0;
    m_elapsedTime = 0.0;

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
        for (auto &particle_other : m_particles)
        {
            if (particle.getId() != particle_other.getId())
            {
                force += particle.getCoulombForce(particle_other);
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
                return key < (m_maxUsedStep > 2000 ? m_maxUsedStep - 2000 : 0);
            });
        }
    }
    if (m_mutualMaxStep - std::min(m_maxUsedStep, m_mutualMaxStep) < getStepsPer20ms())
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
    // Compute all forces before changing any positions.
    for (auto &particle : m_particles)
    {
        Types::Vec3d force(0.0);
        for (auto &other : m_particles)
            if (particle.getId() != other.getId() && particle.getMethodMask() == other.getMethodMask())
                force += particle.getCoulombForce(other);
        particle.setAffectingForce(force);
    }
    for (auto &particle : m_particles)
        particle.update(m_rendererHandle->getDeltaTime());
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
                restartSimulation();
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

    const bool validTiming =
        std::isfinite(timeStep) && timeStep > 0.0 && std::isfinite(simulationTime) && simulationTime > 0.0 &&
        simulationTime / timeStep < double(std::numeric_limits<uint32_t>::max() - 1) &&
        std::ceil(0.02 / timeStep) <= getMaxStepsBuffered() &&
        (m_mode != SimulationMode::PrecalculatedAll || simulationTime / timeStep <= getMaxStepsBuffered());
    ImGui::BeginDisabled(!validTiming);
    if (ImGui::Button("Start", START_BUTTON_SIZE) && m_paused)
    {
        m_timeStep = timeStep;
        m_simulationTime = simulationTime;
        m_skipUpdate = true;
        startSimulation();
    }
    ImGui::EndDisabled();
    if (!validTiming)
        ImGui::TextUnformatted("Use positive finite times; increase timestep or reduce duration/particle count.");

    if (ImGui::Button("Pause", PRESET1_BUTTON_SIZE) && not m_paused)
    {
        m_paused = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Resume", PRESET1_BUTTON_SIZE) && m_paused)
    {
        if (!m_hasStarted)
            startSimulation();
        else
            m_paused = false;
    }
    ImGui::SameLine();
    ImGui::Text("Paused: %s", m_paused ? "true" : "false");
    ImGui::SameLine();

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
                  (m_mode == SimulationMode::Precalculated20Ms &&
                   (m_elapsedTime == 0.0 || m_elapsedTime == m_simulationTime))) ||
                 m_mode == SimulationMode::Static || m_mode == SimulationMode::Realtime))
            {
                ImGui::InputDouble(std::format("Ch[C]##chargel{}", particle.getId()).c_str(), &particle.chargeData());
                ImGui::InputDouble(std::format("M[kg]##massl{}", particle.getId()).c_str(), &particle.massData());
                particle.setMass(particle.getMass());
                ImGui::InputDouble(std::format("X[m]##posxl{}", particle.getId()).c_str(), &particle.posData().x());
                ImGui::InputDouble(std::format("Y[m]##posyl{}", particle.getId()).c_str(), &particle.posData().y());
                ImGui::InputDouble(std::format("Z[m]##poszl{}", particle.getId()).c_str(), &particle.posData().z());
                ImGui::Checkbox(std::format("Mvbl##movable{}", particle.getId()).c_str(), &particle.movableData());

                const int methodIndex = static_cast<int>(particle.getMethodMask()) - 1;
                const char *currentSelected = methodIndex >= 0 && methodIndex < IM_ARRAYSIZE(Constants::methods)
                                                  ? Constants::methods[methodIndex]
                                                  : "Unknown";
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
            ImGui::TextUnformatted(Helpers::vectorFormat(particle.getPos()).c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(Helpers::vectorFormat(particle.getVelocity()).c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(Helpers::vectorFormat(particle.getAcceleration()).c_str());
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(Helpers::vectorFormat(particle.getAffectingForce()).c_str());
        }
        ImGui::EndTable();
    }
    ImGui::End();
    for (auto it = particlesToRemove.rbegin(); it != particlesToRemove.rend(); ++it)
        removeParticle(*it);
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
        std::format("Charge [{}C]", chargePrefixIdx == 0 ? "" : UNIT_PREFIXES[chargePrefixIdx]);
    ImGui::TextUnformatted(chargeText.c_str());
    ImGui::SameLine();
    displayUnitSelector(chargeText, chargePrefixIdx);
    ImGui::InputDouble("##charge", &charge);

    const std::string &massText = std::format("Mass [{}g]", massPrefixIdx == 0 ? "" : UNIT_PREFIXES[massPrefixIdx]);
    ImGui::TextUnformatted(massText.c_str());
    ImGui::SameLine();
    displayUnitSelector(massText, massPrefixIdx);
    ImGui::InputDouble("##mass", &mass);

    const std::string &distanceText =
        std::format("Pos [{}m]", distancePrefixIdx == 0 ? "" : UNIT_PREFIXES[distancePrefixIdx]);
    ImGui::TextUnformatted(distanceText.c_str());
    ImGui::SameLine();
    displayUnitSelector(distanceText, distancePrefixIdx);

    ImGui::InputDouble("X##posx", &pos.x());
    ImGui::InputDouble("Y##posy", &pos.y());
    ImGui::InputDouble("Z##posz", &pos.z());

    const std::string &velocityText =
        std::format("Vel [{}m/s]", velocityPrefixIdx == 0 ? "" : UNIT_PREFIXES[velocityPrefixIdx]);
    ImGui::TextUnformatted(velocityText.c_str());
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
        addParticle(Particle(
            charge *
                Constants::unitPrefixFactor<double>(chargePrefixIdx == 0 ? ' ' : UNIT_PREFIXES[chargePrefixIdx][0]),
            mass * Constants::unitPrefixFactor<double>(massPrefixIdx == 0 ? ' ' : UNIT_PREFIXES[massPrefixIdx][0]) /
                1000.0,
            movable,
            Types::Vec3d(pos.x(), pos.y(), pos.z()) *
                Constants::unitPrefixFactor<double>(distancePrefixIdx == 0 ? ' ' : UNIT_PREFIXES[distancePrefixIdx][0]),
            Types::Vec3d(vel.x(), vel.y(), vel.z()) *
                Constants::unitPrefixFactor<double>(velocityPrefixIdx == 0 ? ' ' : UNIT_PREFIXES[velocityPrefixIdx][0]),
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

    auto selected = std::find_if(m_particles.begin(), m_particles.end(), [this](const Particle &particle) {
        return m_plotSelectedParticle && particle.getId() == *m_plotSelectedParticle;
    });
    if (selected == m_particles.end())
        selected = m_particles.begin();
    m_plotSelectedParticle = selected->getId();
    if (ImGui::BeginCombo("Particle##modeCombo", std::to_string(*m_plotSelectedParticle).c_str()))
    {
        for (auto it = m_particles.begin(); it != m_particles.end(); it++)
        {
            bool isSelected = (*m_plotSelectedParticle == it->getId());
            if (ImGui::Selectable(std::to_string((*it).getId()).c_str(), isSelected))
            {
                m_plotSelectedParticle = it->getId();
                selected = it;
            }
        }
        ImGui::EndCombo();
    }

    std::vector<double> posX{};
    std::vector<double> velX{};
    std::vector<double> forceX{};
    std::vector<double> time{};

    double posXmin = 0.0;
    double posXmax = 0.0;

    std::vector<uint32_t> steps;
    for (const auto &[step, state] : selected->statesData())
        if (step <= m_maxUsedStep)
            steps.push_back(step);
    std::sort(steps.begin(), steps.end());
    for (auto stepId : steps)
    {
        const auto &state = selected->statesData().at(stepId);
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
            const char *selectableText = n == 0 ? "" : UNIT_PREFIXES[n];
            if (ImGui::Selectable(UNIT_PREFIXES[n], is_selected))
            {
                prefixIdx = n;
            }
        }
        ImGui::EndCombo();
    }
}
