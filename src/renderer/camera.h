#pragma once
#include "types.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "game_input.h"
namespace xjar {

struct FirstPerson_Camera {
    glm::vec2 m_mousePosition = glm::vec2(0);
    glm::vec3 m_cameraPosition = glm::vec3(0.0f, 10.0f, 10.0f);
    glm::quat m_cameraOrientation = glm::quat(glm::vec3(0));
    glm::vec3 m_movSpeed = glm::vec3(0.0f);
    glm::vec3 m_up = glm::vec3(0.0f, 0.0f, 1.0f);
    f32       m_mouseSpeed = 1.0f;
    f32       m_acc = 150.0f;
    f32       m_damping = 0.2f;
    f32       m_maxSpeed = 10.0f;
    f32       m_fastCoef = 10.0f;

    void Setup(const glm::vec3 &pos, const glm::vec3 &target, const glm::vec3 &up) {
        m_cameraPosition = pos;
        m_cameraOrientation = glm::lookAt(pos, target, up);
        m_up = up;
    }

    void Update(f32 dt, GameInput *input, const glm::vec2 &mousePos) {
        //glm::vec2 mousePos = glm::vec2(input->mouseX, input->mouseY);

        if (input->mouseButtons[GameMouseInput_Left].pressed) {
            const glm::vec2 delta = mousePos - m_mousePosition;
            const glm::quat deltaQuat = glm::quat(glm::vec3(-m_mouseSpeed * delta.y, m_mouseSpeed * delta.x, 0.0f));
            m_cameraOrientation = glm::normalize(deltaQuat * m_cameraOrientation);
            SetUpVector(m_up);
        }

        m_mousePosition = mousePos;

        const glm::mat4 v = glm::mat4_cast(m_cameraOrientation);

        const glm::vec3 forward = -glm::vec3(v[0][2], v[1][2], v[2][2]);
        const glm::vec3 right = glm::vec3(v[0][0], v[1][0], v[2][0]);
        const glm::vec3 up = glm::cross(right, forward);

        glm::vec3 a(0.0f);
        if (input->actionUp.pressed)
            a += forward;
        if (input->actionDown.pressed)
            a -= forward;
        if (input->actionLeft.pressed)
            a -= right;
        if (input->actionRight.pressed)
            a += right;
        if (input->button1.pressed)
            a += up;
        if (input->button2.pressed)
            a -= up;
        if (input->actionAccelerate.pressed)
            a *= m_fastCoef;

        if (a == glm::vec3(0)) {
            m_movSpeed -= m_movSpeed * std::min((1.0f / m_damping) * static_cast<f32>(dt), 1.0f);
        } else {
            m_movSpeed += a * m_acc * static_cast<f32>(dt);
            const f32 maxSpeed = input->actionAccelerate.pressed ? m_maxSpeed * m_fastCoef : m_maxSpeed;
            if (glm::length(m_movSpeed) > m_maxSpeed) {
                m_movSpeed = glm::normalize(m_movSpeed) * maxSpeed;
            }
        }

        m_cameraPosition += m_movSpeed * static_cast<f32>(dt);
    }

    glm::mat4 GetViewMatrix() const {
        const glm::mat4 t = glm::translate(glm::mat4(1.0f), -m_cameraPosition);
        const glm::mat4 r = glm::mat4_cast(m_cameraOrientation);
        return r * t;
    }

    void SetUpVector(const glm::vec3 &up) {
        const glm::mat4 view = GetViewMatrix();
        const glm::vec3 direction = -glm::vec3(view[0][2], view[1][2], view[2][2]);
        m_cameraOrientation = glm::lookAt(m_cameraPosition, m_cameraPosition + direction, up);
    }
};

}
