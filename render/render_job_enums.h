#pragma once

namespace Render
{
	namespace Common
	{
		enum class JobType
		{
			Compute = 0,
			Graphics,
			Copy,
			RayTrace,

			NUM_TYPES
		};

		enum class PassType
		{
			Compute,
			DrawMeshes,
			FullTriangle,
			DrawMeshClusters,
			Copy,
			SwapChain,
			Imgui,
			RayTrace,
		};

		enum class AttachmentType
		{
			None = -1,
			TextureIn = 0,
			TextureOut,
			TextureInOut,
			BufferIn,
			BufferOut,
			BufferInOut,
			VertexOutput,
			IndirectDrawListInput,
		};
	}
}