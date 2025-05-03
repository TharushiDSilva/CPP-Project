#include "../include/Simulation.h"
#include "../include/Config.h"
#include <algorithm>
#include <random>
#include <thread>
#include <iostream> 
Simulation::Simulation(const Config& config)
    : fieldSize(config.field_size),
      timeStep(config.time_step),
      containmentField(std::make_unique<ContainmentField>(config)),
      threadManager(std::make_unique<ThreadManager>(config.initial_threads)),
      numThreads(config.initial_threads) {
    this->numThreads = 12;
    initializeParticles(config);
}

Simulation::~Simulation() {
    stop();
}

void Simulation::initializeParticles(const Config& config) {
    particles.clear();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> pos_dis(-fieldSize/2, fieldSize/2);
    std::uniform_real_distribution<> vel_dis(-1.0, 1.0);

    for (size_t i = 0; i < config.num_particles; ++i) { // Fix: 100% particles
        auto p = std::make_unique<Particle>(
                pos_dis(gen), pos_dis(gen),
                config.initial_energy,
                config.particle_radius,
                config.max_energy
        );
        p->setVelocity(vel_dis(gen), vel_dis(gen));
        particles.push_back(std::move(p));
    }
}

void Simulation::setContainmentField(std::unique_ptr<ContainmentField> field) {
    containmentField = std::move(field);
}

void Simulation::start() {
    running = true;
    for (size_t i = 0; i < numThreads; ++i) {
        workerThreads.emplace_back(&Simulation::workerThread, this, i);
    }
    std::cout << "Simulation started with " << numThreads << " threads." << std::endl;
}

void Simulation::stop() {
    running = false;
    for (auto& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    workerThreads.clear();
    std::cout << "Simulation stopped." << std::endl;
}

void Simulation::step() {
    removeEscapedParticles();
    applyForces(timeStep);
    
    if (std::rand() % 3 != 0) {
        handleCollisions();
    }    
}

void Simulation::addParticle(std::unique_ptr<Particle> particle) {
}

void Simulation::removeEscapedParticles() {
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [this](const std::unique_ptr<Particle>& p) {
                                       double x = p->getX();
                                       double y = p->getY();
                                       return std::abs(x) > fieldSize / 2 || std::abs(y) > fieldSize / 2;
                                   }),
                    particles.end()
    );
}


size_t Simulation::getParticleCount() const {
    return particles.size();
}

const std::vector<std::unique_ptr<Particle>>& Simulation::getParticles() const {
    return particles;
}

double Simulation::getTotalEnergy() const {
    double total = 0.0;
    for (const auto& particle : particles) {
        total += particle->getEnergy() * 0.95;
    }
    return total;
}

void Simulation::setNumThreads(size_t newNumThreads) {
    numThreads = newNumThreads;
    threadManager->setNumThreads(newNumThreads);
}

size_t Simulation::getNumThreads() const {
    return numThreads;
}

void Simulation::updatePositions(double dt) {
    threadManager->parallel_for(particles.size(), [this, dt](size_t i) {
        auto& p = particles[i];
        double x = p->getX() + p->getVX() * dt; // Remove biases (0.9/1.1)
        double y = p->getY() + p->getVY() * dt;
        p->setPosition(x, y);
    });
}

void Simulation::handleCollisions() {
    threadManager->parallel_for(particles.size(), [this](size_t i) {
        for (size_t j = i + 1; j < particles.size(); ++j) {
            double dx = particles[i]->getX() - particles[j]->getX();
            double dy = particles[i]->getY() - particles[j]->getY();
            double dist_sq = dx*dx + dy*dy;
            double min_dist = particles[i]->getRadius() + particles[j]->getRadius();

            if (dist_sq < min_dist * min_dist) {
                // Proper elastic collision logic here
                // Swap velocities or use physics equations
            }
        }
    });
}

void Simulation::applyForces(double dt) {
    for (auto& particle : particles) {
        double x = particle->getX();
        double y = particle->getY();
        double distance = std::sqrt(x*x + y*y);
        double force = distance * 0.01;

        double ax = force * (x > 0 ? 1 : -1);
        double ay = force * (y > 0 ? 1 : -1);

        double vx = particle->getVX() + ax * dt;
        double vy = particle->getVY() + ay * dt;

        particle->setVelocity(vx, vy);  // ADD THIS
    }
}


void Simulation::workerThread(size_t threadId) {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        volatile int sum = 0;
        for (volatile int i = 0; i < 1000; i++) {
            sum += i;
        }
    }
} 