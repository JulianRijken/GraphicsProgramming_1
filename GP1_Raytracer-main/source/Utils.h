#pragma once
#include <cassert>
#include <fstream>
#include <iostream>

#include "Math.h"
#include "DataTypes.h"

namespace dae
{
	constexpr float EPSILON = 0.001f;


	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS
		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const Vector3 rayOriginToSphere = ray.origin - sphere.origin;

			// Calculate coefficients of the quadratic equation for ray-sphere intersection.
			const float b = Vector3::Dot(rayOriginToSphere, ray.direction);
			const float c = Vector3::Dot(rayOriginToSphere, rayOriginToSphere) - (sphere.radius * sphere.radius);
			const float discriminant = b * b - c;

			// Check if ray intersects
			if (discriminant < 0)
				return false;

			const float distance = -b - std::sqrt(discriminant);

			if (distance < ray.min || distance > ray.max)
				return false;

			//if (not ignoreHitRecord)
			//{
				hitRecord.t = distance;
				hitRecord.point = ray.origin + ray.direction * hitRecord.t;
				hitRecord.normal = (hitRecord.point - sphere.origin) / sphere.radius;
				hitRecord.didHit = true;
				hitRecord.materialIndex = sphere.materialIndex;
			//}

			return true;
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const float normalDot = Vector3::Dot(ray.direction, plane.normal);

			// Don't render back of plane
			if (normalDot > 0)
				return false;

			const float distance = Vector3::Dot(plane.origin - ray.origin, plane.normal) / normalDot;

			// Check if the intersection point is behind the ray's origin
			if (distance < ray.min || distance > ray.max)
				return false;

			//if (not ignoreHitRecord)
			//{
				hitRecord.t = distance;
				hitRecord.point = ray.origin + ray.direction * distance;
				hitRecord.normal = plane.normal;
				hitRecord.didHit = true;
				hitRecord.materialIndex = plane.materialIndex;
			//}

			return true;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest
		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const Vector3 L{ triangle.v0 - ray.origin };

			if (triangle.cullMode == TriangleCullMode::BackFaceCulling)
			{
				if (Vector3::Dot(triangle.normal,ray.direction) > -EPSILON)
					return false; 
			}
			else if (triangle.cullMode == TriangleCullMode::FrontFaceCulling)
			{
				if (Vector3::Dot(triangle.normal, ray.direction) < EPSILON)
					return false; 
			}

			const float t{ Vector3::Dot(L,triangle.normal) / Vector3::Dot(ray.direction,triangle.normal) };

			if (t < ray.min || t > ray.max)
				return false;

			const Vector3 P{ ray.origin + ray.direction * t };

			Vector3 e{};
			Vector3 p{};

			e = triangle.v1 - triangle.v0;
			p = P - triangle.v0;
			if (Vector3::Dot(Vector3::Cross(e, p),  triangle.normal) < 0.0f)
				return false;


			e = triangle.v2 - triangle.v1;
			p = P - triangle.v1;
			if (Vector3::Dot(Vector3::Cross(e, p), triangle.normal) < 0.0f)
				return false;


			e = triangle.v0 - triangle.v2;
			p = P - triangle.v2;
			if (Vector3::Dot(Vector3::Cross(e, p), triangle.normal) < 0.0f)
				return false;
			
			hitRecord.t = t;
			hitRecord.point = ray.origin + ray.direction * t;
			hitRecord.normal = triangle.normal;
			hitRecord.didHit = true;
			hitRecord.materialIndex = triangle.materialIndex;

			return true;
		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region TriangeMesh HitTest
		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			bool hitAnything = false;
			float closestHitDistance = std::numeric_limits<float>::max();

			for (size_t i = 0; i < mesh.indices.size(); i += 3)
			{
				const int i0 = mesh.indices[i];
				const int i1 = mesh.indices[i + 1];
				const int i2 = mesh.indices[i + 2];

				const Vector3& v0 = mesh.transformedPositions[i0];
				const Vector3& v1 = mesh.transformedPositions[i1];
				const Vector3& v2 = mesh.transformedPositions[i2];

				const Vector3 edge1 = v1 - v0;
				const Vector3 edge2 = v2 - v0;
				const Vector3 h = Vector3::Cross(ray.direction, edge2);

				const float a = Vector3::Dot(edge1, h);

				if (mesh.cullMode == TriangleCullMode::BackFaceCulling)
				{
					if (a < -EPSILON)
						continue;
				}
				else if (mesh.cullMode == TriangleCullMode::FrontFaceCulling)
				{
					if (a > -EPSILON)
						continue;
				}



				const float f = 1.0f / a;
				const Vector3 s = ray.origin - v0;
				const float u = f * Vector3::Dot(s, h);

				if (u >= 0.0f && u <= 1.0f)
				{
					const Vector3 q = Vector3::Cross(s, edge1);
					const float v = f * Vector3::Dot(ray.direction, q);

					if (v >= 0.0f && u + v <= 1.0f)
					{
						const float t = f * Vector3::Dot(edge2, q);

						if (t > ray.min && t < ray.max && t < closestHitDistance)
						{
							closestHitDistance = t;

							hitRecord.t = t;
							hitRecord.point = ray.origin + ray.direction * t;
							hitRecord.normal = Vector3::Cross(edge1, edge2).Normalized();
							hitRecord.didHit = true;
							hitRecord.materialIndex = mesh.materialIndex;

							hitAnything = true;
						}
					}
				}

			}

			return hitAnything;
		}



		//	const Vector3 edge1 = v1 - v0;
//	const Vector3 edge2 = v2 - v0;
//	const Vector3 h = Vector3::Cross(ray.direction, edge2);

//	const float a = Vector3::Dot(edge1, h);

//	if (a > -EPSILON || a < EPSILON)
//	{
//		const float f = 1.0f / a;
//		const Vector3 s = ray.origin - v0;
//		const float u = f * Vector3::Dot(s, h);

//		if (u >= 0.0f && u <= 1.0f)
//		{
//			const Vector3 q = Vector3::Cross(s, edge1);
//			const float v = f * Vector3::Dot(ray.direction, q);

//			if (v >= 0.0f && u + v <= 1.0f)
//			{
//				const float t = f * Vector3::Dot(edge2, q);

//				if (t > ray.min && t < ray.max && t < closestHitDistance)
//				{
//					closestHitDistance = t;

//					hitRecord.t = t;
//					hitRecord.point = ray.origin + ray.direction * t;
//					hitRecord.normal = Vector3::Cross(edge1, edge2).Normalized();
//					hitRecord.didHit = true;
//					hitRecord.materialIndex = mesh.materialIndex;

//					hitAnything = true;
//				}
//			}
//		}
//	}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_TriangleMesh(mesh, ray, temp, true);
		}
#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3 origin)
		{
			return { light.origin - origin};
		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			if(light.type == LightType::Point)
				return{ light.color * (light.intensity / Square((light.origin - target).Magnitude())) };
			

			if (light.type == LightType::Directional)
				return{ light.color * light.intensity };

			return {};
		}
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((int)i0 - 1);
					indices.push_back((int)i1 - 1);
					indices.push_back((int)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof()) 
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if(isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}