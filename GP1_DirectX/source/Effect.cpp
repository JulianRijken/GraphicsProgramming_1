#include "pch.h"
#include "Effect.h"

#include <fstream>

Effect::Effect(ID3D11Device* devicePtr, const std::wstring& effectFileName)
{
	const std::ifstream file(effectFileName);
	if (!file)
		std::wcout << "File does not exist" << std::endl;

	m_EffectPtr = LoadEffect(devicePtr, effectFileName);
	m_TechniquePtr = m_EffectPtr->GetTechniqueByName(TECHNIQUE_NAME);

	m_viewProjectionMatrix = m_EffectPtr->GetVariableByName("worldViewProjection")->AsMatrix();

	if(not m_TechniquePtr->IsValid())
		std::wcout << L"Technique is not valid" << std::endl;
}

Effect::~Effect()
{
	m_EffectPtr->Release();
	m_EffectPtr = nullptr;
}

void Effect::UpdateViewProjectionMatrix(const Matrix* viewProjectionMatrix)
{
	// Assuming you have a valid pointer to your ID3DX11Effect object stored in m_pEffect
	if (m_EffectPtr)
	{
		// Assuming you have a valid ID3DX11EffectMatrixVariable pointer stored in m_pViewProjMatrixVar
		ID3DX11EffectMatrixVariable* pMatrixVar = m_viewProjectionMatrix;

		if (pMatrixVar)
		{
			// Transpose the matrix, as HLSL matrices are typically row-major
			Matrix transposedMatrix = Matrix::Transpose(*viewProjectionMatrix);


			const float* data = viewProjectionMatrix->GetMatrixAsFloatArray();

			// Set the matrix variable in the effect
			pMatrixVar->SetMatrix(data);

			delete data;

			//delete data;
		}
	}
}


ID3DX11Effect* Effect::LoadEffect(ID3D11Device* devicePtr, const std::wstring& effectFileName)
{
	HRESULT result;
	ID3D10Blob* errorBlobPtr{ nullptr };
	ID3DX11Effect* effectPtr{ nullptr };

	DWORD shaderFlags{ 0 };


#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	result = D3DX11CompileEffectFromFile
	(
		effectFileName.c_str(),
		nullptr,
		nullptr,
		shaderFlags,
		0,
		devicePtr,
		&effectPtr,
		&errorBlobPtr
	);


	if (FAILED(result))
	{
		if (errorBlobPtr != nullptr)
		{
			const char* errorsPtr = static_cast<char*>(errorBlobPtr->GetBufferPointer());

			std::wstringstream stringStream;
			for (unsigned int i = 0; i < errorBlobPtr->GetBufferSize(); i++)
				stringStream << errorsPtr[i];

			// TODO Test if this works otherwise remove it
			//for (const char* getBufferPointer : errorBlobPtr->GetBufferPointer())
			//	stringStream << getBufferPointer;

			errorBlobPtr->Release();
			errorBlobPtr = nullptr;

			std::wcout << stringStream.str() << std::endl;
		}
		else
		{
			std::wstringstream stringStream;
			stringStream << "EffectLoader failed to create affect from file!\nPath: " << effectFileName;
			std::wcout << stringStream.str() << std::endl;
			return nullptr;
		}
	}

	return effectPtr;
}
