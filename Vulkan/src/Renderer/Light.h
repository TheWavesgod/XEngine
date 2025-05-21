#pragma once

#include "Transform.h"

namespace LittleEngine
{
	enum class LightType
	{
		DIRECTIONAL = 0,
		SPOT,
		POINT,

		TYPENUM
	};

	class Light
	{
	public:
		void SetLightPosition(const glm::vec3& pos) { transform.SetPosition(pos); }
		void SetLightRotation(const glm::vec3& rot) { transform.SetRotation(rot); }
		void SetLightColor(const glm::vec3& col) { color = col; }
		void SetLightIntensity(float val) { intensity = val; }

		inline const glm::vec3& GetLightPosition() const { return transform.GetPosition(); }
		inline const glm::vec3& GetLightDirection() const { return transform.GetForwardVector(); }
		inline const glm::vec3& GetLightRotation() const { return transform.GetRotation(); }
		inline const glm::vec3& GetLightColor() const { return color; }
		inline const Transform& GetLightTransform() const { return transform; }
		inline const float GetLightIntensity() const { return intensity; }

		virtual const glm::mat4& BuildProjection() = 0;
		virtual const glm::mat4& BuildView() = 0;
		virtual const glm::mat4& BuildLightSpaceMatrix() { return BuildProjection() * BuildView(); };
	protected:
		LightType type;
		Transform transform;

		glm::vec3 color;
		float intensity = 5.0f;

	public:
		inline const LightType GetType() const { return type; }
	};

	class DirLight : public Light
	{
	public:
		const glm::mat4& BuildProjection() override;
		const glm::mat4& BuildView() override;
	};

	class SpotLight : public Light
	{
	public:
		const glm::mat4& BuildProjection() override;
		const glm::mat4& BuildView() override;

		float innerCutOff;
		float outerCutOff;

		float innerCutOffInDegree;
		float outerCutOffInDegree;

		float constant = 1.0f;
		float linear;
		float quadratic;
	};

	class PointLight : public Light
	{
	public:
		const glm::mat4& BuildProjection() override;
		const glm::mat4& BuildView() override;

		const glm::mat4 BuildShadowMatrix(int i);

		float constant = 1.0f;
		float linear;
		float quadratic;
	};

}